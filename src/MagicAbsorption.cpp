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

// Mana Shield spell family flags: [0] 0x00008000 [1] 0x00000000 [2] 0x00000008
enum ManaShieldFamilyFlags : uint32
{
    SPELL_FAMILY_FLAG_MANA_SHIELD_0 = 0x00008000,
    SPELL_FAMILY_FLAG_MANA_SHIELD_1 = 0x00000000,
    SPELL_FAMILY_FLAG_MANA_SHIELD_2 = 0x00000008
};

class spell_magic_absorption_damage_handler : public UnitScript
{
public:
    spell_magic_absorption_damage_handler() : UnitScript("spell_magic_absorption_damage_handler") 
    { 
        LOG_ERROR("scripts", "Magic Absorption: UnitScript constructor called - script is loading!");
    }

    void OnDamage(Unit* attacker, Unit* victim, uint32& damage) override
    {
        Player* player = victim->ToPlayer();
        if (!player)
            return;

        // Check if player has Mana Shield active using spell family flags
        bool hasManaShield = HasManaShield(player);

        
        if (!hasManaShield)
            return;

        // Check if player has Magic Absorption talent
        int32 damageReductionPercent = GetMagicAbsorptionReduction(player);
        
        if (damageReductionPercent == 0)
            return;

        // Get player's spell critical strike chance (like warlock spells do)
        float spellCritChance = player->GetFloatValue(PLAYER_SPELL_CRIT_PERCENTAGE1 + static_cast<uint8>(SPELL_SCHOOL_ARCANE));
        float meleeCritChance = player->GetFloatValue(PLAYER_CRIT_PERCENTAGE);
        

        
        // Use spell crit chance for Magic Absorption proc
        float critChance = spellCritChance;
        
        // Roll for Magic Absorption proc
        bool procced = roll_chance_f(critChance);

        
        if (!procced)
            return;

        // Apply damage reduction
        uint32 originalDamage = damage;
        damage = (damage * damageReductionPercent) / 100;
        
        uint32 reducedAmount = originalDamage - damage;
        
    }

private:
    bool HasManaShield(Player* player)
    {
        // Check for any Mana Shield aura using spell family flags
        Unit::AuraApplicationMap const& auras = player->GetAppliedAuras();
    
        
        for (auto const& auraPair : auras)
        {
            Aura const* aura = auraPair.second->GetBase();
            SpellInfo const* spellInfo = aura->GetSpellInfo();
            
            if (spellInfo->SpellFamilyName == SPELLFAMILY_MAGE &&
                spellInfo->SpellFamilyFlags[0] & SPELL_FAMILY_FLAG_MANA_SHIELD_0 &&
                spellInfo->SpellFamilyFlags[1] == SPELL_FAMILY_FLAG_MANA_SHIELD_1 &&
                spellInfo->SpellFamilyFlags[2] & SPELL_FAMILY_FLAG_MANA_SHIELD_2)
            {
                LOG_INFO("scripts", "Magic Absorption Debug: Found Mana Shield spell {} (ID: {})", 
                    spellInfo->SpellName[0], spellInfo->Id);
                return true;
            }
        }
        return false;
    }

    int32 GetMagicAbsorptionReduction(Player* player)
    {
        // Check for Magic Absorption talents
        if (player->HasAura(SPELL_MAGIC_ABSORPTION_RANK_2))
        {
            return 50; // Rank 2: 50% damage
        }
        else if (player->HasAura(SPELL_MAGIC_ABSORPTION_RANK_1))
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
    new spell_magic_absorption_damage_handler();
    new spell_magic_absorption_rank_1_loader();
    new spell_magic_absorption_rank_2_loader();
}
