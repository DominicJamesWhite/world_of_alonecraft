/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Affero General Public License as published by the
 * Free Software Foundation; either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "ScriptMgr.h"
#include "Player.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "SpellScriptLoader.h"
#include "Unit.h"
#include "Log.h"

enum MarkOfPenitenceSpells
{
    SPELL_EMPOWERED_RENEW_DAMAGE            = 200071,  // Custom spell ID for instant damage
    SPELL_PRIEST_EMPOWERED_RENEW_TALENT     = 63544,   // Original Empowered Renew spell (for reference)
};

enum PriestSpellIcons
{
    PRIEST_ICON_ID_EMPOWERED_RENEW_TALENT   = 3021,
};

// Mark of Penitence - Custom DOT spell with Empowered Renew support
class spell_mark_of_penitence : public AuraScript
{
    PrepareAuraScript(spell_mark_of_penitence);

    bool Load() override
    {
        return GetCaster() && GetCaster()->IsPlayer();
    }

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({SPELL_EMPOWERED_RENEW_DAMAGE});
    }

    void HandleApplyEffect(AuraEffect const* aurEff, AuraEffectHandleModes /*mode*/)
    {
        if (Unit* caster = GetCaster())
        {
            // Check for Empowered Renew talent
            if (AuraEffect const* empoweredRenewAurEff = caster->GetDummyAuraEffect(SPELLFAMILY_PRIEST, PRIEST_ICON_ID_EMPOWERED_RENEW_TALENT, EFFECT_1))
            {
                // Verify this is Mark of Penitence using family flags [2] 0x04000000
                if (GetSpellInfo()->SpellFamilyName == SPELLFAMILY_PRIEST && 
                    (GetSpellInfo()->SpellFamilyFlags[2] & 0x04000000))
                {
                    // Calculate instant damage based on total DOT damage
                    uint32 damage = GetEffect(EFFECT_0)->GetAmount();
                    damage = GetTarget()->SpellDamageBonusTaken(caster, GetSpellInfo(), damage, DOT);

                    // Calculate instant damage: talent percentage * total ticks * per-tick damage / 100
                    int32 basepoints0 = empoweredRenewAurEff->GetAmount() * GetEffect(EFFECT_0)->GetTotalTicks() * int32(damage) / 100;
                    
                    // Cast the instant damage spell
                    caster->CastCustomSpell(GetTarget(), SPELL_EMPOWERED_RENEW_DAMAGE, &basepoints0, nullptr, nullptr, true, nullptr, aurEff);
                }
            }
        }
    }

    void Register() override
    {
        OnEffectApply += AuraEffectApplyFn(spell_mark_of_penitence::HandleApplyEffect, EFFECT_0, SPELL_AURA_PERIODIC_DAMAGE, AURA_EFFECT_HANDLE_REAL_OR_REAPPLY_MASK);
    }
};

class spell_mark_of_penitence_loader : public SpellScriptLoader
{
public:
    spell_mark_of_penitence_loader() : SpellScriptLoader("spell_mark_of_penitence") { }

    AuraScript* GetAuraScript() const override
    {
        return new spell_mark_of_penitence();
    }
};

// Registration function
void AddSC_mark_of_penitence()
{
    new spell_mark_of_penitence_loader();
}
