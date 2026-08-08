#include "AlonecraftTestLog.h"
#include "Item.h"
#include "ItemTemplate.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SpellAuraEffects.h"
#include "SpellScript.h"
#include "SpellScriptLoader.h"
#include "Unit.h"

#include <limits>

// Warrior Arms -- the two parry conversions
//
//   Two-Handed Weapon Specialization (12163 / 12711 / 12712)
//       -- woa_2026_08_07_13.sql
//   Weapon Mastery (20504 / 20505)
//       -- woa_2026_08_07_14.sql
//
// TODO.md:
//   "Two-Handed Weapon Specialization: ... while using a 2h weapon your chance
//    to parry is increased by 33/66/100% of your critical strike chance."
//   "Weapon Mastery: ... while using a two-handed weapon your parry chance is
//    increased by your strength."
//
// Handled by DBC:
//   Everything except the amount of one effect on each spell.  The damage
//   bonus, the dodge reduction, the disarm reduction, the aura types and the
//   per-rank percentages are all DBC.
//
// Handled here:
//   The amounts, and for Weapon Mastery the two-handed gate.
//
// Why a script at all.  Two-Handed Weapon Specialization is the only mechanic
// in this redesign with no prior art: SPELL_AURA_MOD_RATING_FROM_STAT (220) is
// the sole stat-to-rating conversion in 3.3.5 and it reads base stats only
// (PlayerUpdates.cpp:612-620).  Nothing in Spell.dbc scales one rating off
// another, so crit-chance-to-parry has to be computed.
//
// Both scripts follow RogueMasterOfDeception.cpp exactly: capture the per-rank
// percentage the DBC put in base points, overwrite the amount with the derived
// value, and force a 2 second heartbeat so the result tracks gear, buffs and
// weapon swaps.  Marking a non-periodic effect periodic from a script is legal
// -- AuraEffect::CalculatePeriodic asks the script first.
//
// Both target aura types respond to AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK
// (SpellAuraEffects.cpp:4597 and :4872), so RecalculateAmount() genuinely
// re-applies the value rather than only updating a stored number.

namespace
{
    bool HasTwoHandedWeapon(Player const* player)
    {
        Item const* mainHand = player->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_MAINHAND);
        if (!mainHand)
            return false;

        ItemTemplate const* proto = mainHand->GetTemplate();
        return proto && proto->InventoryType == INVTYPE_2HWEAPON;
    }
}

// ============================================================
// Two-Handed Weapon Specialization: parry = N% of crit chance
// ============================================================
// No explicit two-handed check here.  The spell itself carries
// EquippedItemClass 2 / EquippedItemSubClassMask 354 and both of its effects
// are meant to be two-handed-only, so core unapplies and re-applies the whole
// aura on a weapon swap.
class spell_warr_2h_spec_parry : public AuraScript
{
    PrepareAuraScript(spell_warr_2h_spec_parry);

    void CalculateAmount(AuraEffect const* /*aurEff*/, int32& amount, bool& /*canBeRecalculated*/)
    {
        // The incoming amount is the per-rank percentage from the DBC
        // (33 / 66 / 100).  Capture it before overwriting.
        int32 const pct = amount;
        amount = 0;

        Player* player = GetUnitOwner() ? GetUnitOwner()->ToPlayer() : nullptr;
        if (!player || pct <= 0)
            return;

        // Mainhand melee crit, the same field melee crit rolls read
        // (Unit.cpp:3920).  Maintained by Player::UpdateCritPercentage, so it
        // already includes crit rating, agility and Tactical Mastery's
        // SPELL_AURA_MOD_CRIT_PCT.
        float const critPct = player->GetFloatValue(PLAYER_CRIT_PERCENTAGE);
        if (critPct <= 0.0f)
            return;

        amount = int32(critPct * float(pct) / 100.0f);

        if (amount != _lastLogged)
        {
            _lastLogged = amount;
            ACTEST("WAR.2HSPEC", "rank={} crit={:.2f} pct={} -> extraParry={} totalParryField={:.2f}",
                GetId(), critPct, pct, amount, player->GetFloatValue(PLAYER_PARRY_PERCENTAGE));
        }
    }

    int32 _lastLogged = std::numeric_limits<int32>::lowest();

    void CalcPeriodic(AuraEffect const* /*aurEff*/, bool& isPeriodic, int32& amplitude)
    {
        isPeriodic = true;
        amplitude  = 2 * IN_MILLISECONDS;
    }

    void HandlePeriodic(AuraEffect const* aurEff)
    {
        PreventDefaultAction();
        GetEffect(aurEff->GetEffIndex())->RecalculateAmount();
    }

    void Register() override
    {
        DoEffectCalcAmount   += AuraEffectCalcAmountFn(spell_warr_2h_spec_parry::CalculateAmount, EFFECT_1, SPELL_AURA_MOD_PARRY_PERCENT);
        DoEffectCalcPeriodic += AuraEffectCalcPeriodicFn(spell_warr_2h_spec_parry::CalcPeriodic, EFFECT_1, SPELL_AURA_MOD_PARRY_PERCENT);
        OnEffectPeriodic     += AuraEffectPeriodicFn(spell_warr_2h_spec_parry::HandlePeriodic, EFFECT_1, SPELL_AURA_MOD_PARRY_PERCENT);
    }
};

// ============================================================
// Weapon Mastery: parry rating from Strength, two-handed only
// ============================================================
// The conversion itself is stock (SPELL_AURA_MOD_RATING_FROM_STAT, the Forceful
// Deflection shape) and the DBC does the arithmetic.  This script only decides
// whether it applies, because the spell cannot carry EquippedItemClass -- the
// dodge reduction on effect 0 has to stay unconditional.
class spell_warr_weapon_mastery_parry : public AuraScript
{
    PrepareAuraScript(spell_warr_weapon_mastery_parry);

    void CalculateAmount(AuraEffect const* /*aurEff*/, int32& amount, bool& /*canBeRecalculated*/)
    {
        Player* player = GetUnitOwner() ? GetUnitOwner()->ToPlayer() : nullptr;
        if (!player || !HasTwoHandedWeapon(player))
        {
            amount = 0;
            return;
        }

        // Otherwise the DBC value (12 / 25, meaning percent of Strength) is
        // left exactly as it is -- core does the stat lookup and the rating
        // conversion in Player::UpdateRating.
        if (amount != _lastLogged)
        {
            _lastLogged = amount;
            ACTEST("WAR.WEAPONMASTERY", "rank={} twoHander=yes strPct={} str={:.0f} parryField={:.2f}",
                GetId(), amount, player->GetStat(STAT_STRENGTH),
                player->GetFloatValue(PLAYER_PARRY_PERCENTAGE));
        }
    }

    int32 _lastLogged = std::numeric_limits<int32>::lowest();

    // Needed here in a way it is not for the sibling script above: this spell
    // has no EquippedItemClass, so nothing in core re-evaluates it on a weapon
    // swap.  The heartbeat is what makes unequipping a two-hander take effect.
    void CalcPeriodic(AuraEffect const* /*aurEff*/, bool& isPeriodic, int32& amplitude)
    {
        isPeriodic = true;
        amplitude  = 2 * IN_MILLISECONDS;
    }

    void HandlePeriodic(AuraEffect const* aurEff)
    {
        PreventDefaultAction();
        GetEffect(aurEff->GetEffIndex())->RecalculateAmount();
    }

    void Register() override
    {
        DoEffectCalcAmount   += AuraEffectCalcAmountFn(spell_warr_weapon_mastery_parry::CalculateAmount, EFFECT_2, SPELL_AURA_MOD_RATING_FROM_STAT);
        DoEffectCalcPeriodic += AuraEffectCalcPeriodicFn(spell_warr_weapon_mastery_parry::CalcPeriodic, EFFECT_2, SPELL_AURA_MOD_RATING_FROM_STAT);
        OnEffectPeriodic     += AuraEffectPeriodicFn(spell_warr_weapon_mastery_parry::HandlePeriodic, EFFECT_2, SPELL_AURA_MOD_RATING_FROM_STAT);
    }
};

class spell_warr_2h_spec_parry_loader : public SpellScriptLoader
{
public:
    spell_warr_2h_spec_parry_loader() : SpellScriptLoader("spell_warr_2h_spec_parry") { }

    AuraScript* GetAuraScript() const override
    {
        return new spell_warr_2h_spec_parry();
    }
};

class spell_warr_weapon_mastery_parry_loader : public SpellScriptLoader
{
public:
    spell_warr_weapon_mastery_parry_loader() : SpellScriptLoader("spell_warr_weapon_mastery_parry") { }

    AuraScript* GetAuraScript() const override
    {
        return new spell_warr_weapon_mastery_parry();
    }
};

void AddSC_war_parry_conversions()
{
    new spell_warr_2h_spec_parry_loader();
    new spell_warr_weapon_mastery_parry_loader();
}
