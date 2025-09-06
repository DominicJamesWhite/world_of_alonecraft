#include "ScriptMgr.h"
#include "Player.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "SpellScriptLoader.h"
#include "Unit.h"
#include "Log.h"
#include "Spell.h"

enum AblativeArmorSpells
{
    SPELL_ABLATIVE_ARMOR_R1 = 11160,
    SPELL_ABLATIVE_ARMOR_R2 = 12518,
    SPELL_ABLATIVE_ARMOR_R3 = 12519
};

// Spell family flags for Frost/Ice Armor
// Both Ice Armor and Frost Armor use the same family flag: 0x02080000
#define FROST_ICE_ARMOR_FAMILY_FLAG 0x02080000

// Enhanced Frost/Ice Armor with spellpower scaling
class spell_frost_ice_armor_enhanced : public AuraScript
{
    PrepareAuraScript(spell_frost_ice_armor_enhanced);

private:
    void UpdateAblativeArmor(Unit* caster)
    {
        if (!caster)
            return;

        Aura* ablativeAura = nullptr;
        if (caster->HasAura(SPELL_ABLATIVE_ARMOR_R3))
            ablativeAura = caster->GetAura(SPELL_ABLATIVE_ARMOR_R3);
        else if (caster->HasAura(SPELL_ABLATIVE_ARMOR_R2))
            ablativeAura = caster->GetAura(SPELL_ABLATIVE_ARMOR_R2);
        else if (caster->HasAura(SPELL_ABLATIVE_ARMOR_R1))
            ablativeAura = caster->GetAura(SPELL_ABLATIVE_ARMOR_R1);

        if (ablativeAura)
        {
            if (AuraEffect* eff = ablativeAura->GetEffect(EFFECT_1))
            {
                eff->RecalculateAmount();
            }
        }
    }

public:
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

    void AfterApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        UpdateAblativeArmor(GetCaster());
    }

    void AfterRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        UpdateAblativeArmor(GetCaster());
    }

    void Register() override
    {
        // Apply spellpower scaling to the armor bonus effect
        DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_frost_ice_armor_enhanced::CalculateAmount, EFFECT_0, SPELL_AURA_MOD_RESISTANCE);
        AfterEffectApply += AuraEffectApplyFn(spell_frost_ice_armor_enhanced::AfterApply, EFFECT_ALL, SPELL_AURA_ANY, AURA_EFFECT_HANDLE_REAL);
        AfterEffectRemove += AuraEffectRemoveFn(spell_frost_ice_armor_enhanced::AfterRemove, EFFECT_ALL, SPELL_AURA_ANY, AURA_EFFECT_HANDLE_REAL);
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
