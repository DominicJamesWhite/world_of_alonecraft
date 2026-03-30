/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Affero General Public License as published by the
 * Free Software Foundation; either version 3 of the License, or (at your
 * option) any later version.
 */

#include "CellImpl.h"
#include "GridNotifiers.h"
#include "ScriptMgr.h"
#include "SpellAuraEffects.h"
#include "SpellScript.h"
#include "SpellScriptLoader.h"
#include "Unit.h"

enum FocusedPowerSpells
{
    SPELL_FOCUSED_POWER_R1 = 33186,
    SPELL_FOCUSED_POWER_R2 = 33190,
};

static bool HasFocusedPower(Unit const* caster)
{
    return caster->HasAura(SPELL_FOCUSED_POWER_R1) ||
           caster->HasAura(SPELL_FOCUSED_POWER_R2);
}

// Mind Sear (48045, 53023) - On channel start, spread SW:P to nearby enemies
class spell_pri_focused_power_mind_sear : public SpellScript
{
    PrepareSpellScript(spell_pri_focused_power_mind_sear);

    void HandleAfterCast()
    {
        Unit* caster = GetCaster();
        Unit* target = GetExplTargetUnit();

        if (!caster || !target || !HasFocusedPower(caster))
            return;

        // Check if target has SW:P from this caster (family flag [0] = 0x8000)
        AuraEffect* swpEffect = target->GetAuraEffect(
            SPELL_AURA_PERIODIC_DAMAGE, SPELLFAMILY_PRIEST,
            0x8000, 0, 0, caster->GetGUID());

        if (!swpEffect)
            return;

        uint32 swpSpellId = swpEffect->GetBase()->GetId();

        // Find all hostile units within 10 yards of the target
        std::list<Unit*> enemies;
        Acore::AnyUnfriendlyUnitInObjectRangeCheck check(target, caster, 10.0f);
        Acore::UnitListSearcher<Acore::AnyUnfriendlyUnitInObjectRangeCheck> searcher(target, enemies, check);
        Cell::VisitObjects(target, searcher, 10.0f);

        for (Unit* enemy : enemies)
        {
            if (enemy == target)
                continue;

            if (!caster->IsValidAttackTarget(enemy))
                continue;

            // Skip if enemy already has SW:P from this caster
            if (enemy->GetAuraEffect(SPELL_AURA_PERIODIC_DAMAGE, SPELLFAMILY_PRIEST,
                0x8000, 0, 0, caster->GetGUID()))
                continue;

            caster->CastSpell(enemy, swpSpellId, true);
        }
    }

    void Register() override
    {
        AfterCast += SpellCastFn(spell_pri_focused_power_mind_sear::HandleAfterCast);
    }
};

class spell_pri_focused_power_mind_sear_loader : public SpellScriptLoader
{
public:
    spell_pri_focused_power_mind_sear_loader() : SpellScriptLoader("spell_pri_focused_power_mind_sear") { }

    SpellScript* GetSpellScript() const override
    {
        return new spell_pri_focused_power_mind_sear();
    }
};

// Holy Nova (damage spells) - On hit, spread Mark of Penitence to nearby enemies
class spell_pri_focused_power_holy_nova : public SpellScript
{
    PrepareSpellScript(spell_pri_focused_power_holy_nova);

    bool _spread = false;

    void HandleAfterHit()
    {
        if (_spread)
            return;

        Unit* caster = GetCaster();
        Unit* target = GetHitUnit();

        if (!caster || !target || !HasFocusedPower(caster))
            return;

        // Check if target has Mark of Penitence from this caster (family flag [2] = 0x04000000)
        AuraEffect* mopEffect = target->GetAuraEffect(
            SPELL_AURA_PERIODIC_DAMAGE, SPELLFAMILY_PRIEST,
            0, 0, 0x04000000, caster->GetGUID());

        if (!mopEffect)
            return;

        _spread = true;
        uint32 mopSpellId = mopEffect->GetBase()->GetId();

        // Find all hostile units within 10 yards of the caster (Holy Nova radius)
        std::list<Unit*> enemies;
        Acore::AnyUnfriendlyUnitInObjectRangeCheck check(caster, caster, 10.0f);
        Acore::UnitListSearcher<Acore::AnyUnfriendlyUnitInObjectRangeCheck> searcher(caster, enemies, check);
        Cell::VisitObjects(caster, searcher, 10.0f);

        for (Unit* enemy : enemies)
        {
            if (!caster->IsValidAttackTarget(enemy))
                continue;

            // Skip if enemy already has MoP from this caster
            if (enemy->GetAuraEffect(SPELL_AURA_PERIODIC_DAMAGE, SPELLFAMILY_PRIEST,
                0, 0, 0x04000000, caster->GetGUID()))
                continue;

            caster->CastSpell(enemy, mopSpellId, true);
        }
    }

    void Register() override
    {
        AfterHit += SpellHitFn(spell_pri_focused_power_holy_nova::HandleAfterHit);
    }
};

class spell_pri_focused_power_holy_nova_loader : public SpellScriptLoader
{
public:
    spell_pri_focused_power_holy_nova_loader() : SpellScriptLoader("spell_pri_focused_power_holy_nova") { }

    SpellScript* GetSpellScript() const override
    {
        return new spell_pri_focused_power_holy_nova();
    }
};

void AddSC_focused_power()
{
    new spell_pri_focused_power_mind_sear_loader();
    new spell_pri_focused_power_holy_nova_loader();
}
