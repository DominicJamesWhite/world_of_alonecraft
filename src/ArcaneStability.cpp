#include "ScriptMgr.h"
#include "Player.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "SpellScriptLoader.h"
#include "SpellMgr.h"
#include "Unit.h"
#include "Log.h"

// Arcane Stability talent IDs
enum ArcaneStabilityRanks
{
    ARCANE_STABILITY_RANK_1 = 11237,
    ARCANE_STABILITY_RANK_2 = 12463,
    ARCANE_STABILITY_RANK_3 = 12464,
    ARCANE_STABILITY_RANK_4 = 16769,
    ARCANE_STABILITY_RANK_5 = 16770
};

// Arcane Missiles Spell IDs (Ranks 1-13)
const uint32 ARCANE_MISSILES_SPELLS[] = {
    5143, 5144, 5145, 8416, 8417, 10263, 10264, 10265, 25344, 25345, 42844, 42845, 42846
};

class spell_mage_arcane_stability_proc : public SpellScript
{
    PrepareSpellScript(spell_mage_arcane_stability_proc);

    void HandleOnHit()
    {
        Unit* caster = GetCaster();
        if (!caster || caster->GetTypeId() != TYPEID_PLAYER)
            return;

        Player* player = caster->ToPlayer();
        uint8 talentRank = 0;

        // Check for talent rank, from highest to lowest
        if (player->HasAura(ARCANE_STABILITY_RANK_5)) talentRank = 5;
        else if (player->HasAura(ARCANE_STABILITY_RANK_4)) talentRank = 4;
        else if (player->HasAura(ARCANE_STABILITY_RANK_3)) talentRank = 3;
        else if (player->HasAura(ARCANE_STABILITY_RANK_2)) talentRank = 2;
        else if (player->HasAura(ARCANE_STABILITY_RANK_1)) talentRank = 1;

        if (talentRank == 0)
            return;

        // Roll for proc
        uint32 procChance = talentRank * 4;
        if (!roll_chance_i(procChance))
            return;

        // Proc successful, reset cooldowns
        ResetInvocationCooldowns(player);
    }

    void Register() override
    {
        OnHit += SpellHitFn(spell_mage_arcane_stability_proc::HandleOnHit);
    }

private:
    bool IsInvocation(SpellInfo const* spellInfo)
    {
        if (!spellInfo)
            return false;
        // Spell family flags for Invocation: [0] 0x00002000 [1] 0x00000000 [2] 0x00000008
        return (spellInfo->SpellFamilyName == SPELLFAMILY_MAGE &&
                spellInfo->SpellFamilyFlags[0] & 0x00002000 &&
                spellInfo->SpellFamilyFlags[1] == 0x00000000 &&
                spellInfo->SpellFamilyFlags[2] == 0x00000008);
    }

    void ResetInvocationCooldowns(Player* player)
    {
        PlayerSpellMap const& spellMap = player->GetSpellMap();
        for (PlayerSpellMap::const_iterator itr = spellMap.begin(); itr != spellMap.end(); ++itr)
        {
            SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(itr->first);
            if (!spellInfo)
                continue;

            if (IsInvocation(spellInfo))
            {
                if (player->HasSpellCooldown(spellInfo->Id))
                {
                    player->RemoveSpellCooldown(spellInfo->Id, true);
                    LOG_INFO("scripts", "Arcane Stability: Cooldown reset for Invocation spell %u (%s)",
                             spellInfo->Id, spellInfo->SpellName[0]);
                }
            }
        }
    }
};

void AddSC_arcane_stability_mechanic()
{
    RegisterSpellScript(spell_mage_arcane_stability_proc);
}
