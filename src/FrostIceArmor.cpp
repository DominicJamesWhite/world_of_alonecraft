#include "ScriptMgr.h"
#include "Player.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "SpellScriptLoader.h"
#include "Unit.h"
#include "Log.h"

// Spell family flags for Frost/Ice Armor
// Both Ice Armor and Frost Armor use the same family flag: 0x02080000
#define FROST_ICE_ARMOR_FAMILY_FLAG 0x02080000

// Enhanced Frost/Ice Armor with spellpower scaling
class spell_frost_ice_armor_enhanced : public AuraScript
{
    PrepareAuraScript(spell_frost_ice_armor_enhanced);

    bool Validate(SpellInfo const* spellInfo) override
    {
        // Validate that this is a Frost/Ice Armor spell by checking family flags
        return spellInfo->SpellFamilyName == SPELLFAMILY_MAGE && 
               (spellInfo->SpellFamilyFlags[0] & FROST_ICE_ARMOR_FAMILY_FLAG);
    }

    void CalculateAmount(AuraEffect const* /*aurEff*/, int32& amount, bool& canBeRecalculated)
    {
        canBeRecalculated = false;
        if (Unit* caster = GetCaster())
        {
            // Base spellpower scaling coefficient for Frost/Ice Armor
            // Using similar scaling to Fire/Frost Ward (+80.68% from sp bonus)
            float bonus = 0.8068f;

            // Frozen Core talent check
            if (caster->HasAura(31669)) // Rank 3
            {
                bonus += 4.50f; // +450%
            }
            else if (caster->HasAura(31668)) // Rank 2
            {
                bonus += 3.00f; // +300%
            }
            else if (caster->HasAura(31667)) // Rank 1
            {
                bonus += 1.50f; // +150%
            }

            bonus *= caster->SpellBaseDamageBonusDone(GetSpellInfo()->GetSchoolMask());
            bonus *= caster->CalculateLevelPenalty(GetSpellInfo());

            amount += int32(bonus);
        }
    }

    void Register() override
    {
        // Apply spellpower scaling to the armor bonus effect
        DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_frost_ice_armor_enhanced::CalculateAmount, EFFECT_0, SPELL_AURA_MOD_RESISTANCE);
    }
};

// Spell script loader
class spell_frost_ice_armor_enhanced_loader : public SpellScriptLoader
{
public:
    spell_frost_ice_armor_enhanced_loader() : SpellScriptLoader("spell_frost_ice_armor_enhanced") { }

    AuraScript* GetAuraScript() const override
    {
        return new spell_frost_ice_armor_enhanced();
    }
};

// Registration function
void AddSC_frost_ice_armor_enhanced()
{
    new spell_frost_ice_armor_enhanced_loader();
}
