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

        // Ignore DoT-originated damage to avoid recursion
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
        // Calculate how much of the existing DoT damage remains (so we don't reapply damage already ticked)
        Aura* emberScars = player->GetAura(EMBER_SCARS_DOT_ID);
        uint32 currentTickDamage = 0;

        // Duration (in seconds) that each damage contribution is spread across
        const uint32 dotDurationSeconds = 10;

        // Total remaining (not per-tick) damage from the existing aura
        uint64 oldRemainingTotal = 0;
        uint32 remainingSec = 0;

        bool newlyApplied = false;

        if (emberScars)
        {
            if (AuraEffect* effect = emberScars->GetEffect(EFFECT_0))
            {
                currentTickDamage = effect->GetAmount();
            }

            // Get remaining duration (milliseconds) and convert to seconds
            int32 remainingMs = emberScars->GetDuration();
            if (remainingMs > 0)
            {
                remainingSec = static_cast<uint32>(remainingMs / 1000);
                if (remainingSec > dotDurationSeconds)
                    remainingSec = dotDurationSeconds;

                // remaining total damage = per-tick * remaining seconds
                oldRemainingTotal = static_cast<uint64>(currentTickDamage) * remainingSec;
            }
        }

        // New damage contribution (total, not per-tick)
        uint32 newDamageTotal = damageToAdd;

        // Ensure the aura exists so we can update it
        if (!emberScars)
        {
            player->AddAura(EMBER_SCARS_DOT_ID, player);
            emberScars = player->GetAura(EMBER_SCARS_DOT_ID);
            newlyApplied = true;
        }

        if (emberScars)
        {
            // Combine remaining old damage with the new damage, then redistribute across full duration
            uint64 combinedTotalDamage = oldRemainingTotal + static_cast<uint64>(newDamageTotal);

            // Compute new per-tick damage (integer division)
            uint32 newTickDamage = static_cast<uint32>(combinedTotalDamage / dotDurationSeconds);

            uint8 newStacks;
            if (newlyApplied)
                newStacks = 1;
            else
                newStacks = std::min<uint8>(emberScars->GetStackAmount() + 1, 5);

            emberScars->SetStackAmount(newStacks);

            // Refresh to full duration after recomputing per-tick
            emberScars->RefreshDuration();

            if (AuraEffect* effect = emberScars->GetEffect(EFFECT_0))
            {
                effect->ChangeAmount(newTickDamage);
            }
        }
    }
};

/*
 * Helper to remove Ember Scars stacks from a player.
 * Extracted so multiple handlers (player-script or trigger-aura) can call it.
 */
static void RemoveEmberScarsStackFromPlayer(Player* player)
{
    if (!player)
        return;

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

/*
 * Listener for the crit-trigger aura (spell ID 200040).
 * When this aura is applied to a player (applied by your external proc on real crit),
 * remove the correct amount of Ember Scars stacks and then remove the trigger aura itself.
 */
class spell_molten_armor_crit_aura_AuraScript : public AuraScript
{
    PrepareAuraScript(spell_molten_armor_crit_aura_AuraScript);

    void OnApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        Unit* target = GetTarget();
        if (!target)
            return;

        Player* player = target->ToPlayer();
        if (!player)
            return;

        // Only react if player currently has Molten Armor active
        if (!(player->HasAura(30482) || player->HasAura(43045) || player->HasAura(43046)))
            return;

        // Remove the appropriate Ember Scars stacks
        RemoveEmberScarsStackFromPlayer(player);

        // Remove the trigger aura (200040) immediately
        if (Aura* trigger = player->GetAura(200040))
            trigger->Remove();
    }

    void Register() override
    {
        // Trigger when the aura is actually applied
        AfterEffectApply += AuraEffectApplyFn(spell_molten_armor_crit_aura_AuraScript::OnApply, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
    }
};

class spell_molten_armor_crit_aura_loader : public SpellScriptLoader
{
public:
    spell_molten_armor_crit_aura_loader() : SpellScriptLoader("spell_molten_armor_crit_aura") { }

    AuraScript* GetAuraScript() const override
    {
        return new spell_molten_armor_crit_aura_AuraScript();
    }
};

/*
 * AuraScript to apply a lightweight tracker aura (200041) while Molten Armor is present.
 * DB: map molten armor spell IDs (e.g. 30482, 43045, 43046) to the script name "spell_molten_armor_apply"
 * so this loader runs when molten armor is applied/reapplied.
 */
class spell_molten_armor_apply_AuraScript : public AuraScript
{
    PrepareAuraScript(spell_molten_armor_apply_AuraScript);

    void OnApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        Unit* target = GetTarget();
        if (!target)
            return;

        Player* player = target->ToPlayer();
        if (!player)
            return;

        // Avoid adding duplicate tracker aura
        if (!player->HasAura(200041))
            player->AddAura(200041, player);
    }

    void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        Unit* target = GetTarget();
        if (!target)
            return;

        Player* player = target->ToPlayer();
        if (!player)
            return;

        if (player->HasAura(200041))
            player->RemoveAura(200041);
    }

    void Register() override
    {
        AfterEffectApply += AuraEffectApplyFn(spell_molten_armor_apply_AuraScript::OnApply, EFFECT_ALL, SPELL_AURA_ANY, AURA_EFFECT_HANDLE_REAL);
        AfterEffectRemove += AuraEffectRemoveFn(spell_molten_armor_apply_AuraScript::OnRemove, EFFECT_ALL, SPELL_AURA_ANY, AURA_EFFECT_HANDLE_REAL);
    }
};

class spell_molten_armor_apply_loader : public SpellScriptLoader
{
public:
    spell_molten_armor_apply_loader() : SpellScriptLoader("spell_molten_armor_apply") { }

    AuraScript* GetAuraScript() const override
    {
        return new spell_molten_armor_apply_AuraScript();
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
    new spell_molten_armor_crit_aura_loader();
    new spell_molten_armor_apply_loader();
    new spell_ember_scars_loader();
}
