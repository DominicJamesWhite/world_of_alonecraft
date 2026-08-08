#include "AlonecraftTestLog.h"
#include "ScriptMgr.h"
#include "SpellAuraEffects.h"
#include "SpellMgr.h"
#include "SpellScript.h"
#include "SpellScriptLoader.h"
#include "Unit.h"

// Tactical Mastery (redesigned) -- Warrior Arms talent
// Ranks: 12295 / 12676 / 12677 -- woa_2026_08_07_11.sql
//
// TODO.md: "Tactical Mastery (2, 1): Redesigned. Parrying an attack grants you
//  3/6/10% critical strike chance for 10 seconds. Landing a critical strike
//  grants you 3/6/10% parry chance for 10 seconds. (3 ranks)"
//
// Handled by DBC:
//   Effect0 = PROC_TRIGGER_SPELL -> Counterpoise (200610/200611/200612), the
//             crit buff.  Effect1 = PROC_TRIGGER_SPELL -> Guard Up
//             (200613/200614/200615), the parry buff.  Both payloads, their
//             magnitudes and their durations are entirely DBC.
//
// Handled by spell_proc:
//   ProcFlags 60 admits both directions, HitMask 34 admits both outcomes.
//
// Handled here:
//   Routing, and only routing.  spell_proc is per-spell, not per-effect, so the
//   table cannot express "effect 0 on parry, effect 1 on crit" -- it can only
//   say "parry or crit". Without this script a parry would grant both buffs and
//   so would a crit.
//
// Checking the hit mask alone is not enough, which is the subtle part.  An
// incoming attack that critically hits *you* carries PROC_EX_CRITICAL_HIT just
// as your own crit does; the two are told apart only by direction, so each
// check tests the type mask as well.  Getting this wrong would mean being
// critically hit granted you parry -- plausible-looking and completely wrong.

class spell_warr_tactical_mastery : public AuraScript
{
    PrepareAuraScript(spell_warr_tactical_mastery);

    // Effect 0: parry -> crit.  A parry is something that happens to us, so it
    // only ever arrives on the taken side.
    bool CheckParry(AuraEffect const* /*aurEff*/, ProcEventInfo& eventInfo)
    {
        bool const ok = (eventInfo.GetHitMask() & PROC_EX_PARRY)
            && (eventInfo.GetTypeMask() & TAKEN_HIT_PROC_FLAG_MASK);

        if (ok)
            ACTEST("WAR.TACMASTERY", "parry->crit talent={} hitMask={:#x} typeMask={:#x}",
                GetId(), eventInfo.GetHitMask(), eventInfo.GetTypeMask());

        return ok;
    }

    // Effect 1: crit -> parry.  Only our own critical strikes count, hence the
    // done-side test.
    bool CheckCrit(AuraEffect const* /*aurEff*/, ProcEventInfo& eventInfo)
    {
        bool const ok = (eventInfo.GetHitMask() & PROC_EX_CRITICAL_HIT)
            && (eventInfo.GetTypeMask() & DONE_HIT_PROC_FLAG_MASK);

        if (ok)
            ACTEST("WAR.TACMASTERY", "crit->parry talent={} hitMask={:#x} typeMask={:#x}",
                GetId(), eventInfo.GetHitMask(), eventInfo.GetTypeMask());

        return ok;
    }

    void Register() override
    {
        DoCheckEffectProc += AuraCheckEffectProcFn(spell_warr_tactical_mastery::CheckParry, EFFECT_0, SPELL_AURA_PROC_TRIGGER_SPELL);
        DoCheckEffectProc += AuraCheckEffectProcFn(spell_warr_tactical_mastery::CheckCrit,  EFFECT_1, SPELL_AURA_PROC_TRIGGER_SPELL);
    }
};

class spell_warr_tactical_mastery_loader : public SpellScriptLoader
{
public:
    spell_warr_tactical_mastery_loader() : SpellScriptLoader("spell_warr_tactical_mastery") { }

    AuraScript* GetAuraScript() const override
    {
        return new spell_warr_tactical_mastery();
    }
};

void AddSC_war_tactical_mastery()
{
    new spell_warr_tactical_mastery_loader();
}
