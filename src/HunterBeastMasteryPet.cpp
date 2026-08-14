/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license:
 * https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE-AGPL3
 */

#include "AlonecraftTestLog.h"
#include "DBCStores.h"
#include "DBCStructure.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SpellAuraEffects.h"
#include "SpellMgr.h"
#include "SpellScript.h"
#include "SpellScriptLoader.h"
#include "Unit.h"

#include "PetTalentHelpers.h"

#include <limits>

using namespace Alonecraft::Pets;

// ---------------------------------------------------------------------------
//  Alonecraft Beast Mastery: everything that lives ON the pet
// ---------------------------------------------------------------------------
//
//  These four talents all describe something the PET does, and
//  Unit::ProcSkillsAndReactives (Unit.cpp:6789) only runs for the actor and the
//  victim of an event -- an event caused by the pet never reaches the hunter's
//  aura list.  So each one needs its own aura on the pet, delivered by
//  spell_pet_auras and carrying an amount read back off the owner's talent rank.
//  PetTalentHelpers.h has the full explanation.
//
//  The hunter-side halves of this pass live in HunterBeastMasteryHunter.cpp.

enum HunterBeastMasteryPetSpells
{
    // Talent rank chains, low rank first -- OwnerTalentAmount walks backwards.
    TALENT_TASTE_FOR_BLOOD_R1       = 19549,
    TALENT_TASTE_FOR_BLOOD_R2       = 19550,
    TALENT_TASTE_FOR_BLOOD_R3       = 19551,

    TALENT_INVIGORATION_R1          = 53252,
    TALENT_INVIGORATION_R2          = 53253,

    // Lacerating Shot (repurposed Scare Beast) -- woa_2026_08_11_02.sql.
    SPELL_LACERATING_SHOT_R1        = 1513,
    SPELL_LACERATING_SHOT_R2        = 14326,
    SPELL_LACERATING_SHOT_R3        = 14327,

    // Pack Hunting (repurposed Freezing Arrow) -- woa_2026_08_11_04.sql.
    SPELL_PACK_HUNTING              = 60192,

    // Instinctive Fire (repurposed Beast Lore) -- woa_2026_08_11_03.sql.
    SPELL_INSTINCTIVE_FIRE          = 1462,

    // Alonecraft custom spells.
    SPELL_TASTE_FOR_BLOOD_DAMAGE    = 200746,
    SPELL_MANA_RETURN               = 200744,
    SPELL_BITE_BACK                 = 200752,
    SPELL_WELL_TRAINED              = 200753,
    SPELL_INSTINCTIVE_FOCUS         = 200757,
};

namespace
{
    // Lacerating Shot's SpellFamilyFlags are all zero after the rework, so a
    // family-mask match is impossible and the ranks have to be named.
    constexpr uint32 LACERATING_SHOT_RANKS[] =
    {
        SPELL_LACERATING_SHOT_R1,
        SPELL_LACERATING_SHOT_R2,
        SPELL_LACERATING_SHOT_R3,
    };

    // Well-Trained is granted after this long without the pet being attacked.
    constexpr uint32 WELL_TRAINED_IDLE_MS = 8 * IN_MILLISECONDS;
}

// ---------------------------------------------------------------------------
//  Taste for Blood -- pet damage against a target the hunter has made bleed
// ---------------------------------------------------------------------------
//  The bleed has to be the OWNER's.  GetAuraEffect with a caster GUID is what
//  makes that true: a second hunter's Lacerating Shot on the same target must
//  not feed this pet, or the talent silently doubles in a group.
class spell_hun_taste_for_blood : public AuraScript
{
    PrepareAuraScript(spell_hun_taste_for_blood);

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_TASTE_FOR_BLOOD_DAMAGE });
    }

    bool CheckProc(ProcEventInfo& eventInfo)
    {
        Unit* victim = eventInfo.GetProcTarget();
        if (!victim)
            return false;

        Player* owner = GetPetOwner(GetUnitOwner());
        if (!owner)
            return false;

        // Never echo our own payload.
        if (SpellInfo const* spellInfo = eventInfo.GetSpellInfo())
            if (spellInfo->Id == SPELL_TASTE_FOR_BLOOD_DAMAGE)
                return false;

        for (uint32 rank : LACERATING_SHOT_RANKS)
            if (victim->GetAuraEffect(rank, EFFECT_0, owner->GetGUID()))
                return true;

        return false;
    }

    void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();

        Unit* pet = GetUnitOwner();
        Unit* victim = eventInfo.GetProcTarget();

        Player* owner = GetPetOwner(pet);
        if (!owner || !victim)
            return;

        int32 const pct = OwnerTalentAmount(pet,
            {TALENT_TASTE_FOR_BLOOD_R1, TALENT_TASTE_FOR_BLOOD_R2, TALENT_TASTE_FOR_BLOOD_R3},
            EFFECT_0);
        if (pct <= 0)
            return;

        // The owner's ranged attack power, not the pet's -- that is the whole
        // design, and it is why this cannot be a DBC coefficient.
        int32 damage = CalculatePct(int32(owner->GetTotalAttackPowerValue(RANGED_ATTACK)), pct);
        if (damage <= 0)
            return;

        pet->CastCustomSpell(victim, SPELL_TASTE_FOR_BLOOD_DAMAGE,
                             &damage, nullptr, nullptr, true, nullptr, aurEff);

        ACTEST("HUN.PET.TASTEFORBLOOD", "pet={} target={} ownerRAP={:.0f} pct={} damage={}",
            Alonecraft::TestLog::N(pet), Alonecraft::TestLog::N(victim),
            owner->GetTotalAttackPowerValue(RANGED_ATTACK), pct, damage);
    }

    void Register() override
    {
        DoCheckProc  += AuraCheckProcFn(spell_hun_taste_for_blood::CheckProc);
        OnEffectProc += AuraEffectProcFn(spell_hun_taste_for_blood::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
    }
};

// ---------------------------------------------------------------------------
//  Thick Hide -- pet dodge from the owner's Agility
// ---------------------------------------------------------------------------
//  The TODO points at "the demo lock talent", which is spell_warl_demon_dodge
//  (WarlockDemonPets.cpp:161).  This is that script with three substitutions:
//  STAT_AGILITY rather than STAT_INTELLECT, and the hunter's own row and
//  coefficient rather than the druid's.
//
//  Player::GetDodgeFromAgility (Player.cpp:5126) cannot be called directly --
//  it reads the *owner's* class off the player it is a method on, and both
//  dodge_base[] and crit_to_dodge[] are file-local arrays there with no
//  accessor.  Only the one constant is mirrored; the DBC ratio table is public.
class spell_hun_thick_hide : public AuraScript
{
    PrepareAuraScript(spell_hun_thick_hide);

    void CalculateAmount(AuraEffect const* /*aurEff*/, int32& amount, bool& /*canBeRecalculated*/)
    {
        amount = 0;

        Player* owner = GetPetOwner(GetUnitOwner());
        if (!owner)
            return;

        uint8 level = owner->GetLevel();
        if (level > GT_MAX_LEVEL)
            level = GT_MAX_LEVEL;

        // Dodge per point of a stat is proportional to crit per point of that
        // stat, which is what this table stores -- the same derivation
        // Player::GetDodgeFromAgility uses.
        GtChanceToMeleeCritEntry const* ratio =
            sGtChanceToMeleeCritStore.LookupEntry((CLASS_HUNTER - 1) * GT_MAX_LEVEL + level - 1);
        if (!ratio)
            return;

        // Mirrors Player.cpp:5158 crit_to_dodge[CLASS_HUNTER - 1].  Keep in sync
        // if that table is ever retuned.
        float const critToDodgeHunter = 1.11f / 1.15f;

        float const agility = owner->GetStat(STAT_AGILITY);
        amount = int32(100.0f * agility * ratio->ratio * critToDodgeHunter);

        // Recalculated every 2 sec; only a real move is worth a line.
        if (amount != _lastLogged)
        {
            _lastLogged = amount;
            ACTEST("HUN.PET.THICKHIDE", "pet={} ownerAgi={:.1f} ratio={:.6f} -> dodgeAmount={}",
                Alonecraft::TestLog::N(GetUnitOwner()), agility, ratio->ratio, amount);
        }
    }

    int32 _lastLogged = std::numeric_limits<int32>::lowest();

    ALONECRAFT_PET_HEARTBEAT(spell_hun_thick_hide)

    void Register() override
    {
        DoEffectCalcAmount   += AuraEffectCalcAmountFn(spell_hun_thick_hide::CalculateAmount, EFFECT_0, SPELL_AURA_MOD_DODGE_PERCENT);
        DoEffectCalcPeriodic += AuraEffectCalcPeriodicFn(spell_hun_thick_hide::CalcPeriodic, EFFECT_0, SPELL_AURA_MOD_DODGE_PERCENT);
        OnEffectPeriodic     += AuraEffectPeriodicFn(spell_hun_thick_hide::HandlePeriodic, EFFECT_0, SPELL_AURA_MOD_DODGE_PERCENT);
    }
};

// ---------------------------------------------------------------------------
//  Superior Training -- bite back when attacked, grow teeth when left alone
// ---------------------------------------------------------------------------
//  Two behaviours on one aura effect, because they are two halves of one talent
//  and share a single piece of state: when the pet was last hit.
//
//  The idle timer rides the proc handler rather than UnitScript::OnDamage.  The
//  proc already fires on every incoming hit that matters, so the timestamp is
//  free here, while an OnDamage hook would run for every unit on every map to
//  serve one talent.
//
//  The 1 Hz tick uses the DoEffectCalcPeriodic trick (see
//  ALONECRAFT_PET_HEARTBEAT's comment) rather than a PERIODIC_DUMMY effect in
//  the DBC, which would cost an effect slot the carrier does not have to spare.
class spell_hun_superior_training : public AuraScript
{
    PrepareAuraScript(spell_hun_superior_training);

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_BITE_BACK, SPELL_WELL_TRAINED });
    }

    bool CheckProc(ProcEventInfo& eventInfo)
    {
        DamageInfo* damageInfo = eventInfo.GetDamageInfo();
        if (!damageInfo || !damageInfo->GetDamage())
            return false;

        // Our own retaliation must not count as being attacked.
        if (SpellInfo const* spellInfo = eventInfo.GetSpellInfo())
            if (spellInfo->Id == SPELL_BITE_BACK)
                return false;

        return GetPetOwner(GetUnitOwner()) != nullptr;
    }

    void HandleProc(AuraEffect const* aurEff, ProcEventInfo& /*eventInfo*/)
    {
        PreventDefaultAction();

        Unit* pet = GetUnitOwner();
        if (!pet)
            return;

        _idleMs = 0;

        // Being attacked is what Well-Trained is the absence of.
        pet->RemoveAurasDueToSpell(SPELL_WELL_TRAINED);

        // One ordinary swing's worth.  CalculateDamage reads
        // UNIT_FIELD_MIN/MAXDAMAGE, which Guardian::UpdateDamagePhysical already
        // wrote with the global -20% folded in, so the retaliation is scaled
        // with the rest of the pet's damage rather than around it.
        int32 damage = int32(pet->CalculateDamage(BASE_ATTACK, true, true));
        if (damage <= 0)
            return;

        pet->CastCustomSpell(pet, SPELL_BITE_BACK, &damage, nullptr, nullptr, true, nullptr, aurEff);

        ACTEST("HUN.PET.SUPERIORTRAINING", "pet={} biteBack damage={}",
            Alonecraft::TestLog::N(pet), damage);
    }

    void CalcPeriodic(AuraEffect const* /*aurEff*/, bool& isPeriodic, int32& amplitude)
    {
        isPeriodic = true;
        amplitude  = 1 * IN_MILLISECONDS;
    }

    void HandlePeriodic(AuraEffect const* /*aurEff*/)
    {
        PreventDefaultAction();

        Unit* pet = GetUnitOwner();
        if (!pet)
            return;

        _idleMs += 1 * IN_MILLISECONDS;

        if (_idleMs < WELL_TRAINED_IDLE_MS || pet->HasAura(SPELL_WELL_TRAINED))
            return;

        pet->CastSpell(pet, SPELL_WELL_TRAINED, true);

        ACTEST("HUN.PET.SUPERIORTRAINING", "pet={} well-trained after {} ms idle",
            Alonecraft::TestLog::N(pet), _idleMs);
    }

    uint32 _idleMs = 0;

    void Register() override
    {
        DoCheckProc          += AuraCheckProcFn(spell_hun_superior_training::CheckProc);
        OnEffectProc         += AuraEffectProcFn(spell_hun_superior_training::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
        DoEffectCalcPeriodic += AuraEffectCalcPeriodicFn(spell_hun_superior_training::CalcPeriodic, EFFECT_0, SPELL_AURA_DUMMY);
        OnEffectPeriodic     += AuraEffectPeriodicFn(spell_hun_superior_training::HandlePeriodic, EFFECT_0, SPELL_AURA_DUMMY);
    }
};

// ---------------------------------------------------------------------------
//  Catlike Reflexes -- a pet dodge resets Pack Hunting
// ---------------------------------------------------------------------------
//  The dodge itself is HitMask PROC_HIT_DODGE in spell_proc; all that is left
//  here is the reset, because 3.3.5 has no SPELL_EFFECT_RESET_COOLDOWN -- it is
//  a Cataclysm effect and every stock reset is a script.  Follows
//  WarriorImprovedCharge.cpp:59-67.
class spell_hun_catlike_reflexes : public AuraScript
{
    PrepareAuraScript(spell_hun_catlike_reflexes);

    void HandleProc(AuraEffect const* /*aurEff*/, ProcEventInfo& /*eventInfo*/)
    {
        PreventDefaultAction();

        Player* owner = GetPetOwner(GetUnitOwner());
        if (!owner || !owner->HasSpell(SPELL_PACK_HUNTING))
            return;

        owner->RemoveSpellCooldown(SPELL_PACK_HUNTING, true);

        // Pack Hunting is RecoveryTime 20000 / Category 0 today, so this is
        // belt-and-braces -- but the category is read off the spell rather than
        // assumed, so giving it one later does not silently break the reset.
        if (SpellInfo const* packHunting = sSpellMgr->GetSpellInfo(SPELL_PACK_HUNTING))
            if (uint32 category = packHunting->GetCategory())
                owner->RemoveCategoryCooldown(category);

        ACTEST("HUN.PET.CATLIKE", "pet={} dodged -> Pack Hunting reset for {}",
            Alonecraft::TestLog::N(GetUnitOwner()), Alonecraft::TestLog::N(owner));
    }

    void Register() override
    {
        OnEffectProc += AuraEffectProcFn(spell_hun_catlike_reflexes::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
    }
};

// ---------------------------------------------------------------------------
//  Invigoration -- a pet crit refunds mana and rearms Instinctive Fire
// ---------------------------------------------------------------------------
//  Core's spell_hun_invigoration (spell_hunter.cpp:561) is deliberately not
//  extended: it is a SpellScript on 53412 and nothing in src/server/ ever casts
//  53412, so the retail talent has never functioned on this core.
//
//  Core's 53398 is also not reused for the mana.  It is SPELL_EFFECT_ENERGIZE_PCT
//  with an implicit CASTER target, and the caster here is the pet -- it would
//  refill a mana bar the pet does not have.  200744 is cast by the owner on
//  itself with a computed flat amount, which sidesteps the targeting entirely.
class spell_hun_invigoration_pet : public AuraScript
{
    PrepareAuraScript(spell_hun_invigoration_pet);

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_MANA_RETURN, SPELL_INSTINCTIVE_FOCUS });
    }

    bool CheckProc(ProcEventInfo& /*eventInfo*/)
    {
        return GetPetOwner(GetUnitOwner()) != nullptr;
    }

    void HandleProc(AuraEffect const* /*aurEff*/, ProcEventInfo& /*eventInfo*/)
    {
        PreventDefaultAction();

        Unit* pet = GetUnitOwner();
        Player* owner = GetPetOwner(pet);
        if (!owner)
            return;

        int32 const pct = OwnerTalentAmount(pet,
            {TALENT_INVIGORATION_R1, TALENT_INVIGORATION_R2}, EFFECT_1);

        if (pct > 0)
        {
            int32 mana = CalculatePct(int32(owner->GetMaxPower(POWER_MANA)), pct);
            if (mana > 0)
                owner->CastCustomSpell(owner, SPELL_MANA_RETURN, &mana, nullptr, nullptr, true);
        }

        // Instinctive Fire is RecoveryTime 8000 with Category 0, so unlike
        // Pack Hunting there is no category to clear as well.
        owner->RemoveSpellCooldown(SPELL_INSTINCTIVE_FIRE, true);
        owner->CastSpell(owner, SPELL_INSTINCTIVE_FOCUS, true);

        ACTEST("HUN.PET.INVIGORATION", "pet={} crit -> owner={} manaPct={} instinctiveFire reset",
            Alonecraft::TestLog::N(pet), Alonecraft::TestLog::N(owner), pct);
    }

    void Register() override
    {
        DoCheckProc  += AuraCheckProcFn(spell_hun_invigoration_pet::CheckProc);
        OnEffectProc += AuraEffectProcFn(spell_hun_invigoration_pet::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
    }
};

void AddSC_hunter_bm_pet()
{
    RegisterSpellScript(spell_hun_taste_for_blood);
    RegisterSpellScript(spell_hun_thick_hide);
    RegisterSpellScript(spell_hun_superior_training);
    RegisterSpellScript(spell_hun_catlike_reflexes);
    RegisterSpellScript(spell_hun_invigoration_pet);
}
