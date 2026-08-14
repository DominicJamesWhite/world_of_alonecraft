/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license:
 * https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE-AGPL3
 */

#include "AlonecraftTestLog.h"
#include "Config.h"
#include "Pet.h"
#include "PetDefines.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SpellAuraEffects.h"
#include "SpellScript.h"
#include "SpellScriptLoader.h"
#include "Unit.h"

// ---------------------------------------------------------------------------
//  Hunter pets: less damage, much more threat
// ---------------------------------------------------------------------------
//
//  TODO.md: "Pet by default does 20% LESS DAMAGE but 60% MORE THREAT from melee
//  and special attacks."  The point is a pet that can hold a pull for a solo
//  hunter, paid for out of its own damage.
//
//  Everything mechanical is in the DBC (200743, woa_2026_08_11_07.sql), which
//  also carries the long-form reasoning for the two numbers.  The short version:
//  threat is 1:1 with post-mitigation damage (Unit.cpp:1297), so the -20% cuts
//  threat too, and reaching "60% more than before" needs a x2.00 threat
//  multiplier rather than a x1.60 one.  0.80 x 2.00 = 1.60.
//
//  This file is only delivery and tuning:
//
//    * spell_pet_auras cannot be used.  It is driven by a SPELL_AURA_DUMMY
//      effect on an owner aura (AuraEffect::HandleAuraDummy ->
//      Unit::AddPetAura, Unit.cpp:13753), and a hunter has no always-on aura to
//      hang one off.  PetScript::OnPetAddToWorld (Pet.cpp:118) covers every way
//      a pet appears -- summon from stable, login, and a fresh tame, which is
//      the case Pet::SynchronizeLevelWithOwner misses too.  Same hook
//      HunterPetLevelSync.cpp uses, for the same reason.
//
//    * the amounts come from config rather than the DBC row, so the trade can
//      be retuned with a server restart instead of a DBC rebuild and an MPQ
//      copy.  AuraEffect::HandleModDamagePercentDone (SpellAuraEffects.cpp:5005)
//      fires on AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK and calls
//      UpdateAllDamagePctDoneMods, so a scripted amount reaches
//      UNIT_MOD_DAMAGE_MAINHAND without any extra work here.
//
//  No heartbeat: neither number depends on owner state, gear or talents.

enum HunterPetTuningSpells
{
    SPELL_HUNTER_BEASTMASTERS_BOND = 200743
};

namespace
{
    bool TuningEnabled()
    {
        return sConfigMgr->GetOption<bool>("Alonecraft.HunterPet.Enable", true);
    }

    int32 TuningDamagePct()
    {
        return sConfigMgr->GetOption<int32>("Alonecraft.HunterPet.DamagePct", -20);
    }

    int32 TuningThreatPct()
    {
        return sConfigMgr->GetOption<int32>("Alonecraft.HunterPet.ThreatPct", 100);
    }
}

class woa_hunter_pet_tuning_pet : public PetScript
{
public:
    woa_hunter_pet_tuning_pet()
        : PetScript("woa_hunter_pet_tuning_pet", { PETHOOK_ON_PET_ADD_TO_WORLD }) { }

    void OnPetAddToWorld(Pet* pet) override
    {
        if (!pet || pet->getPetType() != HUNTER_PET)
            return;

        bool const hasAura = pet->HasAura(SPELL_HUNTER_BEASTMASTERS_BOND);

        // Flipping the config off and re-summoning is enough to undo this.
        if (!TuningEnabled())
        {
            if (hasAura)
                pet->RemoveAurasDueToSpell(SPELL_HUNTER_BEASTMASTERS_BOND);

            return;
        }

        if (!hasAura)
            pet->CastSpell(pet, SPELL_HUNTER_BEASTMASTERS_BOND, true);
    }
};

class spell_hun_pet_tuning : public AuraScript
{
    PrepareAuraScript(spell_hun_pet_tuning);

    void CalculateDamage(AuraEffect const* /*aurEff*/, int32& amount, bool& /*canBeRecalculated*/)
    {
        amount = TuningDamagePct();

        ACTEST("HUN.PET.TUNING", "pet={} damagePct={}",
            Alonecraft::TestLog::N(GetUnitOwner()), amount);
    }

    void CalculateThreat(AuraEffect const* /*aurEff*/, int32& amount, bool& /*canBeRecalculated*/)
    {
        amount = TuningThreatPct();

        // The number worth seeing is the product, not either factor.
        ACTEST("HUN.PET.TUNING", "pet={} threatPct={} netThreatVsBaseline={:.2f}",
            Alonecraft::TestLog::N(GetUnitOwner()), amount,
            (1.0f + TuningDamagePct() / 100.0f) * (1.0f + amount / 100.0f));
    }

    void Register() override
    {
        DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_hun_pet_tuning::CalculateDamage, EFFECT_0, SPELL_AURA_MOD_DAMAGE_PERCENT_DONE);
        DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_hun_pet_tuning::CalculateThreat, EFFECT_1, SPELL_AURA_MOD_THREAT);
    }
};

void AddSC_hunter_pet_tuning()
{
    new woa_hunter_pet_tuning_pet();
    RegisterSpellScript(spell_hun_pet_tuning);
}
