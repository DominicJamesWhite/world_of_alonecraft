#include "AlonecraftTestLog.h"
#include "ScriptMgr.h"
#include "Player.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "SpellScriptLoader.h"
#include "Unit.h"
#include "WarlockShards.h"

using namespace Alonecraft::Warlock;

// Shared "proc -> gain a Soul Shard" handler for three Warlock talents.
//
// All of the *conditions* (which spells, which proc phase, base chance,
// school) live in spell_proc, per the module's DBC-first philosophy.  This
// script only does the two things spell_proc cannot express: re-roll against
// the player's live spell crit chance, and grant the shard.
//
// Handled by DBC / spell_proc:
//   - Burning Soul  (18135/18136, was Intensity): Effect1 -> SPELL_AURA_DUMMY.
//       spell_proc HitMask = PROC_HIT_CRITICAL, SchoolMask = 4 (fire),
//       Chance = 15/30.  Nothing left for C++ but the shard itself.
//   - Aftermath     (18119/18120): Effect1 -> SPELL_AURA_DUMMY, Effect2 kept
//       (ADD_PCT_MODIFIER / SPELLMOD_DOT, the Immolate damage bonus).
//       spell_proc ProcFlags = PROC_FLAG_DONE_PERIODIC, Chance = 100 --
//       the real gate is the crit-chance roll below.
//   - Molten Rain   (18126/18127, was Demonic Power): Effect1 ->
//       SPELL_AURA_DUMMY.  spell_proc SpellFamilyMask0 = 32 (Rain of Fire),
//       Chance = 8/15, no ICD so it can fire per target per tick.
//
// Handled by this script:
//   - Aftermath's "chance equal to your critical strike chance".
//   - Capped shard granting via AddSoulShards().

enum WarlockShardGainSpells
{
    BURNING_SOUL_R1 = 18135,
    BURNING_SOUL_R2 = 18136,
    AFTERMATH_R1    = 18119,
    AFTERMATH_R2    = 18120,
    MOLTEN_RAIN_R1  = 18126,
    MOLTEN_RAIN_R2  = 18127,
};

// How the shard chance is decided for a given talent.
enum ShardGainMode
{
    // spell_proc.Chance already gated the proc -- just grant the shard.
    SHARD_GAIN_FIXED,
    // Re-roll against the player's live fire spell crit chance.
    SHARD_GAIN_SPELL_CRIT_CHANCE,
};

static ShardGainMode GetShardGainMode(uint32 spellId)
{
    switch (spellId)
    {
        case AFTERMATH_R1:
        case AFTERMATH_R2:
            return SHARD_GAIN_SPELL_CRIT_CHANCE;
        default:
            return SHARD_GAIN_FIXED;
    }
}

class spell_warl_shard_on_proc : public AuraScript
{
    PrepareAuraScript(spell_warl_shard_on_proc);

    bool CheckProc(ProcEventInfo& eventInfo)
    {
        return eventInfo.GetSpellInfo() != nullptr;
    }

    void HandleProc(AuraEffect const* /*aurEff*/, ProcEventInfo& /*eventInfo*/)
    {
        PreventDefaultAction();

        Unit* caster = GetTarget();
        if (!caster)
            return;

        Player* player = caster->ToPlayer();
        if (!player)
            return;

        // Cheap early-out so a capped warlock never rolls or chat-spams.
        if (GetSoulShardCount(player) >= SOUL_SHARD_MAX)
            return;

        if (GetShardGainMode(GetId()) == SHARD_GAIN_SPELL_CRIT_CHANCE)
        {
            float critChance = player->GetFloatValue(
                PLAYER_SPELL_CRIT_PERCENTAGE1 + static_cast<uint8>(SPELL_SCHOOL_FIRE));
            if (!roll_chance_f(critChance))
            {
                ACTEST("WARL.SHARDGAIN", "talent={} mode=critchance roll=FAIL fireCrit={:.2f}",
                    GetId(), critChance);
                return;
            }

            ACTEST("WARL.SHARDGAIN", "talent={} mode=critchance roll=PASS fireCrit={:.2f}",
                GetId(), critChance);
        }
        else
        {
            ACTEST("WARL.SHARDGAIN", "talent={} mode=fixed (spell_proc already gated)", GetId());
        }

        AddSoulShards(player, 1, "shardgain");
    }

    void Register() override
    {
        DoCheckProc += AuraCheckProcFn(spell_warl_shard_on_proc::CheckProc);
        OnEffectProc += AuraEffectProcFn(spell_warl_shard_on_proc::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
    }
};

class spell_warl_shard_on_proc_loader : public SpellScriptLoader
{
public:
    spell_warl_shard_on_proc_loader() : SpellScriptLoader("spell_warl_shard_on_proc") { }

    AuraScript* GetAuraScript() const override
    {
        return new spell_warl_shard_on_proc();
    }
};

void AddSC_warl_shard_on_proc()
{
    new spell_warl_shard_on_proc_loader();
}
