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
//  Alonecraft Beast Mastery: the halves that live ON the hunter
// ---------------------------------------------------------------------------
//
//  The pet-side auras are in HunterBeastMasteryPet.cpp.  These two are here
//  because their trigger is something the HUNTER does -- casting Mend Pet,
//  channelling Aspect of the Viper -- so the aura is already in the right place
//  and no spell_pet_auras plumbing is involved.
//
//  Share the Spoils' kill proc and Bestial Discipline's crit-to-Focus are pure
//  DBC (woa_2026_08_11_11.sql / _15.sql); only the Viper amplification needs
//  code, and it needs it on the payload spell rather than the aspect itself.

enum HunterBeastMasteryHunterSpells
{
    SPELL_ASPECT_OF_THE_VIPER_ENERGIZE = 34075,

    SPELL_SPOILS_OF_THE_HUNT           = 200748,
    SPELL_MANA_RETURN                  = 200744,
};

// ---------------------------------------------------------------------------
//  Endurance Training -- Mend Pet also feeds the hunter's mana bar
// ---------------------------------------------------------------------------
//  spell_proc supplies the trigger (a Mend Pet periodic tick observed from the
//  hunter, shaped exactly like core's own -19572 row).  The one thing static
//  data cannot know is how much that tick actually healed for, which is what
//  this reads.
//
//  GetHealInfo() is populated on this path:
//  AuraEffect::HandlePeriodicHealAurasTick (SpellAuraEffects.cpp:6704) passes
//  &healInfo into ProcSkillsAndAuras alongside PROC_FLAG_DONE_PERIODIC.
//
//  GetHeal() rather than GetEffectiveHeal(): the talent pays out on the healing
//  done, so a pet already near full health does not quietly halve the return.
class spell_hun_endurance_training : public AuraScript
{
    PrepareAuraScript(spell_hun_endurance_training);

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_MANA_RETURN });
    }

    bool CheckProc(ProcEventInfo& eventInfo)
    {
        HealInfo* healInfo = eventInfo.GetHealInfo();
        return healInfo && healInfo->GetHeal() > 0;
    }

    void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();

        Unit* hunter = GetTarget();
        HealInfo* healInfo = eventInfo.GetHealInfo();
        if (!hunter || !healInfo)
            return;

        int32 mana = CalculatePct(int32(healInfo->GetHeal()), aurEff->GetAmount());
        if (mana <= 0)
            return;

        hunter->CastCustomSpell(hunter, SPELL_MANA_RETURN, &mana, nullptr, nullptr, true, nullptr, aurEff);

        ACTEST("HUN.ENDURANCETRAINING", "hunter={} heal={} pct={} -> mana={}",
            Alonecraft::TestLog::N(hunter), healInfo->GetHeal(), aurEff->GetAmount(), mana);
    }

    void Register() override
    {
        DoCheckProc  += AuraCheckProcFn(spell_hun_endurance_training::CheckProc);
        OnEffectProc += AuraEffectProcFn(spell_hun_endurance_training::HandleProc, EFFECT_2, SPELL_AURA_DUMMY);
    }
};

// ---------------------------------------------------------------------------
//  Share the Spoils -- Spoils of the Hunt amplifies Aspect of the Viper
// ---------------------------------------------------------------------------
//  This is a SpellScript on the ENERGIZE payload (34075), not an AuraScript on
//  the aspect (34074), and the distinction is the whole reason this file exists.
//
//  Core's spell_hun_ascpect_of_the_viper (spell_hunter.cpp:388-425) computes the
//  mana itself off weapon speed and base mana, then hands the number to 34075
//  via CastCustomSpell(SPELLVALUE_BASE_POINT0).  It consults exactly one
//  modifier on the way -- glyph 56851 -- so any additional aura on 34074 is
//  invisible to it.  An AuraScript there could only energize a SECOND time,
//  which stacks additively with the glyph instead of multiplying with it.
//  Scaling 34075's own effect value composes correctly and needs no core change.
class spell_hun_viper_spoils : public SpellScript
{
    PrepareSpellScript(spell_hun_viper_spoils);

    void HandleEnergize(SpellEffIndex /*effIndex*/)
    {
        Unit* caster = GetCaster();
        if (!caster)
            return;

        AuraEffect const* spoils = caster->GetAuraEffect(SPELL_SPOILS_OF_THE_HUNT, EFFECT_0);
        if (!spoils)
            return;

        // "increase ... by 500%" is a multiplier of 6, not 5.
        int32 const before = GetEffectValue();
        int32 const after = before + CalculatePct(before, spoils->GetAmount());
        SetEffectValue(after);

        ACTEST("HUN.SHARETHESPOILS", "caster={} viperMana {} -> {} (+{}%)",
            Alonecraft::TestLog::N(caster), before, after, spoils->GetAmount());
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_hun_viper_spoils::HandleEnergize, EFFECT_0, SPELL_EFFECT_ENERGIZE);
    }
};

void AddSC_hunter_bm_hunter()
{
    RegisterSpellScript(spell_hun_endurance_training);
    RegisterSpellScript(spell_hun_viper_spoils);
}
