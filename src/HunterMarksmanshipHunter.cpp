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
//  Alonecraft Marksmanship: the halves that live ON the hunter
// ---------------------------------------------------------------------------
//
//  The pet-side scripts are in HunterMarksmanshipPet.cpp.  These three are here
//  because their trigger is something the hunter does -- landing a shot,
//  critting with Aimed/Steady/Chimera -- so the talent aura is already on the
//  right unit and no spell_pet_auras plumbing is involved.
//
//  Four of the seven Marksmanship redesigns need no code at all: Concussive
//  Barrage and Improved Barrage are pure spellmods, and Go for the Throat's
//  Focus half and Focused Aim's hit half were already correct in retail.

enum HunterMarksmanshipHunterSpells
{
    // Instinctive Fire (repurposed Beast Lore) -- woa_2026_08_11_03.sql.
    // 1462 is the button; 200742 is the 6s ranged-haste buff it triggers.
    SPELL_INSTINCTIVE_FIRE_BUFF = 200742,

    // Pack Hunting (repurposed Freezing Arrow) -- woa_2026_08_11_04.sql.
    SPELL_PACK_HUNTING          = 60192,

    // Lacerating Shot (repurposed Scare Beast) -- woa_2026_08_11_02.sql.
    SPELL_LACERATING_SHOT_R1    = 1513,
    SPELL_LACERATING_SHOT_R2    = 14326,
    SPELL_LACERATING_SHOT_R3    = 14327,

    // Improved Hunter's Mark's pet heal -- woa_2026_08_12_10.sql.
    SPELL_IMPROVED_HUNTERS_MARK_HEAL = 200762
};

namespace
{
    // Hunter's Mark, all five ranks: SpellFamilyName 9, SpellFamilyFlags 1024,
    // aura 127 on EFFECT_1.  The family-mask overload matches every rank in one
    // call, which is why the ids are not listed here.
    constexpr uint32 HUNTERS_MARK_FAMILY_FLAG = 1024;
}

// ---------------------------------------------------------------------------
//  Lethal Instincts -- shots refresh Instinctive Fire
// ---------------------------------------------------------------------------
//  Only the REFRESH is here.  The talent's other half, +3/6 sec on the buff's
//  duration, is a plain SPELLMOD_DURATION on Effect2 and needs no code -- which
//  is the point: a spellmod applies to every application of the buff, including
//  a normal Instinctive Fire cast that this script never sees.
//
//  spell_proc supplies everything about WHEN this fires: the four shot family
//  masks, the ranged-damage proc flag, and the 50/100% split.  The only reason
//  the refresh is code at all is the word "refresh".
//
//  A DBC EffectTriggerSpell of 200742 would APPLY the buff, handing permanent
//  +20% ranged haste to a hunter who never presses Instinctive Fire.  Gating it
//  with CasterAuraSpell does not work either: a proc-triggered cast runs with
//  TRIGGERED_FULL_MASK, which includes TRIGGERED_IGNORE_CASTER_AURASTATE
//  (SpellDefines.h:145), so the check at Spell.cpp:5756 never runs.
//
//  Aura::RefreshDuration (SpellAuras.cpp:822) is the whole payload -- it resets
//  the duration to max without recalculating the amount, which is what "refresh"
//  should mean for a buff whose size does not depend on the refreshing event.
//  Note "max" already includes the +3/6 sec, because the modifier was applied
//  when the aura was created (Aura::CalcMaxDuration, SpellAuras.cpp:806).
class spell_hun_lethal_instincts : public AuraScript
{
    PrepareAuraScript(spell_hun_lethal_instincts);

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_INSTINCTIVE_FIRE_BUFF });
    }

    void HandleProc(AuraEffect const* /*aurEff*/, ProcEventInfo& /*eventInfo*/)
    {
        PreventDefaultAction();

        Unit* hunter = GetTarget();

        // Scoped to the hunter's own buff.  Irrelevant solo, free to be right.
        Aura* fire = hunter->GetAura(SPELL_INSTINCTIVE_FIRE_BUFF, hunter->GetGUID());
        if (!fire)
            return;

        fire->RefreshDuration();

        ACTEST("HUN.MM.LETHALINST", "hunter={} refreshed Instinctive Fire to {} ms",
            Alonecraft::TestLog::N(hunter), fire->GetDuration());
    }

    void Register() override
    {
        OnEffectProc += AuraEffectProcFn(spell_hun_lethal_instincts::HandleProc,
                                         EFFECT_0, SPELL_AURA_DUMMY);
    }
};

// ---------------------------------------------------------------------------
//  Rapid Killing -- shots keep Pack Hunting alive
// ---------------------------------------------------------------------------
//  The Rapid Fire cooldown reduction is a plain spellmod on Effect2 and is not
//  touched here.  spell_proc supplies the four shot family masks.
//
//  This REFRESHES an active Pack Hunting; it never applies one.  That is the
//  whole design -- the redirect is something the rotation sustains rather than
//  something a kill hands back -- and it is also why the DBC cannot do it.  A
//  triggered re-cast of 60192 would re-run SPELL_EFFECT_REDIRECT_THREAT,
//  re-register the redirect and burn the 20s cooldown; Aura::RefreshDuration
//  (SpellAuras.cpp:822) is what is actually wanted, and there is no spell effect
//  for it in 3.3.5.
//
//  Nothing is needed on removal: the redirect teardown already rides Pack
//  Hunting's own aura (spell_hun_pack_hunting_aura, HunterPackHunting.cpp:70),
//  and refreshing the duration does not re-trigger that.
class spell_hun_rapid_killing : public AuraScript
{
    PrepareAuraScript(spell_hun_rapid_killing);

    void HandleProc(AuraEffect const* /*aurEff*/, ProcEventInfo& /*eventInfo*/)
    {
        PreventDefaultAction();

        Unit* hunter = GetTarget();

        // Scoped to the hunter's own cast -- Pack Hunting is self-cast, so this
        // is only belt and braces, but it costs nothing to be exact.
        Aura* packHunting = hunter->GetAura(SPELL_PACK_HUNTING, hunter->GetGUID());
        if (!packHunting)
            return;

        packHunting->RefreshDuration();

        ACTEST("HUN.MM.RAPIDKILL", "hunter={} refreshed Pack Hunting to {} ms",
            Alonecraft::TestLog::N(hunter), packHunting->GetDuration());
    }

    void Register() override
    {
        OnEffectProc += AuraEffectProcFn(spell_hun_rapid_killing::HandleProc,
                                         EFFECT_0, SPELL_AURA_DUMMY);
    }
};

// ---------------------------------------------------------------------------
//  Piercing Shots -- the same crit also refreshes Lacerating Shot
// ---------------------------------------------------------------------------
//  This runs ALONGSIDE core's spell_hun_piercing_shots (spell_hunter.cpp:1622),
//  which still does the bleed.  Two scripts on one spell id is supported --
//  _spellScriptsStore is a multimap (ObjectMgr.h:390) and
//  Aura::CallScriptEffectProcHandlers iterates all of them (SpellAuras.cpp:2723).
//
//  TWO THINGS THIS SCRIPT MUST NOT DO, both of which would break core's bleed:
//
//    * No DoCheckProc.  Aura::CallScriptCheckProcHandlers ANDs the results of
//      every script (SpellAuras.cpp:2625), so returning false here would
//      suppress the bleed as well.  The chance roll therefore lives inside the
//      proc handler.
//    * No PreventDefaultAction.  Prevention is global across scripts
//      (SpellAuras.cpp:2734).
//
//  The chance is read from the DBC ProcChance (33/66/100) rather than
//  spell_proc.Chance, which has to stay 100 for the bleed.  That is also the
//  value $h renders in the tooltip, so the number the player reads and the
//  number rolled here are the same field by construction.
class spell_hun_piercing_shots_lacerate : public AuraScript
{
    PrepareAuraScript(spell_hun_piercing_shots_lacerate);

    void HandleProc(AuraEffect const* /*aurEff*/, ProcEventInfo& eventInfo)
    {
        Unit* victim = eventInfo.GetActionTarget();
        if (!victim)
            return;

        if (!roll_chance_i(int32(GetSpellInfo()->ProcChance)))
            return;

        // Lacerating Shot's SpellFamilyFlags are all zero after its own rework,
        // so family-mask matching is impossible and the ranks are named.  Same
        // constraint as spell_hun_taste_for_blood.
        for (uint32 rank : { SPELL_LACERATING_SHOT_R1,
                             SPELL_LACERATING_SHOT_R2,
                             SPELL_LACERATING_SHOT_R3 })
        {
            if (Aura* bleed = victim->GetAura(rank, GetTarget()->GetGUID()))
            {
                bleed->RefreshDuration();

                ACTEST("HUN.MM.PIERCING", "hunter={} refreshed Lacerating Shot {} on {}",
                    Alonecraft::TestLog::N(GetTarget()), rank,
                    Alonecraft::TestLog::N(victim));
                return;
            }
        }
    }

    void Register() override
    {
        OnEffectProc += AuraEffectProcFn(spell_hun_piercing_shots_lacerate::HandleProc,
                                         EFFECT_0, SPELL_AURA_PROC_TRIGGER_SPELL);
    }
};

// ---------------------------------------------------------------------------
//  Improved Hunter's Mark -- your damage on a marked target heals your pet
// ---------------------------------------------------------------------------
//  The aura sits on the hunter, unlike Focused Aim, because the triggering
//  event is the hunter's OWN damage -- that already reaches the hunter's aura
//  list and no spell_pet_auras plumbing is needed.
//
//  YOUR damage only.  The pet's swings are deliberately not a source: the proc
//  flags are DONE_* on the hunter, so a pet event never reaches this aura in
//  the first place (Unit::ProcSkillsAndReactives, Unit.cpp:6789).
//
//  200762 carries SPELL_ATTR1_NO_THREAT, which is what stops this handing the
//  hunter assist threat proportional to their own damage -- see the SQL header
//  for why that would have quietly wrecked the pet-tanking design.
class spell_hun_improved_hunters_mark : public AuraScript
{
    PrepareAuraScript(spell_hun_improved_hunters_mark);

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_IMPROVED_HUNTERS_MARK_HEAL });
    }

    void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();

        Player* hunter = GetTarget()->ToPlayer();
        if (!hunter)
            return;

        // DONE_PERIODIC also covers periodic HEALING, so Mend Pet ticks reach
        // this handler.  GetDamageInfo() is null on that path.
        DamageInfo* damageInfo = eventInfo.GetDamageInfo();
        if (!damageInfo || !damageInfo->GetDamage())
            return;

        Unit* victim = eventInfo.GetProcTarget();
        if (!victim)
            return;

        Unit* pet = hunter->GetGuardianPet();
        if (!pet || !pet->IsAlive())
            return;

        // The mark has to be the HUNTER's own -- another hunter's Hunter's Mark
        // on the same target must not feed this pet.
        if (!victim->GetAuraEffect(SPELL_AURA_RANGED_ATTACK_POWER_ATTACKER_BONUS,
                                   SPELLFAMILY_HUNTER, HUNTERS_MARK_FAMILY_FLAG, 0, 0,
                                   hunter->GetGUID()))
            return;

        int32 amount = CalculatePct(int32(damageInfo->GetDamage()), aurEff->GetAmount());
        if (amount <= 0)
            return;

        hunter->CastCustomSpell(pet, SPELL_IMPROVED_HUNTERS_MARK_HEAL, &amount,
                                nullptr, nullptr, true, nullptr, aurEff);

        ACTEST("HUN.MM.IMPMARK", "hunter={} target={} damage={} pct={} healedPet={} for {}",
            Alonecraft::TestLog::N(hunter), Alonecraft::TestLog::N(victim),
            damageInfo->GetDamage(), aurEff->GetAmount(),
            Alonecraft::TestLog::N(pet), amount);
    }

    void Register() override
    {
        OnEffectProc += AuraEffectProcFn(spell_hun_improved_hunters_mark::HandleProc,
                                         EFFECT_2, SPELL_AURA_DUMMY);
    }
};

void AddSC_hunter_mm_hunter()
{
    RegisterSpellScript(spell_hun_lethal_instincts);
    RegisterSpellScript(spell_hun_rapid_killing);
    RegisterSpellScript(spell_hun_piercing_shots_lacerate);
    RegisterSpellScript(spell_hun_improved_hunters_mark);
}
