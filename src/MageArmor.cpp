#include "Player.h"
#include "ScriptMgr.h"
#include "SpellAuraEffects.h"
#include "SpellScript.h"
#include "Unit.h"
#include "SpellMgr.h"
#include "SpellScriptLoader.h"

class spell_mage_mage_armor_custom : public AuraScript
{
    PrepareAuraScript(spell_mage_mage_armor_custom);

    void HandleEffectCalc(AuraEffect const* aurEff, int32& amount, bool& canBeRecalculated)
    {
        Unit* caster = GetCaster();
        if (!caster || caster->GetTypeId() != TYPEID_PLAYER)
            return;

        Player* mage = caster->ToPlayer();
        if (!mage)
            return;

        // Grab mage stats
        float intellect = static_cast<float>(mage->GetStat(STAT_INTELLECT));
        float spirit    = static_cast<float>(mage->GetStat(STAT_SPIRIT));
        float level     = static_cast<float>(mage->GetLevel());

        // Calculate miss chance
        float missChance = (intellect * 2 + spirit * 2) / std::max(level, 1.0f);

        // Apply dynamically
        amount = static_cast<int32>(missChance * -1);
    }

    void Register() override
    {
        DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_mage_mage_armor_custom::HandleEffectCalc, EFFECT_2, SPELL_AURA_MOD_ATTACKER_MELEE_HIT_CHANCE);
    }
};

class spell_mage_mage_armor_custom_loader : public SpellScriptLoader
{
public:
    spell_mage_mage_armor_custom_loader() : SpellScriptLoader("spell_mage_mage_armor_custom") { }

    AuraScript* GetAuraScript() const override
    {
        return new spell_mage_mage_armor_custom();
    }
};

void AddSC_spell_mage_mage_armor_custom()
{
    new spell_mage_mage_armor_custom_loader();
}
