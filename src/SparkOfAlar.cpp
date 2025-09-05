#include "ScriptMgr.h"
#include "Player.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "SpellScriptLoader.h"
#include "Unit.h"
#include "Log.h"

enum SparkOfAlarSpells
{
    EMBER_SCARS_DOT_ID = 200023,
    SPARK_OF_ALAR_COOLDOWN_DEBUFF = 200029,
    SPARK_OF_ALAR_HEAL_SPELL = 200030
};

class spell_mage_spark_of_alar : public AuraScript
{
    PrepareAuraScript(spell_mage_spark_of_alar);

public:
    spell_mage_spark_of_alar() : AuraScript() { }

    void CalculateAmount(AuraEffect const* /*aurEff*/, int32& amount, bool& /*canBeRecalculated*/)
    {
        // Set absorption amount to unlimited, the logic is in the Absorb hook
        amount = -1;
    }

private:
    void Absorb(AuraEffect* aurEff, DamageInfo& dmgInfo, uint32& absorbAmount)
    {
        Unit* victim = GetTarget();
        if (!victim->IsPlayer())
            return;

        LOG_ERROR("scripts", "Spark of Alar: Absorb hook triggered for player {}.", victim->GetName());
        LOG_ERROR("scripts", "Spark of Alar: Health: {}, Damage: {}", victim->GetHealth(), dmgInfo.GetDamage());

        int32 remainingHealth = int32(victim->GetHealth()) - int32(dmgInfo.GetDamage());
        bool isFatal = remainingHealth <= 0;
        bool hasCooldown = victim->HasAura(SPARK_OF_ALAR_COOLDOWN_DEBUFF);

        if (!isFatal)
        {
            LOG_ERROR("scripts", "Spark of Alar: Damage is not fatal. Resulting health would be {}.", remainingHealth);
            return;
        }

        if (hasCooldown)
        {
            LOG_ERROR("scripts", "Spark of Alar: Player has cooldown debuff.");
            return;
        }

        LOG_ERROR("scripts", "Spark of Alar: Conditions met. Activating effect.");

        // Absorb all the damage
        absorbAmount = dmgInfo.GetDamage();

        // Heal for 30% of max health
        int32 healAmount = victim->CountPctFromMaxHealth(30);
        victim->CastCustomSpell(SPARK_OF_ALAR_HEAL_SPELL, SPELLVALUE_BASE_POINT0, healAmount, victim, true, nullptr, aurEff);
        LOG_ERROR("scripts", "Spark of Alar: Healed for {}.", healAmount);

        // Apply cooldown debuff
        victim->CastSpell(victim, SPARK_OF_ALAR_COOLDOWN_DEBUFF, true);
        LOG_ERROR("scripts", "Spark of Alar: Applied cooldown debuff.");

        // Remove all stacks of Ember Scars
        if (victim->HasAura(EMBER_SCARS_DOT_ID))
        {
            victim->RemoveAurasDueToSpell(EMBER_SCARS_DOT_ID);
            LOG_ERROR("scripts", "Spark of Alar: Removed Ember Scars.");
        }
    }

    void Register() override
    {
        DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_mage_spark_of_alar::CalculateAmount, EFFECT_1, SPELL_AURA_SCHOOL_ABSORB);
        OnEffectAbsorb += AuraEffectAbsorbFn(spell_mage_spark_of_alar::Absorb, EFFECT_1);
    }
};

class spell_spark_of_alar_loader : public SpellScriptLoader
{
    public:
        spell_spark_of_alar_loader() : SpellScriptLoader("spell_spark_of_alar") { }

        AuraScript* GetAuraScript() const override
        {
            return new spell_mage_spark_of_alar();
        }
};

void AddSC_spark_of_alar()
{
    new spell_spark_of_alar_loader();
}
