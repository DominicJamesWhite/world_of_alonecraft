#include "ScriptMgr.h"
#include "Player.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "SpellScriptLoader.h"
#include "Unit.h"
#include "Log.h"

#define MOLTEN_ARMOR_SPELL_ID 43046
#define EMBER_SCARS_DOT_ID 200023
#define EMBER_SCARS_REMOVAL_CD 200024

class spell_molten_armor_damage_handler : public UnitScript
{
public:
    spell_molten_armor_damage_handler() : UnitScript("spell_molten_armor_damage_handler") { }

    void OnDamage(Unit* attacker, Unit* victim, uint32& damage) override
    {
        Player* player = victim->ToPlayer();
        if (!player)
            return;

        // Check if the damage is from Ember Scars
        if (attacker == player && player->HasAura(EMBER_SCARS_DOT_ID))
            return;

        // Check if player has Molten Armor active
        if (!player->HasAura(MOLTEN_ARMOR_SPELL_ID))
            return;
        
        LOG_INFO("module.moltenarmor", "Molten Armor OnDamage hook triggered for player {}. Damage: {}", player->GetName().c_str(), damage);

        // Split damage: 50% instant, 50% to DoT
        uint32 instantDamage = damage / 2;
        uint32 dotDamage = damage - instantDamage;

        // Apply only the instant portion
        damage = instantDamage;

        // Add the remaining damage to Ember Scars DoT
        AddOrUpdateEmberScars(player, dotDamage);
    }

private:
    void AddOrUpdateEmberScars(Player* player, uint32 damageToAdd)
    {
        LOG_INFO("module.moltenarmor", "Adding {} damage to Ember Scars for player {}", damageToAdd, player->GetName());
        Aura* emberScars = player->GetAura(EMBER_SCARS_DOT_ID);
        uint32 totalStoredDamage = 0;
        uint8 currentStacks = 0;

        if (emberScars)
        {
            // Get current stored damage from effect amount (we'll store total damage here)
            AuraEffect* effect = emberScars->GetEffect(EFFECT_0);
            if (effect)
            {
                // We're storing total damage * 100 to avoid float precision issues
                totalStoredDamage = effect->GetAmount() * 10;
                currentStacks = emberScars->GetStackAmount();
            }
        }

        // Add new damage
        totalStoredDamage += damageToAdd;

        // Calculate new stack count (max 5, but damage can exceed)
        uint8 newStacks = std::min<uint8>(currentStacks + 1, 5);

        if (!emberScars)
        {
            // Apply the DoT aura
            player->AddAura(EMBER_SCARS_DOT_ID, player);
            emberScars = player->GetAura(EMBER_SCARS_DOT_ID);
        }

        if (emberScars)
        {
            // Set stack amount
            emberScars->SetStackAmount(newStacks);

            // Calculate tick damage (10% of total damage per second)
            uint32 tickDamage = totalStoredDamage / 10;

            // Store total damage in the effect amount (divided by 10 to fit)
            AuraEffect* effect = emberScars->GetEffect(EFFECT_0);
            if (effect)
            {
                emberScars->RefreshDuration();
                effect->ChangeAmount(totalStoredDamage / 10);
                // Update the periodic damage
                effect->SetPeriodicTimer(1000); // 1 second intervals
            }
        }
    }
};

class spell_molten_armor_spell_cast_handler : public PlayerScript
{
public:
    spell_molten_armor_spell_cast_handler() : PlayerScript("spell_molten_armor_spell_cast_handler") { }

    void OnPlayerSpellCast(Player* player, Spell* spell, bool skipCheck) override
    {
        // Check if this is a fire spell critical hit
        if (!IsFireSpell(spell->GetSpellInfo()) || !player->HasAura(MOLTEN_ARMOR_SPELL_ID))
            return;

        LOG_INFO("module.moltenarmor", "Molten Armor OnPlayerSpellCast hook triggered for player {}. Spell: {}", player->GetName().c_str(), spell->GetSpellInfo()->SpellName[0]);

        // Check if it was a critical hit (simplified - you may need to adjust this)
        if (!roll_chance_f(player->GetFloatValue(PLAYER_CRIT_PERCENTAGE)))
            return;

        // Check 1-second cooldown
        if (player->HasSpellCooldown(EMBER_SCARS_REMOVAL_CD))
            return;

        // Remove stack from Ember Scars
        RemoveEmberScarsStack(player);

        // Apply 1-second cooldown
        player->AddSpellCooldown(EMBER_SCARS_REMOVAL_CD, 0, 1000);
    }

private:
    void RemoveEmberScarsStack(Player* player)
    {
        LOG_INFO("module.moltenarmor", "Removing Ember Scars stack for player {}", player->GetName().c_str());
        Aura* emberScars = player->GetAura(EMBER_SCARS_DOT_ID);
        if (!emberScars || emberScars->GetStackAmount() == 0)
            return;

        AuraEffect* effect = emberScars->GetEffect(EFFECT_0);
        if (!effect)
            return;

        // Get current total damage
        uint32 totalDamage = effect->GetAmount() * 10;

        // Remove 20% of damage (keep 80%)
        uint32 newTotalDamage = (totalDamage * 80) / 100;

        // Remove one stack
        uint8 newStacks = emberScars->GetStackAmount() - 1;

        if (newStacks == 0 || newTotalDamage == 0)
        {
            // Remove the aura completely
            emberScars->Remove();
            return;
        }

        // Update stack count
        emberScars->SetStackAmount(newStacks);

        // Update stored damage and tick damage
        uint32 newTickDamage = newTotalDamage / 10;
        effect->ChangeAmount(newTotalDamage / 10);
    }

    bool IsFireSpell(SpellInfo const* spell)
    {
        if (!spell)
            return false;

        // Check if spell school is fire
        return (spell->GetSchoolMask() & SPELL_SCHOOL_MASK_FIRE) != 0;
    }
};

// Spell script for Ember Scars to handle the DoT mechanics
class spell_ember_scars_AuraScript : public AuraScript
{
    PrepareAuraScript(spell_ember_scars_AuraScript);

    void OnPeriodicTick(AuraEffect const* aurEff)
    {
        Unit* target = GetTarget();
        Unit* caster = GetCaster();
        if (!target || !caster)
            return;

        // Get the stored total damage
        uint32 totalDamage = aurEff->GetAmount() * 10;
        
        // Calculate 10% damage for this tick
        uint32 tickDamage = totalDamage / 10;

        if (tickDamage > 0)
        {
            // Deal the tick damage
            Unit::DealDamage(caster, target, tickDamage, nullptr, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_FIRE, aurEff->GetSpellInfo(), false);
        }

        // Reduce total stored damage by the amount we just dealt
        uint32 remainingDamage = totalDamage - tickDamage;
        
        if (remainingDamage == 0)
        {
            // No damage left, remove the aura
            GetAura()->Remove();
            return;
        }

        // Update stored damage
        const_cast<AuraEffect*>(aurEff)->ChangeAmount(remainingDamage / 10);
    }

    void Register() override
    {
        OnEffectPeriodic += AuraEffectPeriodicFn(spell_ember_scars_AuraScript::OnPeriodicTick, EFFECT_0, SPELL_AURA_PERIODIC_DAMAGE);
    }
};

class spell_ember_scars_loader : public SpellScriptLoader
{
    public:
        spell_ember_scars_loader() : SpellScriptLoader("spell_ember_scars") { }

        AuraScript* GetAuraScript() const override
        {
            return new spell_ember_scars_AuraScript();
        }
};

// Registration
void AddSC_molten_armor_mechanic()
{
    new spell_molten_armor_damage_handler();
    new spell_molten_armor_spell_cast_handler();
    new spell_ember_scars_loader();
}
