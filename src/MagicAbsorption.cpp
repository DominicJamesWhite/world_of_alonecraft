#include "ScriptMgr.h"
#include "Player.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "SpellScriptLoader.h"
#include "Unit.h"
#include "Log.h"

// Magic Absorption talent IDs
enum MagicAbsorptionTalents
{
    SPELL_MAGIC_ABSORPTION_RANK_1 = 29441,  // 75% damage reduction
    SPELL_MAGIC_ABSORPTION_RANK_2 = 29444   // 50% damage reduction
};

// Safe Magic Absorption implementation using UnitScript (with proper safety checks)
class spell_magic_absorption_handler : public UnitScript
{
public:
    spell_magic_absorption_handler() : UnitScript("spell_magic_absorption_handler") { }

    void OnDamage(Unit* attacker, Unit* victim, uint32& damage) override
    {
        if (!victim || damage == 0)
            return;

        Player* player = victim->ToPlayer();
        if (!player)
            return;

        // Check if player has Magic Absorption talent (safe method)
        AuraEffect const* talentAurEff = player->GetAuraEffectOfRankedSpell(SPELL_MAGIC_ABSORPTION_RANK_1, EFFECT_0);
        if (!talentAurEff)
            return;

        // Check if player has Mana Shield active (safe method using HasAura)
        if (!player->HasAura(1463) &&   // Mana Shield Rank 1
            !player->HasAura(8494) &&   // Mana Shield Rank 2
            !player->HasAura(8495) &&   // Mana Shield Rank 3
            !player->HasAura(10191) &&  // Mana Shield Rank 4
            !player->HasAura(10192) &&  // Mana Shield Rank 5
            !player->HasAura(10193) &&  // Mana Shield Rank 6
            !player->HasAura(27131) &&  // Mana Shield Rank 7
            !player->HasAura(43019) &&  // Mana Shield Rank 8
            !player->HasAura(43020))    // Mana Shield Rank 9
            return;

        // Get spell crit chance safely
        float critChance = player->GetFloatValue(PLAYER_SPELL_CRIT_PERCENTAGE1 + static_cast<uint8>(SPELL_SCHOOL_ARCANE));
        
        if (!roll_chance_f(critChance))
            return;

        // Get damage reduction based on talent rank
        int32 reductionPercent = GetMagicAbsorptionReduction(player);
        if (reductionPercent == 0)
            return;

        // Apply damage reduction
        uint32 originalDamage = damage;
        damage = (damage * reductionPercent) / 100;
        uint32 reducedAmount = originalDamage - damage;

        LOG_DEBUG("scripts", "Magic Absorption: Reduced damage from {} to {} ({}% reduction)", 
                 originalDamage, damage, 100 - reductionPercent);
    }

private:
    int32 GetMagicAbsorptionReduction(Player* player)
    {
        if (!player)
            return 0;

        // Use GetAuraEffectOfRankedSpell for safe talent checking
        if (AuraEffect const* aurEff = player->GetAuraEffectOfRankedSpell(SPELL_MAGIC_ABSORPTION_RANK_2, EFFECT_0))
        {
            return 50; // Rank 2: 50% damage
        }
        else if (AuraEffect const* aurEff = player->GetAuraEffectOfRankedSpell(SPELL_MAGIC_ABSORPTION_RANK_1, EFFECT_0))
        {
            return 75; // Rank 1: 75% damage
        }
        
        return 0; // No Magic Absorption talent
    }
};

// Enhanced Magic Absorption talents with proper descriptions
class spell_magic_absorption_rank_1 : public AuraScript
{
    PrepareAuraScript(spell_magic_absorption_rank_1);

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_MAGIC_ABSORPTION_RANK_1 });
    }

    void Register() override
    {
        // This is a passive talent, the actual effect is handled by the UnitScript
    }
};

class spell_magic_absorption_rank_2 : public AuraScript
{
    PrepareAuraScript(spell_magic_absorption_rank_2);

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_MAGIC_ABSORPTION_RANK_2 });
    }

    void Register() override
    {
        // This is a passive talent, the actual effect is handled by the UnitScript
    }
};

// Spell script loaders
class spell_magic_absorption_rank_1_loader : public SpellScriptLoader
{
public:
    spell_magic_absorption_rank_1_loader() : SpellScriptLoader("spell_magic_absorption_rank_1") { }

    AuraScript* GetAuraScript() const override
    {
        return new spell_magic_absorption_rank_1();
    }
};

class spell_magic_absorption_rank_2_loader : public SpellScriptLoader
{
public:
    spell_magic_absorption_rank_2_loader() : SpellScriptLoader("spell_magic_absorption_rank_2") { }

    AuraScript* GetAuraScript() const override
    {
        return new spell_magic_absorption_rank_2();
    }
};

// Registration
void AddSC_magic_absorption()
{
    new spell_magic_absorption_handler();
    new spell_magic_absorption_rank_1_loader();
    new spell_magic_absorption_rank_2_loader();
}
