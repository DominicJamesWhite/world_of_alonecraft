#include "Player.h"
#include "ScriptMgr.h"
#include "Spell.h"
#include "SpellAuraEffects.h"
#include "SpellScript.h"
#include "Log.h"
#include "Unit.h"
#include "SpellMgr.h"
#include "SpellScriptLoader.h"

// Arcane Fortitude talent IDs
enum ArcaneFortitudeTalents
{
    SPELL_ARCANE_FORTITUDE_RANK_1 = 28574,
    SPELL_ARCANE_FORTITUDE_RANK_2 = 54658,
    SPELL_ARCANE_FORTITUDE_RANK_3 = 54659
};

// Mana Shield spell family flags: [0] 0x00008000 [1] 0x00000000 [2] 0x00000008
enum ManaShieldFamilyFlags : uint32
{
    SPELL_FAMILY_FLAG_MANA_SHIELD_0 = 0x00008000,
    SPELL_FAMILY_FLAG_MANA_SHIELD_1 = 0x00000000,
    SPELL_FAMILY_FLAG_MANA_SHIELD_2 = 0x00000008
};

// Enhanced Mana Shield with Arcane Fortitude scaling
class spell_mage_arcane_fortitude_mana_shield : public AuraScript
{
    PrepareAuraScript(spell_mage_arcane_fortitude_mana_shield);

    void CalculateAmount(AuraEffect const* /*aurEff*/, int32& amount, bool& canBeRecalculated)
    {
        canBeRecalculated = false;
        if (Unit* caster = GetCaster())
        {
            // Base calculation from original Mana Shield (+80.53% from sp bonus)
            float bonus = 0.8053f;
            bonus *= caster->SpellBaseDamageBonusDone(GetSpellInfo()->GetSchoolMask());
            bonus *= caster->CalculateLevelPenalty(GetSpellInfo());

            // Check for Arcane Fortitude talent and add spell power scaling
            int32 arcaneFortitudeBonus = 0;
            if (AuraEffect const* arcaneFortitude = caster->GetAuraEffect(SPELL_ARCANE_FORTITUDE_RANK_3, EFFECT_0))
            {
                // Rank 3: 300% of spell power
                arcaneFortitudeBonus = caster->SpellBaseDamageBonusDone(GetSpellInfo()->GetSchoolMask()) * 3;
            }
            else if (AuraEffect const* arcaneFortitude = caster->GetAuraEffect(SPELL_ARCANE_FORTITUDE_RANK_2, EFFECT_0))
            {
                // Rank 2: 200% of spell power
                arcaneFortitudeBonus = caster->SpellBaseDamageBonusDone(GetSpellInfo()->GetSchoolMask()) * 2;
            }
            else if (AuraEffect const* arcaneFortitude = caster->GetAuraEffect(SPELL_ARCANE_FORTITUDE_RANK_1, EFFECT_0))
            {
                // Rank 1: 100% of spell power
                arcaneFortitudeBonus = caster->SpellBaseDamageBonusDone(GetSpellInfo()->GetSchoolMask());
            }

            // Apply level penalty to the Arcane Fortitude bonus as well
            if (arcaneFortitudeBonus > 0)
            {
                arcaneFortitudeBonus = int32(arcaneFortitudeBonus * caster->CalculateLevelPenalty(GetSpellInfo()));
            }

            amount += int32(bonus) + arcaneFortitudeBonus;

            // Debug logging
            if (arcaneFortitudeBonus > 0)
            {
                LOG_INFO("scripts", "Arcane Fortitude Mana Shield: Base amount: {}, SP bonus: {}, Arcane Fortitude bonus: {}, Total: {}", 
                    amount - int32(bonus) - arcaneFortitudeBonus, int32(bonus), arcaneFortitudeBonus, amount);
            }
        }
    }

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_ARCANE_FORTITUDE_RANK_1, SPELL_ARCANE_FORTITUDE_RANK_2, SPELL_ARCANE_FORTITUDE_RANK_3 });
    }

    void Register() override
    {
        DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_mage_arcane_fortitude_mana_shield::CalculateAmount, EFFECT_0, SPELL_AURA_MANA_SHIELD);
    }
};

// Spell script loader for Arcane Fortitude enhanced Mana Shield
class spell_mage_arcane_fortitude_mana_shield_loader : public SpellScriptLoader
{
public:
    spell_mage_arcane_fortitude_mana_shield_loader() : SpellScriptLoader("spell_mage_arcane_fortitude_mana_shield") { }

    AuraScript* GetAuraScript() const override
    {
        return new spell_mage_arcane_fortitude_mana_shield();
    }
};

void AddSC_arcane_fortitude_mana_shield()
{
    new spell_mage_arcane_fortitude_mana_shield_loader();
}
