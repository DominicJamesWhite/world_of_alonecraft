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
#include "ThreatManager.h"
#include "Unit.h"

#include "PetTalentHelpers.h"

using namespace Alonecraft::Pets;

// ---------------------------------------------------------------------------
//  Alonecraft Marksmanship: everything that lives ON the pet
// ---------------------------------------------------------------------------
//
//  Both talents here describe something the PET does, and
//  Unit::ProcSkillsAndReactives (Unit.cpp:6789) only runs for the actor and the
//  victim of an event -- an event caused by the pet never reaches the hunter's
//  aura list.  So each needs its own aura on the pet, delivered by
//  spell_pet_auras and carrying its amount from the owner's talent rank.
//  PetTalentHelpers.h has the full explanation.
//
//  The hunter-side halves of this pass live in HunterMarksmanshipHunter.cpp.

enum HunterMarksmanshipPetSpells
{
    // Talent rank chains, low rank first -- OwnerTalentAmount walks backwards.
    TALENT_FOCUSED_AIM_R1        = 53620,
    TALENT_FOCUSED_AIM_R2        = 53621,
    TALENT_FOCUSED_AIM_R3        = 53622,

    TALENT_GO_FOR_THE_THROAT_R1  = 34950,
    TALENT_GO_FOR_THE_THROAT_R2  = 34954,

    // Go for the Throat's bleed payload -- woa_2026_08_12_02.sql.
    SPELL_GO_FOR_THE_THROAT_BLEED = 200761
};

namespace
{
    // Hunter's Mark, all five ranks: SpellFamilyName 9, SpellFamilyFlags 1024,
    // aura 127 on EFFECT_1.  The family-mask overload matches every rank in one
    // call, which is why the ids are not listed here.
    constexpr uint32 HUNTERS_MARK_FAMILY_FLAG = 1024;
}

// ---------------------------------------------------------------------------
//  Focused Aim -- extra pet threat on the hunter's marked target
// ---------------------------------------------------------------------------
//  Why this is code rather than data: ThreatManager::CalculateModifiedThreat
//  (ThreatManager.cpp:699-718) offers exactly two hooks -- spell_threat.pctMod,
//  which is per-SPELL, and _singleSchoolModifiers, which is per-GENERATOR.
//  Neither can be conditioned on a property of the victim, and a pet's white
//  swing carries no spellProto at all (Unit::DealDamage, Unit.cpp:1297).
//
//  The extra threat is added with spell = nullptr, so it goes through
//  CalculateModifiedThreat like any other threat the pet generates and picks up
//  the x2.00 from Beastmaster's Bond (200743).  That is deliberate: the bonus
//  should scale WITH the pet tuning rather than sit outside it.
//
//  SPELL_EFFECT_THREAT is not used for the same reason -- Spell::EffectThreat
//  (SpellEffects.cpp:3709) passes ignoreModifiers = true, which would desync the
//  bonus from the hit that earned it.
class spell_hun_focused_aim : public AuraScript
{
    PrepareAuraScript(spell_hun_focused_aim);

    void HandleProc(AuraEffect const* /*aurEff*/, ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();

        Unit* pet = GetUnitOwner();
        Unit* victim = eventInfo.GetProcTarget();
        if (!victim)
            return;

        Player* owner = GetPetOwner(pet);
        if (!owner)
            return;

        DamageInfo* damageInfo = eventInfo.GetDamageInfo();
        if (!damageInfo || !damageInfo->GetDamage())
            return;

        // The mark has to be the OWNER's -- a second hunter's Hunter's Mark on
        // the same target must not feed this pet.
        if (!victim->GetAuraEffect(SPELL_AURA_RANGED_ATTACK_POWER_ATTACKER_BONUS,
                                   SPELLFAMILY_HUNTER, HUNTERS_MARK_FAMILY_FLAG, 0, 0,
                                   owner->GetGUID()))
            return;

        int32 const pct = OwnerTalentAmount(pet,
            {TALENT_FOCUSED_AIM_R1, TALENT_FOCUSED_AIM_R2, TALENT_FOCUSED_AIM_R3},
            EFFECT_0);
        if (pct <= 0)
            return;

        float const extra = float(CalculatePct(int32(damageInfo->GetDamage()), pct));
        if (extra <= 0.0f)
            return;

        // `this` is the mob that owns the threat list; the parameter is the unit
        // generating the threat.  Reads backwards, but see Unit.cpp:11489.
        victim->AddThreat(pet, extra);

        ACTEST("HUN.MM.FOCUSEDAIM", "pet={} target={} damage={} pct={} extraThreat={:.0f}",
            Alonecraft::TestLog::N(pet), Alonecraft::TestLog::N(victim),
            damageInfo->GetDamage(), pct, extra);
    }

    void Register() override
    {
        OnEffectProc += AuraEffectProcFn(spell_hun_focused_aim::HandleProc,
                                         EFFECT_0, SPELL_AURA_DUMMY);
    }
};

// ---------------------------------------------------------------------------
//  Go for the Throat -- while frenzied, every pet melee hit rends
// ---------------------------------------------------------------------------
//  This aura is NOT permanent and is not delivered by spell_pet_auras.  The
//  hunter's ranged crit applies 200760 to the pet for 10s through the talent's
//  own Effect2 proc trigger, and this script runs off that temporary aura.
//
//  So there are two proc layers, and the crit requirement belongs to exactly
//  one of them: core's spell_proc -34950 (HitMask 2) gates the HUNTER's crit,
//  while 200760's own row has HitMask 0 so every pet melee hit inside the
//  window counts.  Requiring a crit in both places was the earlier design and
//  made the talent far rarer than it reads.
//
//  The rank amount still sits on EFFECT_1 of the talent.  Effect2 is now a
//  PROC_TRIGGER_SPELL rather than a dummy, but a proc-trigger effect still
//  carries an amount, so OwnerTalentAmount reads 25/50 from it unchanged.
//
//  CastDelayedSpellWithPeriodicAmount (Unit.cpp:16312) is the Deep Wounds
//  pattern: it rolls whatever is left of an existing same-caster bleed into the
//  new amount rather than clipping it, and casts with SPELLVALUE_BASE_POINT0.
//  The amount handed over must already be per-tick.
class spell_hun_go_for_the_throat_bleed : public AuraScript
{
    PrepareAuraScript(spell_hun_go_for_the_throat_bleed);

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_GO_FOR_THE_THROAT_BLEED });
    }

    void HandleProc(AuraEffect const* /*aurEff*/, ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();

        Unit* pet = GetUnitOwner();
        Unit* victim = eventInfo.GetProcTarget();
        if (!victim)
            return;

        DamageInfo* damageInfo = eventInfo.GetDamageInfo();
        if (!damageInfo || !damageInfo->GetDamage())
            return;

        // Never echo our own payload.  The bleed carries
        // SPELL_ATTR3_SUPPRESS_CASTER_PROCS as well, so this is belt and braces.
        if (SpellInfo const* spellInfo = eventInfo.GetSpellInfo())
            if (spellInfo->Id == SPELL_GO_FOR_THE_THROAT_BLEED)
                return;

        int32 const pct = OwnerTalentAmount(pet,
            {TALENT_GO_FOR_THE_THROAT_R1, TALENT_GO_FOR_THE_THROAT_R2},
            EFFECT_1);
        if (pct <= 0)
            return;

        SpellInfo const* bleed = sSpellMgr->AssertSpellInfo(SPELL_GO_FOR_THE_THROAT_BLEED);
        ASSERT(bleed->GetMaxTicks() > 0);

        // GetDamage() is post-mitigation, which is what "of the damage done"
        // means.  It is also why 200761 carries IGNORE_ARMOR -- the parent hit
        // was already taxed once.
        int32 amount = CalculatePct(int32(damageInfo->GetDamage()), pct) / int32(bleed->GetMaxTicks());
        if (amount <= 0)
            return;

        victim->CastDelayedSpellWithPeriodicAmount(pet, SPELL_GO_FOR_THE_THROAT_BLEED,
                                                   SPELL_AURA_PERIODIC_DAMAGE, amount);

        ACTEST("HUN.MM.THROAT", "pet={} target={} hitDamage={} pct={} perTick={}",
            Alonecraft::TestLog::N(pet), Alonecraft::TestLog::N(victim),
            damageInfo->GetDamage(), pct, amount);
    }

    void Register() override
    {
        OnEffectProc += AuraEffectProcFn(spell_hun_go_for_the_throat_bleed::HandleProc,
                                         EFFECT_0, SPELL_AURA_DUMMY);
    }
};

void AddSC_hunter_mm_pet()
{
    RegisterSpellScript(spell_hun_focused_aim);
    RegisterSpellScript(spell_hun_go_for_the_throat_bleed);
}
