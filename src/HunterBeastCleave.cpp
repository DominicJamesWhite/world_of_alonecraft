/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license:
 * https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE-AGPL3
 */

#include "Player.h"
#include "ScriptMgr.h"
#include "SpellAuraEffects.h"
#include "SpellScript.h"
#include "SpellScriptLoader.h"
#include "Unit.h"

// Beast Cleave (replaces Aspect of the Beast) -- Hunter, trained at level 30
// Spells: 13161 the button / 200739 the splash -- woa_2026_08_11_00.sql
//
// TODO.md: "For 15 sec your pet's melee attacks also strike all other enemies
//  within 8 yds for 75% of the damage dealt.  30s cooldown."
//
// Handled by DBC:
//   13161's aura is applied with TARGET_UNIT_PET, so it lands on the pet and
//   nothing here has to find one.  DurationIndex 8 is the 15s window,
//   RecoveryTime the 30s cooldown, and a spell_proc row supplies the proc flags
//   (melee auto-attack | melee damage class) that make the dummy aura fire.
//   200739 does its own area selection with TARGET_SRC_CASTER /
//   TARGET_UNIT_SRC_AREA_ENEMY and radius index 14 (8 yd), the same targeting
//   Whirlwind (1680) uses.
//
//   A spell_threat row of (13161, 0, 1, 0) zeroes the cast's initial threat
//   (woa_2026_08_11_06.sql).  Without it HandleThreatSpells falls back to
//   SpellLevel, and 30 threat lands on the HUNTER for every mob the pet is
//   already holding -- Category 0 means the category-47 aspect exemption in
//   SpellMgr.cpp:3493 no longer covers this spell.
//
// Handled here:
//   The one number nothing static can know -- how hard the pet just hit -- and
//   the exclusion of the primary target, which has already taken that hit.
//
// Two details decide whether this behaves:
//
//   * DamageInfo::GetDamage() is POST-mitigation, which is what "75% of the
//     damage dealt" means.  It is also why 200739 carries
//     SPELL_ATTR0_CU_IGNORE_ARMOR: the parent hit was mitigated once already,
//     and letting Unit::IsDamageReducedByArmor run again would double-dip.
//     (Sweeping Strikes uses GetUnmitigatedDamage() instead, because its extra
//     attack is a fresh weapon swing rather than an echo of one.)
//
//   * The primary victim is passed to CastCustomSpell as the EXPLICIT target,
//     which survives independently of Effect1's implicit area selection.  That
//     is what lets FilterTargets remove exactly it, and nothing else, without
//     having to re-derive who was hit.
//
// 200739 is SPELL_DAMAGE_CLASS_NONE with no spell_bonus_data row, so
// Unit::SpellDamageBonusDone (Unit.cpp:8898) returns before applying any
// coefficient -- the amount passed in is the amount dealt.  That, plus
// SPELL_ATTR3_SUPPRESS_CASTER_PROCS, is what stops the splash proccing itself.

enum BeastCleaveSpells
{
    SPELL_HUNTER_BEAST_CLEAVE        = 13161,
    SPELL_HUNTER_BEAST_CLEAVE_DAMAGE = 200739,

    // Against the Odds (Beast Mastery talent 19559/19560) marks the pet when a
    // Multi-Shot triggers a free Beast Cleave -- woa_2026_08_11_12.sql.
    SPELL_HUNTER_AGAINST_THE_ODDS    = 200749
};

// Lives on the PET, because 13161's aura is applied with TARGET_UNIT_PET.
class spell_hun_beast_cleave : public AuraScript
{
    PrepareAuraScript(spell_hun_beast_cleave);

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_HUNTER_BEAST_CLEAVE_DAMAGE });
    }

    bool CheckProc(ProcEventInfo& eventInfo)
    {
        DamageInfo* damageInfo = eventInfo.GetDamageInfo();
        if (!damageInfo || !damageInfo->GetDamage())
        {
            return false;
        }

        // Never echo our own splash.
        if (SpellInfo const* spellInfo = eventInfo.GetSpellInfo())
        {
            if (spellInfo->Id == SPELL_HUNTER_BEAST_CLEAVE_DAMAGE)
            {
                return false;
            }
        }

        return eventInfo.GetProcTarget() != nullptr;
    }

    void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();

        DamageInfo* damageInfo = eventInfo.GetDamageInfo();
        if (!damageInfo)
        {
            return;
        }

        // Against the Odds empowers the window rather than duplicating it.  A
        // second "empowered Beast Cleave" aura would sit alongside this one,
        // pass CheckProc too, and splash twice on every swing; a marker the
        // splash reads cannot do that.  It also means a manually-cast Beast
        // Cleave inside the window benefits, which is the intended feel.
        int32 pct = aurEff->GetAmount();
        if (AuraEffect const* odds = GetTarget()->GetAuraEffect(SPELL_HUNTER_AGAINST_THE_ODDS, EFFECT_0))
        {
            pct += CalculatePct(pct, odds->GetAmount());
        }

        int32 damage = CalculatePct(int32(damageInfo->GetDamage()), pct);
        if (damage <= 0)
        {
            return;
        }

        // GetTarget() is the pet, so the pet is the splash's caster -- correct
        // for damage attribution, threat and pet damage modifiers.
        GetTarget()->CastCustomSpell(eventInfo.GetProcTarget(), SPELL_HUNTER_BEAST_CLEAVE_DAMAGE,
                                     &damage, nullptr, nullptr, true, nullptr, aurEff);
    }

    void Register() override
    {
        DoCheckProc += AuraCheckProcFn(spell_hun_beast_cleave::CheckProc);
        OnEffectProc += AuraEffectProcFn(spell_hun_beast_cleave::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
    }
};

class spell_hun_beast_cleave_damage : public SpellScript
{
    PrepareSpellScript(spell_hun_beast_cleave_damage);

    void FilterTargets(std::list<WorldObject*>& targets)
    {
        // The primary victim already took the pet's full hit.
        if (Unit* primary = GetExplTargetUnit())
        {
            targets.remove(primary);
        }
    }

    void Register() override
    {
        OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(
            spell_hun_beast_cleave_damage::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);
    }
};

// TARGET_UNIT_PET with no pet does not fail the cast -- Spell.cpp:1823 guards
// with `if (target && target->ToUnit())`, so the aura effect is silently
// skipped while the cooldown still burns.  This makes the failure visible.
class spell_hun_beast_cleave_cast : public SpellScript
{
    PrepareSpellScript(spell_hun_beast_cleave_cast);

    SpellCastResult CheckCast()
    {
        if (!GetCaster()->GetGuardianPet())
        {
            return SPELL_FAILED_NO_PET;
        }

        return SPELL_CAST_OK;
    }

    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_hun_beast_cleave_cast::CheckCast);
    }
};

void AddSC_hunter_beast_cleave()
{
    RegisterSpellScript(spell_hun_beast_cleave);
    RegisterSpellScript(spell_hun_beast_cleave_damage);
    RegisterSpellScript(spell_hun_beast_cleave_cast);
}
