#include "ScriptMgr.h"
#include "Player.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "SpellScriptLoader.h"
#include "Unit.h"
#include "Log.h"

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
        if (!(player->HasAura(30482) || player->HasAura(43045) || player->HasAura(43046)))
            return;

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
        Aura* emberScars = player->GetAura(EMBER_SCARS_DOT_ID);
        uint32 currentTickDamage = 0;

        if (emberScars)
        {
            if (AuraEffect* effect = emberScars->GetEffect(EFFECT_0))
            {
                currentTickDamage = effect->GetAmount();
            }
        }

        // Calculate new tick damage to add (damage is over 10 seconds)
        uint32 newTickDamage = damageToAdd / 10;
        uint32 totalTickDamage = currentTickDamage + newTickDamage;

        if (!emberScars)
        {
            // Apply the DoT aura and get it
            player->AddAura(EMBER_SCARS_DOT_ID, player);
            emberScars = player->GetAura(EMBER_SCARS_DOT_ID);
        }

        if (emberScars)
        {
            uint8 newStacks = std::min<uint8>(emberScars->GetStackAmount() + 1, 5);
            emberScars->SetStackAmount(newStacks);
            emberScars->RefreshDuration();

            if (AuraEffect* effect = emberScars->GetEffect(EFFECT_0))
            {
                effect->ChangeAmount(totalTickDamage);
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
        if (!IsFireSpell(spell->GetSpellInfo()) || !(player->HasAura(30482) || player->HasAura(43045) || player->HasAura(43046)))
            return;

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
        Aura* emberScars = player->GetAura(EMBER_SCARS_DOT_ID);
        if (!emberScars || emberScars->GetStackAmount() == 0)
            return;

        AuraEffect* effect = emberScars->GetEffect(EFFECT_0);
        if (!effect)
            return;

        uint32 currentTickDamage = effect->GetAmount();
        uint8 currentStacks = emberScars->GetStackAmount();

        // Check for specific auras to remove 2 stacks
        bool hasSpecialAura = player->HasAura(11083) || player->HasAura(12351);
        uint8 stacksToRemove = hasSpecialAura ? 2 : 1;
        uint32 damageReductionPercent = hasSpecialAura ? 40 : 20;

        // Ensure we don't remove more stacks than available
        if (currentStacks < stacksToRemove)
        {
            stacksToRemove = currentStacks;
            damageReductionPercent = stacksToRemove * 20;
        }

        // Reduce tick damage
        uint32 newTickDamage = (currentTickDamage * (100 - damageReductionPercent)) / 100;

        // Remove stacks
        uint8 newStacks = currentStacks - stacksToRemove;

        if (newStacks == 0 || newTickDamage == 0)
        {
            emberScars->Remove();
            return;
        }

        emberScars->SetStackAmount(newStacks);
        effect->ChangeAmount(newTickDamage);
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

        // The amount is the tick damage
        uint32 tickDamage = aurEff->GetAmount();

        if (tickDamage > 0)
        {
            // Deal the tick damage
            Unit::DealDamage(caster, target, tickDamage, nullptr, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_FIRE, aurEff->GetSpellInfo(), false);
        }
        else
        {
            // No damage per tick, remove the aura
            GetAura()->Remove();
        }
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
