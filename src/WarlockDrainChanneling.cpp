#include "AlonecraftTestLog.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SpellAuraEffects.h"
#include "SpellAuras.h"
#include "SpellScript.h"
#include "SpellScriptLoader.h"
#include "Unit.h"
#include "WarlockShards.h"

using namespace Alonecraft::Warlock;

// Two Affliction talents that pay out while the warlock is standing in a
// drain channel.  Both hang off the channel aura itself rather than off the
// talent, because "while channeling" has no DBC representation.
//
//   Improved Drain Soul (18213 / 18372) -- woa_2026_08_06_08.sql (DBC),
//                                          woa_2026_08_06_09.sql (script)
//     The threat reduction is gone; the talent now raises Drain Soul's Soul
//     Shard generation rate by 50% / 100%.
//
//   Fel Concentration (17783 / 17784 / 17785) -- woa_2026_08_06_06.sql
//     Was pure pushback resistance, which Alonecraft removed entirely.  Now
//     reduces damage taken by 10% / 20% / 30% while channeling Drain Soul or
//     Drain Life.
//
// Handled by DBC:
//   Both talents' effects are bare SPELL_AURA_DUMMY rank carriers.  Improved
//   Drain Soul's effect 2 holds the percentage this script reads; Fel
//   Concentration's ranks are distinguished by spell id alone and its three
//   damage-reduction values live in the buff spells 200523-200525.
//
// Handled here:
//   The channel-lifetime bookkeeping, and the extra shard roll.

enum WarlockDrainChannelingSpells
{
    IMPROVED_DRAIN_SOUL_R1 = 18213,

    FEL_CONCENTRATION_R1   = 17783,
    FEL_CONCENTRATION_R2   = 17784,
    FEL_CONCENTRATION_R3   = 17785,

    FEL_CONCENTRATION_BUFF_R1 = 200523,
    FEL_CONCENTRATION_BUFF_R2 = 200524,
    FEL_CONCENTRATION_BUFF_R3 = 200525,
};

// -------------------------------------------------------------------------
//  Improved Drain Soul -- faster Soul Shard generation
// -------------------------------------------------------------------------
//
// Core rolls a flat 20% per Drain Soul tick to create a shard
// (spell_warl_drain_soul::HandleTick, spell_warlock.cpp:1240).  That core
// function is deliberately left alone -- this is an *independent* extra roll
// on the same tick, which is the same thing in expectation and keeps the
// change module-only.  If core's rate ever changes, this constant has to
// follow it; nothing else links the two.
constexpr uint32 DRAIN_SOUL_BASE_SHARD_CHANCE = 20;

class spell_warl_improved_drain_soul_shards_AuraScript : public AuraScript
{
    PrepareAuraScript(spell_warl_improved_drain_soul_shards_AuraScript);

    void HandleTick(AuraEffect const* /*aurEff*/)
    {
        Unit* caster = GetCaster();
        Unit* target = GetTarget();
        if (!caster || !target)
            return;

        Player* player = caster->ToPlayer();
        if (!player)
            return;

        // Same gate core uses -- otherwise grey mobs become a shard farm.
        if (!player->isHonorOrXPTarget(target))
            return;

        Aura const* talent = player->GetAuraOfRankedSpell(IMPROVED_DRAIN_SOUL_R1, player->GetGUID());
        if (!talent)
            return;

        // 50 at rank 1, 100 at rank 2.
        int32 const bonusPct = talent->GetSpellInfo()->Effects[EFFECT_1].CalcValue();
        if (bonusPct <= 0)
            return;

        // Cheap early-out so a capped warlock never rolls or chat-spams.
        if (GetSoulShardCount(player) >= SOUL_SHARD_MAX)
            return;

        uint32 const chance = DRAIN_SOUL_BASE_SHARD_CHANCE * uint32(bonusPct) / 100;
        if (!roll_chance_i(int32(chance)))
        {
            ACTEST("WARL.SHARDGAIN", "improved-drain-soul talent={} roll=FAIL chance={} bonusPct={}",
                talent->GetId(), chance, bonusPct);
            return;
        }

        ACTEST("WARL.SHARDGAIN", "improved-drain-soul talent={} roll=PASS chance={} bonusPct={}",
            talent->GetId(), chance, bonusPct);

        AddSoulShards(player, 1, "improved-drain-soul");
    }

    void Register() override
    {
        OnEffectPeriodic += AuraEffectPeriodicFn(spell_warl_improved_drain_soul_shards_AuraScript::HandleTick,
            EFFECT_1, SPELL_AURA_PERIODIC_DAMAGE);
    }
};

class spell_warl_improved_drain_soul_shards : public SpellScriptLoader
{
public:
    spell_warl_improved_drain_soul_shards() : SpellScriptLoader("spell_warl_improved_drain_soul_shards") { }

    AuraScript* GetAuraScript() const override
    {
        return new spell_warl_improved_drain_soul_shards_AuraScript();
    }
};

// -------------------------------------------------------------------------
//  Fel Concentration -- damage reduction while channeling
// -------------------------------------------------------------------------
//
// Registered on both -1120 (Drain Soul) and -689 (Drain Life).  The drain
// aura lives on the *target*, so channel interrupt, target death and range
// break all remove it -- and therefore drop the buff -- with no timer here.
//
// EFFECT_0 differs between the two spells (CHANNEL_DEATH_ITEM on Drain Soul,
// PERIODIC_LEECH on Drain Life), hence SPELL_AURA_ANY.

static uint32 GetFelConcentrationBuff(Unit* caster)
{
    if (!caster)
        return 0;

    if (Aura const* talent = caster->GetAuraOfRankedSpell(FEL_CONCENTRATION_R1, caster->GetGUID()))
    {
        switch (talent->GetId())
        {
            case FEL_CONCENTRATION_R1: return FEL_CONCENTRATION_BUFF_R1;
            case FEL_CONCENTRATION_R2: return FEL_CONCENTRATION_BUFF_R2;
            case FEL_CONCENTRATION_R3: return FEL_CONCENTRATION_BUFF_R3;
            default: break;
        }
    }

    return 0;
}

class spell_warl_fel_concentration_AuraScript : public AuraScript
{
    PrepareAuraScript(spell_warl_fel_concentration_AuraScript);

    void OnApply(AuraEffect const* aurEff, AuraEffectHandleModes /*mode*/)
    {
        Unit* caster = GetCaster();
        uint32 const buff = GetFelConcentrationBuff(caster);
        if (!buff)
            return;

        caster->CastSpell(caster, buff, true, nullptr, aurEff);

        ACTEST("WARL.FELCONC", "channel={} applied buff={} to={}",
            GetId(), buff, caster->GetName());
    }

    void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        // Deliberately not gated on the talent: a respec mid-channel must
        // still be able to clean up the buff it granted a moment ago.
        Unit* caster = GetCaster();
        if (!caster)
            return;

        caster->RemoveAurasDueToSpell(FEL_CONCENTRATION_BUFF_R1);
        caster->RemoveAurasDueToSpell(FEL_CONCENTRATION_BUFF_R2);
        caster->RemoveAurasDueToSpell(FEL_CONCENTRATION_BUFF_R3);

        ACTEST("WARL.FELCONC", "channel={} ended, buff cleared from={}",
            GetId(), caster->GetName());
    }

    void Register() override
    {
        AfterEffectApply += AuraEffectApplyFn(spell_warl_fel_concentration_AuraScript::OnApply,
            EFFECT_0, SPELL_AURA_ANY, AURA_EFFECT_HANDLE_REAL);
        AfterEffectRemove += AuraEffectRemoveFn(spell_warl_fel_concentration_AuraScript::OnRemove,
            EFFECT_0, SPELL_AURA_ANY, AURA_EFFECT_HANDLE_REAL);
    }
};

class spell_warl_fel_concentration : public SpellScriptLoader
{
public:
    spell_warl_fel_concentration() : SpellScriptLoader("spell_warl_fel_concentration") { }

    AuraScript* GetAuraScript() const override
    {
        return new spell_warl_fel_concentration_AuraScript();
    }
};

void AddSC_warl_drain_channeling()
{
    new spell_warl_improved_drain_soul_shards();
    new spell_warl_fel_concentration();
}
