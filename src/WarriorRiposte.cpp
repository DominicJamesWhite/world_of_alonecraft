#include "AlonecraftTestLog.h"
#include "ScriptMgr.h"
#include "SpellScript.h"
#include "SpellScriptLoader.h"
#include "Unit.h"

// Riposte (was Iron Will) -- Warrior Arms talent
// Talent ranks: 12300 / 12959 / 12960
// Counterattacks: 200600 / 200601 / 200602 -- woa_2026_08_07_10.sql
//
// TODO.md: "Iron Will (Renamed to Riposte): Redesigned. Parrying an attack
//  immediately counter-attacks every enemy within 8 yards for 40/70/100%
//  weapon damage. Cannot occur more than once every second. Requires 5 points
//  in Deflection. (Only affects enemies where CC wouldn't be broken)"
//
// Handled by DBC:
//   The talent is SPELL_AURA_PROC_TRIGGER_SPELL onto the counterattack, and the
//   counterattack is SPELL_EFFECT_WEAPON_PERCENT_DAMAGE with
//   EffectImplicitTargetA 22 (TARGET_UNIT_SRC_AREA_ENEMY) at radius index 14
//   (8 yards).  Damage, radius and rank scaling are all DBC.
//
// Handled by spell_proc:
//   HitMask 32 (PROC_EX_PARRY) selects the parry, and Cooldown 2000 is the
//   rate limit.  Shipped at 2 sec rather than the 1 sec in TODO.md -- see the
//   SQL file for why.
//
// Handled here:
//   Only the crowd-control exclusion.  Nothing else -- if this script were
//   removed the ability would still work, it would just break sheeps.
//
// Same predicate and reasoning as WarlockRuin.cpp's spread filter.

class spell_warr_riposte : public SpellScript
{
    PrepareSpellScript(spell_warr_riposte);

    void FilterTargets(std::list<WorldObject*>& targets)
    {
        uint32 const before = targets.size();

        targets.remove_if([](WorldObject* obj) -> bool
        {
            Unit* unit = obj->ToUnit();
            // Never break someone else's crowd control.
            return !unit || unit->HasBreakableByDamageCrowdControlAura();
        });

        if (targets.size() != before)
            ACTEST("WAR.RIPOSTE", "spell={} targets={} -> {} (skipped breakable-cc)",
                GetSpellInfo()->Id, before, targets.size());
    }

    void Register() override
    {
        OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(
            spell_warr_riposte::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);
    }
};

class spell_warr_riposte_loader : public SpellScriptLoader
{
public:
    spell_warr_riposte_loader() : SpellScriptLoader("spell_warr_riposte") { }

    SpellScript* GetSpellScript() const override
    {
        return new spell_warr_riposte();
    }
};

void AddSC_war_riposte()
{
    new spell_warr_riposte_loader();
}
