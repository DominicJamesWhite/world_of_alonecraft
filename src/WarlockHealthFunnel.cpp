/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license:
 * https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE-AGPL3
 */

#include "AlonecraftTestLog.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SpellAuraEffects.h"
#include "SpellMgr.h"
#include "SpellScript.h"
#include "SpellScriptLoader.h"
#include "Unit.h"

// ---------------------------------------------------------------------------
//  Alonecraft: Mana Feed's Health Funnel half (Demonology item 11b)
// ---------------------------------------------------------------------------
//
//  Health Funnel additionally restores 5% of the demon's maximum mana per tick.
//  The percentage lives in talent 30326 EFFECT_1, a SPELL_AURA_DUMMY holder
//  (woa_2026_08_02_03.sql).  Mana Feed's other half -- the demon's damage
//  returning mana to the owner -- is 30326 EFFECT_0 and is implemented
//  separately as spell_warl_demon_mana_feed in WarlockDemonPets.cpp.
//
//  This script is registered ALONGSIDE core's spell_warl_health_funnel rather
//  than replacing it -- `spell_script_names` allows several rows per spell, so
//  no core file is touched.
//
//  Sacrifice of Blood (item 5, talents 18703/18704) used to live here too.  It
//  no longer needs any C++: core's spell_warl_health_funnel already casts
//  60955/60956 on the demon for the duration of the channel, gated on the
//  talent, so the whole redesign is a DBC rewrite of those two buffs.  See
//  woa_2026_08_04_07.sql.
// ---------------------------------------------------------------------------

enum WarlockHealthFunnelSpells
{
    TALENT_MANA_FEED = 30326,
};

class spell_warl_health_funnel_mana_feed : public AuraScript
{
    PrepareAuraScript(spell_warl_health_funnel_mana_feed);

    void HandlePeriodic(AuraEffect const* /*aurEff*/)
    {
        Unit* caster = GetCaster();
        Unit* demon  = GetTarget();
        if (!caster || !demon)
            return;

        // Single-rank talent, so no rank walk is needed.
        AuraEffect const* manaFeed = caster->GetAuraEffect(TALENT_MANA_FEED, EFFECT_1);
        if (!manaFeed)
            return;

        uint32 const maxMana = demon->GetMaxPower(POWER_MANA);
        if (!maxMana)
            return;

        int32 const mana = int32(CalculatePct(maxMana, manaFeed->GetAmount()));
        if (mana <= 0)
            return;

        uint32 const before = demon->GetPower(POWER_MANA);
        demon->ModifyPower(POWER_MANA, mana);

        ACTEST("WARL.MANAFEED",
            "Mana Feed tick demon={} pct={} mana={} demonMana {} -> {} (max={})",
            Alonecraft::TestLog::N(demon), manaFeed->GetAmount(), mana,
            before, demon->GetPower(POWER_MANA), maxMana);
    }

    void Register() override
    {
        OnEffectPeriodic += AuraEffectPeriodicFn(spell_warl_health_funnel_mana_feed::HandlePeriodic, EFFECT_0, SPELL_AURA_PERIODIC_HEAL);
    }
};

class spell_warl_health_funnel_mana_feed_loader : public SpellScriptLoader
{
public:
    spell_warl_health_funnel_mana_feed_loader() : SpellScriptLoader("spell_warl_health_funnel_mana_feed") { }

    AuraScript* GetAuraScript() const override
    {
        return new spell_warl_health_funnel_mana_feed();
    }
};

void AddSC_warl_health_funnel_mana_feed()
{
    new spell_warl_health_funnel_mana_feed_loader();
}
