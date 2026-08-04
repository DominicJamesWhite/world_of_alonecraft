/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license:
 * https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE-AGPL3
 */

#include "AlonecraftTestLog.h"
#include "Chat.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SpellAuraEffects.h"
#include "SpellScript.h"
#include "SpellScriptLoader.h"
#include "Unit.h"
#include "WarlockShards.h"

using namespace Alonecraft::Warlock;

// ---------------------------------------------------------------------------
//  Alonecraft: Metamorphosis becomes a shard-fuelled stance
// ---------------------------------------------------------------------------
//
//  No cooldown, no fixed duration.  Entering costs one Soul Shard and the form
//  then burns one more every 6 seconds, dropping automatically when the
//  warlock runs dry.
//
//  The DBC side (woa_2026_08_02_07.sql) does most of the work:
//      RecoveryTime  180000 -> 0        no cooldown
//      DurationIndex      9 -> 21       30s -> permanent
//      Effect2  MOD_BASE_RESISTANCE_PCT 600% -> PERIODIC_DUMMY, amplitude 6000
//  That one effect swap both removes the defensive bonus (it now belongs to
//  Demonic Aegis) and creates the upkeep heartbeat, so no extra custom spell
//  id was needed.
//
//  Effect1 (MOD_SHAPESHIFT, form 22) and Effect3 (+20% damage done) are
//  untouched, as is the in-form ability kit: 50589 Immolation Aura, 54785
//  Demon Charge, 59671 Challenging Howl and 50581 Shadow Cleave.
//
//  The form remains cancellable -- 47241's Attributes are 16
//  (SPELL_ATTR0_IS_ABILITY), NOT 0x80000000 SPELL_ATTR0_NO_AURA_CANCEL, so
//  right-clicking the buff off works.
// ---------------------------------------------------------------------------

enum WarlockMetamorphosisSpells
{
    SPELL_METAMORPHOSIS = 47241,
};

// One shard to enter, one more every 6 seconds.
static constexpr uint32 METAMORPHOSIS_SHARD_COST = 1;

class spell_warl_metamorphosis : public SpellScript
{
    PrepareSpellScript(spell_warl_metamorphosis);

    SpellCastResult CheckCast()
    {
        Player* player = GetCaster() ? GetCaster()->ToPlayer() : nullptr;
        if (!player)
            return SPELL_FAILED_DONT_REPORT;

        uint32 const shards = GetSoulShardCount(player);
        ACTEST("WARL.META", "cast attempt shards={} need={} result={}", shards,
            METAMORPHOSIS_SHARD_COST,
            shards < METAMORPHOSIS_SHARD_COST ? "FAILED_REAGENTS" : "OK");

        if (shards < METAMORPHOSIS_SHARD_COST)
            return SPELL_FAILED_REAGENTS;

        return SPELL_CAST_OK;
    }

    void HandleAfterCast()
    {
        if (Player* player = GetCaster() ? GetCaster()->ToPlayer() : nullptr)
        {
            ConsumeSoulShards(player, METAMORPHOSIS_SHARD_COST, "meta-enter");
            ACTEST("WARL.META", "entered form shardsLeft={}", GetSoulShardCount(player));
        }
    }

    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_warl_metamorphosis::CheckCast);
        AfterCast   += SpellCastFn(spell_warl_metamorphosis::HandleAfterCast);
    }
};

class spell_warl_metamorphosis_upkeep : public AuraScript
{
    PrepareAuraScript(spell_warl_metamorphosis_upkeep);

    void HandlePeriodic(AuraEffect const* /*aurEff*/)
    {
        PreventDefaultAction();

        Player* player = GetTarget() ? GetTarget()->ToPlayer() : nullptr;
        if (!player)
        {
            Remove();
            return;
        }

        if (!ConsumeSoulShards(player, METAMORPHOSIS_SHARD_COST, "meta-upkeep"))
        {
            ACTEST("WARL.META", "upkeep tick FAILED (no shards) -- form dropping");
            ChatHandler(player->GetSession()).PSendSysMessage(
                "Your Metamorphosis fades as the last of your Soul Shards is spent.");
            Remove();
            return;
        }

        ACTEST("WARL.META", "upkeep tick paid shardsLeft={}", GetSoulShardCount(player));

        // Courtesy warning on the tick that leaves the warlock empty, so the
        // form never drops without notice mid-pull.
        if (GetSoulShardCount(player) == 0)
            ChatHandler(player->GetSession()).PSendSysMessage(
                "You have no Soul Shards left - Metamorphosis will fade shortly.");
    }

    void Register() override
    {
        OnEffectPeriodic += AuraEffectPeriodicFn(spell_warl_metamorphosis_upkeep::HandlePeriodic, EFFECT_1, SPELL_AURA_PERIODIC_DUMMY);
    }
};

class spell_warl_metamorphosis_loader : public SpellScriptLoader
{
public:
    spell_warl_metamorphosis_loader() : SpellScriptLoader("spell_warl_metamorphosis") { }

    SpellScript* GetSpellScript() const override
    {
        return new spell_warl_metamorphosis();
    }

    AuraScript* GetAuraScript() const override
    {
        return new spell_warl_metamorphosis_upkeep();
    }
};

void AddSC_warl_metamorphosis()
{
    new spell_warl_metamorphosis_loader();
}
