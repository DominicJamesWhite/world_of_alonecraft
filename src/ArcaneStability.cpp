#include "ScriptMgr.h"
#include "Player.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "SpellScriptLoader.h"
#include "SpellMgr.h"
#include "Unit.h"
#include "Log.h"

// Arcane Stability talent IDs
#define ARCANE_STABILITY_RANK_1 11237
#define ARCANE_STABILITY_RANK_2 12463
#define ARCANE_STABILITY_RANK_3 12464
#define ARCANE_STABILITY_RANK_4 16769
#define ARCANE_STABILITY_RANK_5 16770

// Spell family flags for Arcane Missiles: [0] 0x00000800 [1] 0x00000000 [2] 0x00000000
// Spell family flags for Invocation: [0] 0x00002000 [1] 0x00000000 [2] 0x00000008

class spell_arcane_stability_proc : public PlayerScript
{
public:
    spell_arcane_stability_proc() : PlayerScript("spell_arcane_stability_proc") { }

    void OnPlayerSpellCast(Player* player, Spell* spell, bool skipCheck) override
    {
        if (!spell || !spell->GetSpellInfo())
            return;

        // Check if this is Arcane Missiles
        if (!IsArcaneMissiles(spell->GetSpellInfo()))
            return;

        // Get Arcane Stability talent rank
        uint8 talentRank = GetArcaneStabilityRank(player);
        if (talentRank == 0)
            return;

        // Calculate proc chance based on rank (4/8/12/16/20%)
        uint32 procChance = talentRank * 4;

        // Roll for proc
        if (!roll_chance_i(procChance))
            return;

        // Reset cooldown on Invocation spells
        ResetInvocationCooldowns(player);
    }

private:
    bool IsArcaneMissiles(SpellInfo const* spellInfo)
    {
        if (!spellInfo)
            return false;

        // Check spell family and flags for Arcane Missiles
        return (spellInfo->SpellFamilyName == SPELLFAMILY_MAGE &&
                spellInfo->SpellFamilyFlags[0] & 0x00000800 &&
                spellInfo->SpellFamilyFlags[1] == 0x00000000 &&
                spellInfo->SpellFamilyFlags[2] == 0x00000000);
    }

    bool IsInvocation(SpellInfo const* spellInfo)
    {
        if (!spellInfo)
            return false;

        // Check spell family and flags for Invocation
        return (spellInfo->SpellFamilyName == SPELLFAMILY_MAGE &&
                spellInfo->SpellFamilyFlags[0] & 0x00002000 &&
                spellInfo->SpellFamilyFlags[1] == 0x00000000 &&
                spellInfo->SpellFamilyFlags[2] & 0x00000008);
    }

    uint8 GetArcaneStabilityRank(Player* player)
    {
        if (player->HasAura(ARCANE_STABILITY_RANK_5))
            return 5;
        if (player->HasAura(ARCANE_STABILITY_RANK_4))
            return 4;
        if (player->HasAura(ARCANE_STABILITY_RANK_3))
            return 3;
        if (player->HasAura(ARCANE_STABILITY_RANK_2))
            return 2;
        if (player->HasAura(ARCANE_STABILITY_RANK_1))
            return 1;
        return 0;
    }

    void ResetInvocationCooldowns(Player* player)
    {
        // Get all known spells and check for Invocation spells
        PlayerSpellMap const& spellMap = player->GetSpellMap();
        for (PlayerSpellMap::const_iterator itr = spellMap.begin(); itr != spellMap.end(); ++itr)
        {
            SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(itr->first);
            if (!spellInfo)
                continue;

            // Check if this is an Invocation spell
            if (IsInvocation(spellInfo))
            {
                // Check if spell is on cooldown
                SpellCooldowns::iterator citr = player->GetSpellCooldownMap().find(spellInfo->Id);
                if (citr != player->GetSpellCooldownMap().end())
                {
                    // Remove the cooldown
                    if (citr->second.needSendToClient)
                        player->RemoveSpellCooldown(spellInfo->Id, true);
                    else
                        player->RemoveSpellCooldown(spellInfo->Id, false);

                    LOG_INFO("scripts", "Arcane Stability proc: Reset cooldown for spell {} ({})", 
                             spellInfo->Id, spellInfo->SpellName[0]);
                }
            }
        }
    }
};

// Registration
void AddSC_arcane_stability_mechanic()
{
    new spell_arcane_stability_proc();
}
