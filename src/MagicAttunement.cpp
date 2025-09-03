#include "ScriptMgr.h"
#include "Player.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "SpellScriptLoader.h"
#include "Unit.h"
#include "Log.h"

// Magic Attunement talent IDs
enum MagicAttunementTalents
{
    MAGIC_ATTUNEMENT_RANK_1 = 11247,
    MAGIC_ATTUNEMENT_RANK_2 = 12606
};

// Invocation spell family flags: [0] 0x00002000 [1] 0x00000000 [2] 0x00000008
enum InvocationFamilyFlags : uint32
{
    SPELL_FAMILY_FLAG_INVOCATION_0 = 0x00002000,
    SPELL_FAMILY_FLAG_INVOCATION_1 = 0x00000000,
    SPELL_FAMILY_FLAG_INVOCATION_2 = 0x00000008
};

// Safe Magic Attunement implementation using core patterns
class spell_magic_attunement_handler : public PlayerScript
{
public:
    spell_magic_attunement_handler() : PlayerScript("spell_magic_attunement_handler") { }

    void OnPlayerSpellCast(Player* player, Spell* spell, bool skipCheck) override
    {
        if (!player || !spell)
            return;

        SpellInfo const* spellInfo = spell->GetSpellInfo();
        if (!spellInfo)
            return;

        // Use GetAuraEffectOfRankedSpell like core scripts for safe talent checking
        AuraEffect const* talentAurEff = player->GetAuraEffectOfRankedSpell(MAGIC_ATTUNEMENT_RANK_1, EFFECT_0);
        if (!talentAurEff)
            return;

        // Check for Invocation using GetAuraEffect (much safer than iteration)
        if (!HasInvocationActive(player))
            return;

        uint32 manaCost = spell->GetPowerCost();
        if (manaCost == 0)
            return;

        // Calculate mana return based on talent rank
        int32 manaReturnPercent = GetMagicAttunementPercent(player);
        if (manaReturnPercent == 0)
            return;

        int32 manaReturn = CalculatePct(manaCost, manaReturnPercent);
        if (manaReturn > 0)
        {
            player->ModifyPower(POWER_MANA, manaReturn);
            
            LOG_DEBUG("scripts", "Magic Attunement: Returned {} mana ({}% of {} cost) to player {}", 
                     manaReturn, manaReturnPercent, manaCost, player->GetName());
        }
    }

private:
    bool HasInvocationActive(Player* player)
    {
        if (!player)
            return false;

        // Use HasAura for simple spell family checking (much safer)
        // Check for common Invocation spells by ID instead of family flags
        return player->HasAura(604) || // Invocation
               player->HasAura(8450) || // Invocation Rank 2
               player->HasAura(8451) || // Invocation Rank 3
               player->HasAura(10173) ||   // Invocation Rank 4
               player->HasAura(10174) ||   // Invocation Rank 5
               player->HasAura(33944) ||   // Invocation Rank 6
               player->HasAura(43015);   // Invocation Rank 7
    }

    int32 GetMagicAttunementPercent(Player* player)
    {
        if (!player)
            return 0;

        // Use GetAuraEffectOfRankedSpell for safe talent checking
        if (AuraEffect const* aurEff = player->GetAuraEffectOfRankedSpell(MAGIC_ATTUNEMENT_RANK_2, EFFECT_0))
        {
            return 200; // Rank 2: 200% return
        }
        else if (AuraEffect const* aurEff = player->GetAuraEffectOfRankedSpell(MAGIC_ATTUNEMENT_RANK_1, EFFECT_0))
        {
            return 100; // Rank 1: 100% return
        }
        
        return 0; // No Magic Attunement talent
    }
};

// Magic Attunement talent aura scripts (passive talents)
class spell_magic_attunement_rank_1 : public AuraScript
{
    PrepareAuraScript(spell_magic_attunement_rank_1);

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ MAGIC_ATTUNEMENT_RANK_1 });
    }

    void Register() override
    {
        // This is a passive talent, the actual effect is handled by the PlayerScript
    }
};

class spell_magic_attunement_rank_2 : public AuraScript
{
    PrepareAuraScript(spell_magic_attunement_rank_2);

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ MAGIC_ATTUNEMENT_RANK_2 });
    }

    void Register() override
    {
        // This is a passive talent, the actual effect is handled by the PlayerScript
    }
};

// Spell script loaders
class spell_magic_attunement_rank_1_loader : public SpellScriptLoader
{
public:
    spell_magic_attunement_rank_1_loader() : SpellScriptLoader("spell_magic_attunement_rank_1") { }

    AuraScript* GetAuraScript() const override
    {
        return new spell_magic_attunement_rank_1();
    }
};

class spell_magic_attunement_rank_2_loader : public SpellScriptLoader
{
public:
    spell_magic_attunement_rank_2_loader() : SpellScriptLoader("spell_magic_attunement_rank_2") { }

    AuraScript* GetAuraScript() const override
    {
        return new spell_magic_attunement_rank_2();
    }
};

// Registration
void AddSC_magic_attunement()
{
    new spell_magic_attunement_handler();
    new spell_magic_attunement_rank_1_loader();
    new spell_magic_attunement_rank_2_loader();
}
