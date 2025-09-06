#include "Player.h"
#include "ScriptMgr.h"
#include "Spell.h"
#include "SpellAuraEffects.h"
#include "SpellScript.h"
#include "Log.h"
#include "Unit.h"
#include "SpellScriptLoader.h"
#include "Spell.h"

enum AblativeArmorSpells
{
    SPELL_ABLATIVE_ARMOR_R1 = 11160,
    SPELL_ABLATIVE_ARMOR_R2 = 12518,
    SPELL_ABLATIVE_ARMOR_R3 = 12519
};

#define FROST_ICE_ARMOR_FAMILY_FLAG 0x02080000

class spell_mage_ablative_armor : public AuraScript
{
    PrepareAuraScript(spell_mage_ablative_armor);

public:
    bool HasFrostOrIceArmor(Unit* caster) const
    {
        for (auto const& auraPair : caster->GetAppliedAuras())
        {
            if (Aura* aura = auraPair.second->GetBase())
            {
                if (SpellInfo const* spellInfo = aura->GetSpellInfo())
                {
                    if (spellInfo->SpellFamilyName == SPELLFAMILY_MAGE &&
                        (spellInfo->SpellFamilyFlags[0] & FROST_ICE_ARMOR_FAMILY_FLAG))
                    {
                        return true;
                    }
                }
            }
        }
        return false;
    }

    void CalculateAmount(AuraEffect const* aurEff, int32& amount, bool& canBeRecalculated)
    {
        canBeRecalculated = true;
        amount = 0;

        if (Unit* caster = GetCaster())
        {
            if (HasFrostOrIceArmor(caster))
            {
                int32 intellect = caster->GetStat(STAT_INTELLECT);
                int32 spirit = caster->GetStat(STAT_SPIRIT);

                uint32 spellId = GetSpellInfo()->Id;
                float pct = 0.0f;
                if (spellId == SPELL_ABLATIVE_ARMOR_R1)
                    pct = 0.33f;
                else if (spellId == SPELL_ABLATIVE_ARMOR_R2)
                    pct = 0.66f;
                else if (spellId == SPELL_ABLATIVE_ARMOR_R3)
                    pct = 1.0f;

                if (pct > 0.0f)
                {
                    amount = (intellect + spirit) * pct;
                }
            }
        }
    }

    void Register() override
    {
        DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_mage_ablative_armor::CalculateAmount, EFFECT_1, SPELL_AURA_MOD_STAT);
    }
};

class spell_mage_ablative_armor_loader : public SpellScriptLoader
{
public:
    spell_mage_ablative_armor_loader() : SpellScriptLoader("spell_mage_ablative_armor") { }

    AuraScript* GetAuraScript() const override
    {
        return new spell_mage_ablative_armor();
    }
};

void AddSC_spell_mage_ablative_armor()
{
    new spell_mage_ablative_armor_loader();
}
