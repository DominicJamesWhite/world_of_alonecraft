#include "AlonecraftTestLog.h"
#include "ScriptMgr.h"
#include "SpellScript.h"
#include "SpellScriptLoader.h"
#include "Unit.h"

// Warrior Protection -- Intervene charges friend or foe
//
//   Intervene   (3411)   -- woa_2026_08_09_25.sql
//
// TODO.md:
//   "Intervene: Gains a second charge and is only usable in Defensive Stance.
//    Generates no rage.  Used on an enemy it reduces the damage they deal by
//    10% for 10 sec; used on an ally it reduces the damage they take by 10%
//    for 10 sec."
//
// woa_2026_08_09_24.sql had already turned Intervene into Protection's gap
// closer by re-pointing it at an enemy, which fixed the real problem (Charge is
// Battle Stance only, Intercept is Berserker Stance only, so the spec that must
// stand in Defensive Stance had no way to close) at the cost of the ability's
// original job.  This file gives both halves back: one button, and which half
// you get is decided by who you aimed it at.
//
// This is Holy Shock's shape, deliberately.  20473 is the core's one spell that
// heals a friend or damages an enemy from a single cast, and it does it with:
//
//   * a single SPELL_EFFECT_DUMMY effect on EffectImplicitTargetA = 25
//     (TARGET_UNIT_TARGET_ANY), so the client and Spell::CheckExplicitTarget
//     accept either kind of target, and
//   * a SpellScript whose dummy handler branches on Unit::IsFriendlyTo and
//     casts one of two triggered child spells (spell_pal_holy_shock,
//     src/server/scripts/Spells/spell_paladin.cpp:883).
//
// So 3411's Effect3 stopped being SPELL_EFFECT_TRIGGER_SPELL -> 200640 and
// became a dummy, and both its effects moved to target 25.  The two payloads:
//
//   200640 Staggered  -- enemy, -10% damage done,  10 sec (target 6, negative)
//   200650 Interceded -- ally,  -10% damage taken, 10 sec (target 21, positive)
//
// Why there is no DBC-only version of this.  Implicit targets are resolved per
// effect but validated for the spell as a whole, so an Effect2 with target 21
// and an Effect3 with target 6 does not mean "whichever applies" -- it means a
// cast that can satisfy neither and fails outright.  Nothing in the DBC
// expresses "trigger this only if the target is hostile".
//
// Two consequences of copying Holy Shock worth recording, because both are
// invisible until you look for them:
//
//   1. 3411 is now a *positive* spell as far as SpellInfo is concerned.
//      _IsPositiveTarget only treats the explicitly-enemy target types as
//      negative, and 25 is not one of them (SpellInfo.cpp:3243), which is
//      equally true of Holy Shock.  The practical effect is in EffectCharge
//      (SpellEffects.cpp:4929): it only pins the charge to a moving target GUID
//      for non-positive spells, so an enemy charge now lands on the position
//      the target occupied at cast time rather than tracking them.  Charge and
//      Intercept keep the tracking because they are unambiguously hostile.
//
//   2. Aggro comes from the debuff, not from the charge.  200640 is negative
//      and lands on the enemy, exactly as Holy Shock's damage half does the
//      engaging there.
//
// Rage is untouched: 3411 has no SPELL_EFFECT_ENERGIZE and its 10 rage cost was
// removed in woa_2026_08_09_18.sql.

enum WarriorInterveneSpells
{
    SPELL_WARRIOR_INTERVENE_STAGGERED  = 200640,
    SPELL_WARRIOR_INTERVENE_INTERCEDED = 200650
};

class spell_warr_intervene_dual : public SpellScript
{
    PrepareSpellScript(spell_warr_intervene_dual);

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_WARRIOR_INTERVENE_STAGGERED, SPELL_WARRIOR_INTERVENE_INTERCEDED });
    }

    // Mirrors spell_pal_holy_shock::CheckCast.  A friendly target needs no
    // extra checks -- the engine's own ally validation has already run -- but a
    // hostile one has to pass the attack-target and facing tests that a spell
    // declared with TARGET_UNIT_TARGET_ANY never gets for free.
    SpellCastResult CheckCast()
    {
        Unit* caster = GetCaster();
        Unit* target = GetExplTargetUnit();

        if (!target)
            return SPELL_FAILED_BAD_TARGETS;

        if (!caster->IsFriendlyTo(target))
        {
            if (!caster->IsValidAttackTarget(target))
                return SPELL_FAILED_BAD_TARGETS;

            if (!caster->isInFront(target))
                return SPELL_FAILED_UNIT_NOT_INFRONT;
        }

        return SPELL_CAST_OK;
    }

    void HandleDummy(SpellEffIndex /*effIndex*/)
    {
        Unit* caster = GetCaster();
        Unit* target = GetHitUnit();

        if (!caster || !target)
            return;

        bool const friendly = caster->IsFriendlyTo(target);
        uint32 const payload = friendly ? SPELL_WARRIOR_INTERVENE_INTERCEDED : SPELL_WARRIOR_INTERVENE_STAGGERED;

        ACTEST("WAR.INTERVENE", "target={} friendly={} payload={}", target->GetName(), friendly, payload);
        caster->CastSpell(target, payload, true);
    }

    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_warr_intervene_dual::CheckCast);
        OnEffectHitTarget += SpellEffectFn(spell_warr_intervene_dual::HandleDummy, EFFECT_2, SPELL_EFFECT_DUMMY);
    }
};

class spell_warr_intervene_dual_loader : public SpellScriptLoader
{
public:
    spell_warr_intervene_dual_loader() : SpellScriptLoader("spell_warr_intervene_dual") { }

    SpellScript* GetSpellScript() const override
    {
        return new spell_warr_intervene_dual();
    }
};

void AddSC_war_intervene()
{
    new spell_warr_intervene_dual_loader();
}
