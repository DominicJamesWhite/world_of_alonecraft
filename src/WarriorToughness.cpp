#include "AlonecraftTestLog.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SpellAuraEffects.h"
#include "SpellScript.h"
#include "SpellScriptLoader.h"
#include "Unit.h"

#include <limits>

// Warrior Protection -- Toughness scales block value off Strength
//
//   Toughness (12299 / 12761 / 12762 / 12763 / 12764)
//       -- woa_2026_08_09_12.sql
//
// TODO.md:
//   "Toughness (0, 0): (As today plus) doubles block value scaling from
//    Strength. (5 ranks)"
//
// Handled by DBC:
//   Everything except one number.  The armour bonus (Effect1), the slow
//   duration reduction (Effect2), the new SPELL_AURA_MOD_SHIELD_BLOCKVALUE
//   effect slot (Effect3), the per-rank percentage and the description are all
//   DBC.
//
// Handled here:
//   The amount of Effect3, because it is derived from a stat.
//
// Why a script at all.  Player::GetShieldBlockValue (Player.cpp:5115) is
//
//     (m_auraBaseFlatMod[SHIELD_BLOCK_VALUE] + GetStat(STAT_STRENGTH) * 0.5f - 10)
//         * m_auraBasePctMod[SHIELD_BLOCK_VALUE]
//
// The 0.5f is hard-coded and no aura scales block value off a stat --
// SPELL_AURA_MOD_RATING_FROM_STAT (220) is the only stat-to-anything
// conversion in 3.3.5 and it feeds combat ratings, not block value.  The one
// thing that IS available is the flat term, so the talent adds a second
// Strength slice there: at rank N it contributes Strength * 0.5 * (N * 10)%,
// which at 5/5 is another full Strength * 0.5 -- exactly double.  Landing in
// the flat term is also the correct place, because it sits INSIDE the
// m_auraBasePctMod multiplier and therefore still benefits from Shield
// Mastery's percentage.
//
// The DBC base points hold the per-rank percentage (10 / 20 / 30 / 40 / 50) and
// the description prints it with $s3, so the tooltip stays honest without the
// script touching it.
//
// This follows WarriorParryConversions.cpp exactly: capture the percentage the
// DBC put in base points, overwrite the amount with the derived value, and
// force a 2 second heartbeat so the result tracks gear and buffs.  Marking a
// non-periodic effect periodic from a script is legal -- AuraEffect::
// CalculatePeriodic asks the script first.
//
// SPELL_AURA_MOD_SHIELD_BLOCKVALUE responds to
// AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK (SpellAuraEffects.cpp:5042), so
// RecalculateAmount() genuinely re-applies through HandleBaseModFlatValue
// rather than only updating a stored number.

class spell_warr_toughness_blockvalue : public AuraScript
{
    PrepareAuraScript(spell_warr_toughness_blockvalue);

    void CalculateAmount(AuraEffect const* /*aurEff*/, int32& amount, bool& /*canBeRecalculated*/)
    {
        // The incoming amount is the per-rank percentage from the DBC
        // (10 / 20 / 30 / 40 / 50).  Capture it before overwriting.
        int32 const pct = amount;
        amount = 0;

        Player* player = GetUnitOwner() ? GetUnitOwner()->ToPlayer() : nullptr;
        if (!player || pct <= 0)
            return;

        // The same 0.5f coefficient GetShieldBlockValue applies to the base
        // Strength term, scaled by the talent's percentage.
        float const strength = player->GetStat(STAT_STRENGTH);
        amount = int32(strength * 0.5f * float(pct) / 100.0f);

        if (amount != _lastLogged)
        {
            _lastLogged = amount;
            ACTEST("WAR.TOUGHNESS", "rank={} str={:.0f} pct={} -> extraBlockValue={} totalBlockValue={}",
                GetId(), strength, pct, amount, player->GetShieldBlockValue());
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
        DoEffectCalcAmount   += AuraEffectCalcAmountFn(spell_warr_toughness_blockvalue::CalculateAmount, EFFECT_2, SPELL_AURA_MOD_SHIELD_BLOCKVALUE);
        DoEffectCalcPeriodic += AuraEffectCalcPeriodicFn(spell_warr_toughness_blockvalue::CalcPeriodic, EFFECT_2, SPELL_AURA_MOD_SHIELD_BLOCKVALUE);
        OnEffectPeriodic     += AuraEffectPeriodicFn(spell_warr_toughness_blockvalue::HandlePeriodic, EFFECT_2, SPELL_AURA_MOD_SHIELD_BLOCKVALUE);
    }
};

class spell_warr_toughness_blockvalue_loader : public SpellScriptLoader
{
public:
    spell_warr_toughness_blockvalue_loader() : SpellScriptLoader("spell_warr_toughness_blockvalue") { }

    AuraScript* GetAuraScript() const override
    {
        return new spell_warr_toughness_blockvalue();
    }
};

void AddSC_war_toughness()
{
    new spell_warr_toughness_blockvalue_loader();
}
