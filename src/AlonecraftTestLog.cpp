/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license:
 * https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE-AGPL3
 */

#include "AlonecraftTestLog.h"

#include "Chat.h"
#include "CommandScript.h"
#include "GameTime.h"
#include "Log.h"
#include "Pet.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SpellAuraEffects.h"
#include "SpellAuras.h"
#include "SpellInfo.h"
#include "Timer.h"
#include "Unit.h"
#include "WarlockShards.h"

#include <atomic>
#include <fstream>
#include <mutex>

namespace
{
    std::atomic<bool> s_enabled{false};
    std::mutex        s_mutex;
    std::ofstream     s_file;
    std::string       s_path;
    std::string       s_case = "-";

    // Opens the trace file on first use. Caller must hold s_mutex.
    std::ofstream& Stream()
    {
        if (!s_file.is_open())
        {
            std::string dir = sLog->GetLogsDir();
            if (!dir.empty() && dir.back() != '/' && dir.back() != '\\')
                dir += '/';

            s_path = dir + "alonecraft_test.log";
            s_file.open(s_path, std::ios::out | std::ios::app);
        }

        return s_file;
    }

    // "2026-08-03 14:22:31 +012345 | " -- wall clock for correlating with the
    // player's own notes, plus the monotonic server ms for ordering ticks that
    // land inside the same second.
    std::string Stamp()
    {
        return Acore::StringFormat("{} +{:09} | ",
            Acore::Time::TimeToTimestampStr(Seconds(0), "%Y-%m-%d %H:%M:%S"),
            GameTime::GetGameTimeMS().count());
    }

    void WriteRaw(std::string const& line)
    {
        std::lock_guard<std::mutex> guard(s_mutex);

        std::ofstream& out = Stream();
        if (!out.is_open())
            return;

        out << line << '\n';
        out.flush();   // a crash mid-test must not lose the trace
    }
}

namespace Alonecraft::TestLog
{
    bool Enabled()
    {
        return s_enabled.load(std::memory_order_relaxed);
    }

    void SetEnabled(bool on)
    {
        s_enabled.store(on, std::memory_order_relaxed);
    }

    std::string const& FilePath()
    {
        std::lock_guard<std::mutex> guard(s_mutex);
        Stream();
        return s_path;
    }

    void Write(std::string_view tag, std::string const& message)
    {
        WriteRaw(Acore::StringFormat("{}[{}] {} {}", Stamp(), s_case, tag, message));
    }

    void Marker(std::string const& message)
    {
        WriteRaw(Acore::StringFormat("{}{}", Stamp(), message));
    }

    std::string N(Unit const* unit)
    {
        return unit ? unit->GetName() : "-";
    }
}

namespace
{
    // -----------------------------------------------------------------------
    //  State snapshot
    // -----------------------------------------------------------------------
    //  Covers the changes that have no C++ hook to instrument -- pure DBC
    //  talent redesigns show up here as an aura with an amount, a duration or
    //  a stat that moved between two snapshots.

    void DumpAuras(Unit* unit, std::string_view who)
    {
        for (auto const& pair : unit->GetAppliedAuras())
        {
            AuraApplication const* app = pair.second;
            if (!app)
                continue;

            Aura const* aura = app->GetBase();
            if (!aura)
                continue;

            SpellInfo const* info = aura->GetSpellInfo();

            std::string effects;
            for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
            {
                AuraEffect const* eff = aura->GetEffect(i);
                if (!eff)
                    continue;

                effects += Acore::StringFormat(" eff{}=aura{}:{}",
                    uint32(i), uint32(eff->GetAuraType()), eff->GetAmount());
            }

            Alonecraft::TestLog::Write("STATE.AURA",
                Acore::StringFormat("{} id={} name=\"{}\" stacks={} charges={} dur={}/{}{}",
                    who, aura->GetId(), info && info->SpellName[0] ? info->SpellName[0] : "?",
                    uint32(aura->GetStackAmount()), uint32(aura->GetCharges()),
                    aura->GetDuration(), aura->GetMaxDuration(), effects));
        }
    }

    void DumpPet(Player* player)
    {
        Pet* pet = player->GetPet();
        if (!pet)
        {
            Alonecraft::TestLog::Write("STATE.PET", "none");
            return;
        }

        Alonecraft::TestLog::Write("STATE.PET",
            Acore::StringFormat("entry={} name=\"{}\" lvl={} hp={}/{} mana={}/{} "
                "dodge={:.2f} atkTime={} ap={} armor={}",
                pet->GetEntry(), pet->GetName(), uint32(pet->GetLevel()),
                pet->GetHealth(), pet->GetMaxHealth(),
                pet->GetPower(POWER_MANA), pet->GetMaxPower(POWER_MANA),
                pet->GetUnitDodgeChance(), pet->GetAttackTime(BASE_ATTACK),
                uint32(pet->GetTotalAttackPowerValue(BASE_ATTACK)), pet->GetArmor()));

        DumpAuras(pet, "pet");
    }
}

namespace Alonecraft::TestLog
{
    void DumpState(Player* player, std::string_view label)
    {
        if (!player)
            return;

        Write("STATE", Acore::StringFormat("---- {} ---- {} lvl={} class={} spec={}",
            label, player->GetName(), uint32(player->GetLevel()),
            uint32(player->getClass()), uint32(player->GetActiveSpec()) + 1));

        Write("STATE.RES", Acore::StringFormat(
            "hp={}/{} mana={}/{} shards={} combo={}",
            player->GetHealth(), player->GetMaxHealth(),
            player->GetPower(POWER_MANA), player->GetMaxPower(POWER_MANA),
            Alonecraft::Warlock::GetSoulShardCount(player),
            uint32(player->GetComboPoints())));

        Write("STATE.STAT", Acore::StringFormat(
            "agi={:.1f} int={:.1f} str={:.1f} sta={:.1f} spi={:.1f} "
            "ap={} armor={} atkSpeedMod={:.4f}",
            player->GetStat(STAT_AGILITY), player->GetStat(STAT_INTELLECT),
            player->GetStat(STAT_STRENGTH), player->GetStat(STAT_STAMINA),
            player->GetStat(STAT_SPIRIT),
            uint32(player->GetTotalAttackPowerValue(BASE_ATTACK)),
            player->GetArmor(), player->m_modAttackSpeedPct[BASE_ATTACK]));

        Write("STATE.AVOID", Acore::StringFormat(
            "dodge={:.2f} parry={:.2f} block={:.2f} meleeCrit={:.2f} rangedCrit={:.2f}",
            player->GetFloatValue(PLAYER_DODGE_PERCENTAGE),
            player->GetFloatValue(PLAYER_PARRY_PERCENTAGE),
            player->GetFloatValue(PLAYER_BLOCK_PERCENTAGE),
            player->GetFloatValue(PLAYER_CRIT_PERCENTAGE),
            player->GetFloatValue(PLAYER_RANGED_CRIT_PERCENTAGE)));

        // Spell crit is per school; fire and shadow are the two the Warlock
        // changes move, so all seven are dumped rather than guessing.
        std::string spellCrit;
        for (uint8 school = 0; school < MAX_SPELL_SCHOOL; ++school)
            spellCrit += Acore::StringFormat(" s{}={:.2f}", uint32(school),
                player->GetFloatValue(PLAYER_SPELL_CRIT_PERCENTAGE1 + school));

        Write("STATE.SPELLCRIT", spellCrit);

        // Agility's raw contribution to dodge, so Master of Deception's
        // "33/66/100% more effective" can be checked as a ratio rather than
        // inferred from the total.
        float dodgeDim = 0.0f;
        float dodgeNonDim = 0.0f;
        player->GetDodgeFromAgility(dodgeDim, dodgeNonDim);
        Write("STATE.DODGEAGI", Acore::StringFormat(
            "diminishing={:.4f} nondiminishing={:.4f} total={:.4f}",
            dodgeDim, dodgeNonDim, dodgeDim + dodgeNonDim));

        DumpAuras(player, "self");
        DumpPet(player);

        Write("STATE", Acore::StringFormat("---- end {} ----", label));
    }
}

// ---------------------------------------------------------------------------
//  .woatest command
// ---------------------------------------------------------------------------

using namespace Acore::ChatCommands;

class woatest_commandscript : public CommandScript
{
public:
    woatest_commandscript() : CommandScript("woatest_commandscript") { }

    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable woatestTable =
        {
            { "begin", HandleBegin, SEC_PLAYER, Console::No },
            { "end",   HandleEnd,   SEC_PLAYER, Console::No },
            { "note",  HandleNote,  SEC_PLAYER, Console::No },
            { "state", HandleState, SEC_PLAYER, Console::No },
            { "on",    HandleOn,    SEC_PLAYER, Console::No },
            { "off",   HandleOff,   SEC_PLAYER, Console::No },
            { "",      HandleStatus, SEC_PLAYER, Console::No },
        };

        static ChatCommandTable commandTable =
        {
            { "woatest", woatestTable },
        };

        return commandTable;
    }

    // .woatest begin <case-id>
    // Opens a named case: enables tracing, writes a BEGIN marker, and takes a
    // "before" snapshot so value changes have a baseline.
    static bool HandleBegin(ChatHandler* handler, Tail caseId)
    {
        Player* player = handler->GetSession()->GetPlayer();
        if (!player)
            return false;

        if (caseId.empty())
        {
            handler->PSendSysMessage("Usage: .woatest begin <case-id>");
            return true;
        }

        s_case = std::string(caseId);
        Alonecraft::TestLog::SetEnabled(true);
        Alonecraft::TestLog::Marker(Acore::StringFormat(
            "======== BEGIN case={} char={} ========", s_case, player->GetName()));
        Alonecraft::TestLog::DumpState(player, "before");

        handler->PSendSysMessage("woatest: case '{}' started. Tracing ON.", s_case);
        return true;
    }

    // .woatest end
    static bool HandleEnd(ChatHandler* handler)
    {
        Player* player = handler->GetSession()->GetPlayer();
        if (!player)
            return false;

        Alonecraft::TestLog::DumpState(player, "after");
        Alonecraft::TestLog::Marker(Acore::StringFormat(
            "======== END case={} char={} ========", s_case, player->GetName()));
        Alonecraft::TestLog::SetEnabled(false);

        handler->PSendSysMessage("woatest: case '{}' ended. Tracing OFF.", s_case);
        s_case = "-";
        return true;
    }

    // .woatest note <text> -- a landmark inside a case, e.g. "pulled mob 2".
    static bool HandleNote(ChatHandler* handler, Tail text)
    {
        Alonecraft::TestLog::Marker(Acore::StringFormat("-------- NOTE case={} {}",
            s_case, std::string(text)));
        handler->PSendSysMessage("woatest: note recorded.");
        return true;
    }

    // .woatest state -- snapshot without changing the enabled flag.
    static bool HandleState(ChatHandler* handler, Tail label)
    {
        Player* player = handler->GetSession()->GetPlayer();
        if (!player)
            return false;

        // A snapshot is always worth writing even outside a case.
        bool const wasEnabled = Alonecraft::TestLog::Enabled();
        Alonecraft::TestLog::SetEnabled(true);
        Alonecraft::TestLog::DumpState(player, label.empty() ? "manual" : std::string(label));
        Alonecraft::TestLog::SetEnabled(wasEnabled);

        handler->PSendSysMessage("woatest: state written.");
        return true;
    }

    static bool HandleOn(ChatHandler* handler)
    {
        Alonecraft::TestLog::SetEnabled(true);
        handler->PSendSysMessage("woatest: tracing ON -> {}", Alonecraft::TestLog::FilePath());
        return true;
    }

    static bool HandleOff(ChatHandler* handler)
    {
        Alonecraft::TestLog::SetEnabled(false);
        handler->PSendSysMessage("woatest: tracing OFF.");
        return true;
    }

    static bool HandleStatus(ChatHandler* handler)
    {
        handler->PSendSysMessage("woatest: tracing {}, case '{}'",
            Alonecraft::TestLog::Enabled() ? "ON" : "OFF", s_case);
        handler->PSendSysMessage("file: {}", Alonecraft::TestLog::FilePath());
        handler->PSendSysMessage("commands: begin <case> | end | note <text> | state [label] | on | off");
        return true;
    }
};

void AddSC_alonecraft_testlog()
{
    new woatest_commandscript();
}
