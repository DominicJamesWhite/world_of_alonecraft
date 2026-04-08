/*
 * .spec command — switch between talent specs and manage spec count.
 *
 * Usage:
 *   .spec <1-8>         Switch to the given spec (player-accessible)
 *   .spec preview <1-8> Preview a spec's talents in the talent window
 *   .spec unlock [N]    Set spec count to N, default current+1 (GM only)
 *   .spec count         Show current spec count and active spec
 *   .spec query         Send machine-readable spec state (for addon)
 */

#include "Chat.h"
#include "CommandScript.h"
#include "DBCStores.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "Spell.h"

using namespace Acore::ChatCommands;

// Send a machine-readable system message the addon can parse:
// "MULTISPEC:active:count:preview" e.g. "MULTISPEC:3:8:5"
static void SendSpecState(Player* player, ChatHandler* handler)
{
    handler->PSendSysMessage("MULTISPEC:{}:{}:{}",
        player->GetActiveSpec() + 1,
        player->GetSpecsCount(),
        player->GetClientPreviewSpec() + 1);
}

class spec_commandscript : public CommandScript
{
public:
    spec_commandscript() : CommandScript("spec_commandscript") { }

    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable specCommandTable =
        {
            { "preview", HandleSpecPreviewCommand, SEC_PLAYER,     Console::No },
            { "unlock",  HandleSpecUnlockCommand,  SEC_GAMEMASTER, Console::No },
            { "count",   HandleSpecCountCommand,   SEC_PLAYER,     Console::No },
            { "query",   HandleSpecQueryCommand,   SEC_PLAYER,     Console::No },
            { "",        HandleSpecSwitchCommand,   SEC_PLAYER,     Console::No },
        };

        static ChatCommandTable commandTable =
        {
            { "spec", specCommandTable },
        };

        return commandTable;
    }

    // .spec <1-8>
    // Casts the "Activate Spec" spell (63645) with a cast bar, just like
    // Blizzard's dual-spec activation: ~5s cast, interrupted by movement,
    // damage, or entering combat.
    static constexpr uint32 SPELL_ACTIVATE_SPEC = 63645;

    static bool HandleSpecSwitchCommand(ChatHandler* handler, uint8 specNum)
    {
        Player* player = handler->GetSession()->GetPlayer();
        if (!player)
            return false;

        if (specNum < 1 || specNum > MAX_TALENT_SPECS)
        {
            handler->PSendSysMessage("Spec must be between 1 and {}.", MAX_TALENT_SPECS);
            return true;
        }

        if (specNum > player->GetSpecsCount())
        {
            handler->PSendSysMessage("You only have {} spec(s) unlocked.", player->GetSpecsCount());
            return true;
        }

        uint8 specIndex = specNum - 1; // 0-based internally
        if (specIndex == player->GetActiveSpec())
        {
            handler->PSendSysMessage("Spec {} is already active.", specNum);
            return true;
        }

        if (player->IsInCombat())
        {
            handler->PSendSysMessage("Cannot switch specs while in combat.");
            return true;
        }

        if (player->IsInFlight())
        {
            handler->PSendSysMessage("Cannot switch specs while in flight.");
            return true;
        }

        // Cast the spec-activation spell. SPELLVALUE_BASE_POINT0 passes
        // through CalcBaseValue which subtracts 1 (DieSides != 0), then
        // CalcValue adds max(1, DieSides) = 1 back, so damage = bp0.
        // EffectActivateSpec does ActivateSpec(damage - 1), so pass specNum.
        int32 bp0 = static_cast<int32>(specNum);
        player->CastCustomSpell(SPELL_ACTIVATE_SPEC, SPELLVALUE_BASE_POINT0, bp0, player, false);

        // Don't SendSpecState here — the cast has a cast bar. The
        // OnPlayerAfterSpecSlotChanged hook sends it after the switch completes.
        return true;
    }

    // .spec preview <1-8> — load a spec's talent data into client slot 1
    static bool HandleSpecPreviewCommand(ChatHandler* handler, uint8 specNum)
    {
        Player* player = handler->GetSession()->GetPlayer();
        if (!player)
            return false;

        if (specNum < 1 || specNum > MAX_TALENT_SPECS)
        {
            handler->PSendSysMessage("Spec must be between 1 and {}.", MAX_TALENT_SPECS);
            return true;
        }

        if (specNum > player->GetSpecsCount())
        {
            handler->PSendSysMessage("You only have {} spec(s) unlocked.", player->GetSpecsCount());
            return true;
        }

        uint8 specIndex = specNum - 1;
        player->SetClientPreviewSpec(specIndex);
        player->SendTalentsInfoData(false); // re-send packet with new preview
        SendSpecState(player, handler);
        return true;
    }

    // .spec unlock [N]
    static bool HandleSpecUnlockCommand(ChatHandler* handler, Optional<uint8> count)
    {
        Player* player = handler->GetSession()->GetPlayer();
        if (!player)
            return false;

        uint8 newCount = count.value_or(player->GetSpecsCount() + 1);

        if (newCount < 1 || newCount > MAX_TALENT_SPECS)
        {
            handler->PSendSysMessage("Spec count must be between 1 and {}.", MAX_TALENT_SPECS);
            return true;
        }

        if (newCount == player->GetSpecsCount())
        {
            handler->PSendSysMessage("You already have {} spec(s).", newCount);
            return true;
        }

        player->UpdateSpecCount(newCount);
        handler->PSendSysMessage("Spec count set to {}.", newCount);
        SendSpecState(player, handler);
        return true;
    }

    // .spec count
    static bool HandleSpecCountCommand(ChatHandler* handler)
    {
        Player* player = handler->GetSession()->GetPlayer();
        if (!player)
            return false;

        handler->PSendSysMessage("Active: spec {} of {} unlocked.",
            player->GetActiveSpec() + 1, player->GetSpecsCount());
        return true;
    }

    // .spec query (machine-readable, for addon)
    static bool HandleSpecQueryCommand(ChatHandler* handler)
    {
        Player* player = handler->GetSession()->GetPlayer();
        if (!player)
            return false;

        SendSpecState(player, handler);
        return true;
    }
};

// ----------------------------------------------------------------
// PlayerScript: send talent distributions for all specs on login
// so the MultiSpec addon can show correct icons immediately.
// Format: "MULTISPEC_TALENTS:spec:p1:p2:p3" per spec.
// ----------------------------------------------------------------
static void GetTalentTreePointsForSpec(Player* player, uint8 spec, uint8 (&points)[3])
{
    points[0] = points[1] = points[2] = 0;
    const PlayerTalentMap& talentMap = player->GetTalentMap();
    for (auto itr = talentMap.begin(); itr != talentMap.end(); ++itr)
    {
        if (itr->second->State != PLAYERSPELL_REMOVED && itr->second->IsInSpec(spec))
        {
            if (TalentEntry const* talentInfo = sTalentStore.LookupEntry(itr->second->talentID))
            {
                if (TalentTabEntry const* tab = sTalentTabStore.LookupEntry(talentInfo->TalentTab))
                {
                    if (tab->tabpage < 3)
                    {
                        uint8 currentRank = 0;
                        for (uint8 rank = 0; rank < MAX_TALENT_RANK; ++rank)
                            if (talentInfo->RankID[rank] && itr->first == talentInfo->RankID[rank])
                            {
                                currentRank = rank + 1;
                                break;
                            }
                        points[tab->tabpage] += currentRank;
                    }
                }
            }
        }
    }
}

class multispec_player_script : public PlayerScript
{
public:
    multispec_player_script() : PlayerScript("multispec_player_script") { }

    void OnPlayerAfterSpecSlotChanged(Player* player, uint8 /*newSlot*/) override
    {
        ChatHandler handler(player->GetSession());
        handler.PSendSysMessage("MULTISPEC:{}:{}:{}",
            player->GetActiveSpec() + 1,
            player->GetSpecsCount(),
            player->GetClientPreviewSpec() + 1);
    }

    void OnPlayerLogin(Player* player) override
    {
        ChatHandler handler(player->GetSession());

        // Send overall spec state
        handler.PSendSysMessage("MULTISPEC:{}:{}:{}",
            player->GetActiveSpec() + 1,
            player->GetSpecsCount(),
            player->GetClientPreviewSpec() + 1);

        // Send talent distribution for each unlocked spec (skip empty specs)
        for (uint8 i = 0; i < player->GetSpecsCount(); ++i)
        {
            uint8 points[3] = {};
            GetTalentTreePointsForSpec(player, i, points);
            if (points[0] + points[1] + points[2] > 0)
                handler.PSendSysMessage("MULTISPEC_TALENTS:{}:{}:{}:{}",
                    i + 1, points[0], points[1], points[2]);
        }
    }
};

void AddSC_spec_command()
{
    new spec_commandscript();
    new multispec_player_script();
}
