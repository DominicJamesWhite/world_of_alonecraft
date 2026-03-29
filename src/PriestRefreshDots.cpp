/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Affero General Public License as published by the
 * Free Software Foundation; either version 3 of the License, or (at your
 * option) any later version.
 */

#include "ScriptMgr.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "SpellScriptLoader.h"
#include "Unit.h"

// 200086 - Mind Blast helper -> refresh Devouring Plague
class spell_pri_refresh_devouring_plague : public SpellScript
{
    PrepareSpellScript(spell_pri_refresh_devouring_plague);

    void HandleScriptEffect(SpellEffIndex /*effIndex*/)
    {
        Unit* unitTarget = GetHitUnit();
        Unit* caster = GetCaster();

        if (!unitTarget || !caster)
            return;

        // Devouring Plague search by exact spell IDs
        static const uint32 DP_IDS[] = { 2944, 19276, 19277, 19278, 19279, 19280, 25467, 48299, 48300 };
        for (uint32 id : DP_IDS)
        {
            if (Aura* base = unitTarget->GetAura(id, caster->GetGUID()))
            {
                // Find periodic damage effect
                AuraEffect* dam = nullptr;
                for (uint8 i = 0; i < 3; ++i)
                {
                    if (AuraEffect* e = base->GetEffect(i))
                    {
                        if (e->GetAuraType() == SPELL_AURA_PERIODIC_DAMAGE)
                        {
                            dam = e;
                            break;
                        }
                    }
                }

                base->RefreshTimersWithMods();

                if (dam)
                {
                    if (Unit* aurCaster = dam->GetCaster())
                        dam->ChangeAmount(dam->CalculateAmount(aurCaster), false);
                }

                return; // done
            }
        }
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_pri_refresh_devouring_plague::HandleScriptEffect, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
    }
};

class spell_pri_refresh_devouring_plague_loader : public SpellScriptLoader
{
public:
    spell_pri_refresh_devouring_plague_loader() : SpellScriptLoader("spell_pri_refresh_devouring_plague") { }

    SpellScript* GetSpellScript() const override
    {
        return new spell_pri_refresh_devouring_plague();
    }
};

// 200087 - Smite helper -> refresh Mark of Penitence
class spell_pri_refresh_mark_of_penitence : public SpellScript
{
    PrepareSpellScript(spell_pri_refresh_mark_of_penitence);

    void HandleScriptEffect(SpellEffIndex /*effIndex*/)
    {
        Unit* unitTarget = GetHitUnit();
        Unit* caster = GetCaster();

        if (!unitTarget || !caster)
            return;

        // Mark of Penitence flags: [2] 0x04000000
        if (AuraEffect* aur = unitTarget->GetAuraEffect(SPELL_AURA_PERIODIC_DAMAGE, SPELLFAMILY_PRIEST, 0x00000000, 0x00000000, 0x04000000, caster->GetGUID()))
        {
            aur->GetBase()->RefreshTimersWithMods();

            if (Unit* aurCaster = aur->GetCaster())
                aur->ChangeAmount(aur->CalculateAmount(aurCaster), false);
        }
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_pri_refresh_mark_of_penitence::HandleScriptEffect, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
    }
};

class spell_pri_refresh_mark_of_penitence_loader : public SpellScriptLoader
{
public:
    spell_pri_refresh_mark_of_penitence_loader() : SpellScriptLoader("spell_pri_refresh_mark_of_penitence") { }

    SpellScript* GetSpellScript() const override
    {
        return new spell_pri_refresh_mark_of_penitence();
    }
};

void AddSC_priest_refresh_dots()
{
    new spell_pri_refresh_devouring_plague_loader();
    new spell_pri_refresh_mark_of_penitence_loader();
}
