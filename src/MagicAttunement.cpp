#include "ScriptMgr.h"
#include "Player.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "SpellScriptLoader.h"
#include "Unit.h"
#include "Log.h"

#define MAGIC_ATTUNEMENT_RANK_1 11247
#define MAGIC_ATTUNEMENT_RANK_2 12606

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

        // Check if player has Magic Attunement talent
        uint8 talentRank = GetMagicAttunementRank(player);
        if (talentRank == 0)
            return;

        // Check if Invocation is active
        if (!HasInvocationActive(player))
            return;

        // Check if this spell has a mana cost
        uint32 manaCost = spell->GetPowerCost();
        if (manaCost == 0)
            return;

        // Calculate mana return based on talent rank
        uint32 manaReturn = 0;
        if (talentRank == 1)
        {
            manaReturn = manaCost; // 100% return
        }
        else if (talentRank == 2)
        {
            manaReturn = manaCost * 2; // 200% return
        }

        if (manaReturn > 0)
        {
            // Return mana to the player
            player->ModifyPower(POWER_MANA, manaReturn);
        }
    }

private:
    uint8 GetMagicAttunementRank(Player* player)
    {
        if (player->HasTalent(MAGIC_ATTUNEMENT_RANK_2, player->GetActiveSpec()))
            return 2;
        else if (player->HasTalent(MAGIC_ATTUNEMENT_RANK_1, player->GetActiveSpec()))
            return 1;
        return 0;
    }

    bool HasInvocationActive(Player* player)
    {
        // Check all auras on the player for Invocation family flag
        Unit::AuraApplicationMap const& auras = player->GetAppliedAuras();
        for (auto const& auraPair : auras)
        {
            AuraApplication const* aurApp = auraPair.second;
            if (!aurApp)
                continue;

            Aura const* aura = aurApp->GetBase();
            if (!aura)
                continue;

            SpellInfo const* spellInfo = aura->GetSpellInfo();
            if (!spellInfo)
                continue;

            // Check if this spell matches Invocation family flag
            // Family flag: [0] 0x00002000 [1] 0x00000000 [2] 0x00000008
            if (spellInfo->SpellFamilyFlags[0] & 0x00002000 && 
                spellInfo->SpellFamilyFlags[2] & 0x00000008)
            {
                return true;
            }
        }
        return false;
    }
};

// Registration
void AddSC_magic_attunement()
{
    new spell_magic_attunement_handler();
}
