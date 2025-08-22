#include "Player.h"
#include "ScriptMgr.h"
#include "SpellAuraEffects.h"
#include "SpellInfo.h"
#include "SpellMgr.h"

class TreeOfLifeAP : public PlayerScript
{
public:
    TreeOfLifeAP() : PlayerScript("TreeOfLifeAP") { }

    void OnPlayerAfterUpdateAttackPowerAndDamage(Player* player, float& /*level*/, float& /*base_attPower*/, float& attPowerMod, float& /*attPowerMultiplier*/, bool ranged) override
    {
        if (ranged || player->GetShapeshiftForm() != FORM_TREE)
        {
            return;
        }

        float mLevelMult = 0.0f;
        float weapon_bonus = 0.0f;
        float predatory_strikes_multiplier = 150.0f; // Default to max rank Predatory Strikes

        // First, get the raw AP from the weapon
        uint32 raw_weapon_ap = 0;
        if (Item* mainHand = player->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_MAINHAND))
        {
            ItemTemplate const* proto = mainHand->GetTemplate();
            if (proto)
            {
                raw_weapon_ap = proto->getFeralBonus();
                for (uint8 i = 0; i < MAX_ITEM_PROTO_STATS; ++i)
                {
                    if (i >= proto->StatsCount)
                        break;
                    if (proto->ItemStat[i].ItemStatType == ITEM_MOD_ATTACK_POWER)
                        raw_weapon_ap += proto->ItemStat[i].ItemStatValue;
                }

                for (uint8 i = 0; i < MAX_ITEM_PROTO_SPELLS; ++i)
                {
                    if (!proto->Spells[i].SpellId || proto->Spells[i].SpellTrigger != ITEM_SPELLTRIGGER_ON_EQUIP)
                        continue;
                    SpellInfo const* spellproto = sSpellMgr->GetSpellInfo(proto->Spells[i].SpellId);
                    if (!spellproto)
                        continue;
                    for (uint8 j = 0; j < MAX_SPELL_EFFECTS; ++j)
                        if (spellproto->Effects[j].ApplyAuraName == SPELL_AURA_MOD_ATTACK_POWER)
                            raw_weapon_ap += spellproto->Effects[j].CalcValue();
                }
            }
        }

        // Now, check for Predatory Strikes to get the multipliers
        Unit::AuraEffectList const& mDummy = player->GetAuraEffectsByType(SPELL_AURA_DUMMY);
        for (Unit::AuraEffectList::const_iterator itr = mDummy.begin(); itr != mDummy.end(); ++itr)
        {
            AuraEffect* aurEff = *itr;
            if (aurEff->GetSpellInfo()->SpellIconID == 1563) // Predatory Strikes Icon
            {
                switch (aurEff->GetEffIndex())
                {
                case 0: // Level multiplier
                    mLevelMult = CalculatePct(1.0f, aurEff->GetAmount());
                    break;
                case 1: // Weapon AP bonus multiplier
                    predatory_strikes_multiplier = aurEff->GetAmount();
                    break;
                }
            }
        }

        // Calculate final weapon bonus
        weapon_bonus = CalculatePct(float(raw_weapon_ap), predatory_strikes_multiplier);

        // The core calculation for non-feral druid AP is missing the level multiplier and weapon bonus.
        // We calculate this missing part and add it to the attack power modifier.
        float bonus_ap = (player->GetLevel() * mLevelMult) + weapon_bonus;
        attPowerMod += bonus_ap;
    }
};

void AddSC_spell_tree_of_life_ap()
{
    new TreeOfLifeAP();
}
