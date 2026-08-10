#include "AlonecraftTestLog.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SpellAuraEffects.h"
#include "SpellScript.h"
#include "SpellScriptLoader.h"
#include "Unit.h"
#include "Util.h"

#include <limits>

// Warrior Protection -- the Alonecraft stat conversions
//
//   Improved Disciplines  (12312 / 12803)          -- woa_2026_08_09_39.sql
//                                                     woa_2026_08_09_42.sql
//   Puncture              (12308 / 12810 / 12811)  -- woa_2026_08_09_40.sql
//   Controlled Aggression (12301 / 12818)          -- woa_2026_08_09_43.sql
//
// TODO.md:
//   "Improved Disciplines (0, 4): Redesigned.  While in Defensive Stance, you
//    gain melee haste rating equal to 50/100% of your defense rating, doubled
//    while Shield Block or Shield Wall is active. (2 ranks)"
//   "Puncture (2, 3): Redesigned.  Increases your Strength by 10/20/30% of your
//    Stamina. (3 ranks)"
//   "Improved Bloodrage (Renamed to Controlled Aggression) (3, 2): Redesigned.
//    While Bloodrage or Berserker Rage is active, 50/100% of your shield block
//    value is added to your attack power. (2 ranks)"
//
// Handled by DBC:
//   Everything except the amounts, and the two conditions -- Improved
//   Disciplines' stance gate and doubling window, and Controlled Aggression's
//   pair of buffs.  The aura types, the target rating/stat and the per-rank
//   percentages are all SQL.
//
// Handled here:
//   The amounts.
//
// Why a script at all.  None of the three conversions has an aura in 3.3.5.
// SPELL_AURA_MOD_RATING_FROM_STAT (220) is the only conversion aura in the
// game and it goes stat -> rating only, reading base stats
// (PlayerUpdates.cpp:609-620).  Improved Disciplines is rating -> rating,
// Puncture is stat -> stat, and Controlled Aggression's source is block value,
// which is neither -- so all three amounts have to be computed.
//
// All three follow WarriorParryConversions.cpp exactly: capture the per-rank
// percentage the DBC put in base points, overwrite the amount with the derived
// value, and force a 2 second heartbeat so the result tracks gear, buffs,
// stance changes and the gating cooldowns.  Marking a non-periodic effect
// periodic from a script is legal -- AuraEffect::CalculatePeriodic asks the
// script first.  The heartbeat is not an optimisation here: nothing in core
// re-evaluates an aura amount when an unrelated aura or the shapeshift form
// changes, so it is the only thing that makes the conditions take effect.
//
// Every target aura type responds to AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK
// (SpellAuraEffects.cpp:4199, :4855 and :4890), so RecalculateAmount() genuinely
// re-applies the value rather than only updating a stored number.  For haste
// specifically that matters twice over: Player::ApplyRatingMod (Player.cpp:5288)
// is where CR_HASTE_MELEE turns into attack speed, and it is only reached
// through the handler.

enum WarriorProtConversionSpells
{
    SPELL_WARRIOR_SHIELD_WALL     = 871,
    SPELL_WARRIOR_SHIELD_BLOCK    = 2565,
    SPELL_WARRIOR_BLOODRAGE_AURA  = 29131,
    SPELL_WARRIOR_BERSERKER_RAGE  = 18499
};

// ============================================================
// Improved Disciplines: melee haste rating from defense rating
// ============================================================
// Rating to rating, which is why the value cannot live in the DBC.
//
// The stance gate is here rather than in the DBC Stances field on purpose.
// That field has broken three talents in this fork already (TODO.md:416, :484),
// and a script check simply reads zero out of stance instead of unapplying and
// reapplying the whole aura on every stance dance.
//
// Shield Block and Shield Wall double the conversion (woa_2026_08_09_42.sql).
// Barricade (200652) is deliberately NOT in that list even though it is
// Alonecraft's second Shield Block: it is 10 seconds up on a 10 second
// cooldown, so counting it would make the doubled value permanent rather than
// a burst window, which is the whole point of tying it to a cooldown.
class spell_warr_imp_disciplines : public AuraScript
{
    PrepareAuraScript(spell_warr_imp_disciplines);

    void CalculateAmount(AuraEffect const* /*aurEff*/, int32& amount, bool& /*canBeRecalculated*/)
    {
        // The incoming amount is the per-rank percentage from the DBC
        // (25 / 50).  Capture it before overwriting.
        int32 pct = amount;
        amount = 0;

        Player* player = GetUnitOwner() ? GetUnitOwner()->ToPlayer() : nullptr;
        if (!player || pct <= 0)
            return;

        if (player->GetShapeshiftForm() != FORM_DEFENSIVESTANCE)
            return;

        bool const doubled = player->HasAura(SPELL_WARRIOR_SHIELD_BLOCK)
                          || player->HasAura(SPELL_WARRIOR_SHIELD_WALL);
        if (doubled)
            pct *= 2;

        // The character-sheet defense rating field, written by
        // Player::UpdateRating (PlayerUpdates.cpp:621).  It is the gear rating
        // plus any SPELL_AURA_MOD_RATING_FROM_STAT contribution -- the number
        // the player reads off the character sheet, which is what the tooltip
        // promises a share of.
        //
        // Reading the field rather than m_baseRatingValue is also what keeps
        // this non-circular in an obvious way: the effect writes to
        // CR_HASTE_MELEE and never to CR_DEFENSE_SKILL.
        uint32 const defenseRating = player->GetUInt32Value(
            static_cast<uint16>(PLAYER_FIELD_COMBAT_RATING_1) + static_cast<uint16>(CR_DEFENSE_SKILL));
        if (!defenseRating)
            return;

        amount = int32(defenseRating) * pct / 100;

        if (amount != _lastLogged)
        {
            _lastLogged = amount;
            ACTEST("WAR.IMPDISCIPLINES", "rank={} defenseRating={} pct={} doubled={} -> hasteRating={} attackTime={}",
                GetId(), defenseRating, pct, doubled, amount, player->GetAttackTime(BASE_ATTACK));
        }
    }

    int32 _lastLogged = std::numeric_limits<int32>::lowest();

    // The heartbeat is what makes a stance change or a Shield Block press take
    // effect -- nothing in core re-evaluates an aura amount when the shapeshift
    // form changes or when an unrelated aura is gained or lost.
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
        DoEffectCalcAmount   += AuraEffectCalcAmountFn(spell_warr_imp_disciplines::CalculateAmount, EFFECT_0, SPELL_AURA_MOD_RATING);
        DoEffectCalcPeriodic += AuraEffectCalcPeriodicFn(spell_warr_imp_disciplines::CalcPeriodic, EFFECT_0, SPELL_AURA_MOD_RATING);
        OnEffectPeriodic     += AuraEffectPeriodicFn(spell_warr_imp_disciplines::HandlePeriodic, EFFECT_0, SPELL_AURA_MOD_RATING);
    }
};

// ============================================================
// Puncture: Strength from Stamina
// ============================================================
// Stat to stat, which aura 29 cannot express on its own -- it takes a flat
// number of stat points, not a share of another stat.
//
// No stance or gear gate: the talent is unconditional, so the heartbeat exists
// only to track Stamina itself as gear, buffs and Vitality change it.
class spell_warr_puncture : public AuraScript
{
    PrepareAuraScript(spell_warr_puncture);

    void CalculateAmount(AuraEffect const* /*aurEff*/, int32& amount, bool& /*canBeRecalculated*/)
    {
        // The incoming amount is the per-rank percentage from the DBC
        // (5 / 10 / 15).  Capture it before overwriting.
        int32 const pct = amount;
        amount = 0;

        Player* player = GetUnitOwner() ? GetUnitOwner()->ToPlayer() : nullptr;
        if (!player || pct <= 0)
            return;

        // Total Stamina, the character-sheet number, not the base value aura
        // 220 would have read.  The tooltip says "your Stamina", and on a
        // Protection warrior almost all of it is gear and buffs.
        //
        // Non-circular by construction: the input is Stamina and the output is
        // Strength, so this can never feed itself.
        float const stamina = player->GetStat(STAT_STAMINA);
        if (stamina <= 0.0f)
            return;

        amount = int32(stamina * float(pct) / 100.0f);

        if (amount != _lastLogged)
        {
            _lastLogged = amount;
            ACTEST("WAR.PUNCTURE", "rank={} stamina={:.0f} pct={} -> extraStr={} totalStr={:.0f}",
                GetId(), stamina, pct, amount, player->GetStat(STAT_STRENGTH));
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
        DoEffectCalcAmount   += AuraEffectCalcAmountFn(spell_warr_puncture::CalculateAmount, EFFECT_0, SPELL_AURA_MOD_STAT);
        DoEffectCalcPeriodic += AuraEffectCalcPeriodicFn(spell_warr_puncture::CalcPeriodic, EFFECT_0, SPELL_AURA_MOD_STAT);
        OnEffectPeriodic     += AuraEffectPeriodicFn(spell_warr_puncture::HandlePeriodic, EFFECT_0, SPELL_AURA_MOD_STAT);
    }
};

// ============================================================
// Controlled Aggression: attack power from shield block value
// ============================================================
// Was Improved Bloodrage.  Block value is neither a stat nor a rating --
// Player::GetShieldBlockValue (Player.cpp:5115) derives it from Strength and
// the SHIELD_BLOCK_VALUE base mods -- so no aura can read it as a conversion
// source, and the amount has to be computed here.
//
// Both gates are real 10 second buff-bar windows.  Note the Bloodrage aura is
// 29131, NOT 2687: 2687 is instantaneous (SPELL_EFFECT_ENERGIZE plus a
// SPELL_EFFECT_TRIGGER_SPELL) and 29131 is the periodic-energize aura it
// triggers, which is the one a warrior actually "has active".
//
// No circularity: block value reads Strength and the block-value aura mods,
// and flat attack power feeds neither.
class spell_warr_controlled_aggression : public AuraScript
{
    PrepareAuraScript(spell_warr_controlled_aggression);

    void CalculateAmount(AuraEffect const* /*aurEff*/, int32& amount, bool& /*canBeRecalculated*/)
    {
        // The incoming amount is the per-rank percentage from the DBC
        // (12 / 25).  Capture it before overwriting.
        int32 const pct = amount;
        amount = 0;

        Player* player = GetUnitOwner() ? GetUnitOwner()->ToPlayer() : nullptr;
        if (!player || pct <= 0)
            return;

        if (!player->HasAura(SPELL_WARRIOR_BLOODRAGE_AURA)
            && !player->HasAura(SPELL_WARRIOR_BERSERKER_RAGE))
            return;

        // Already includes Toughness' Strength slice and Shield Mastery's
        // percentage, since both land inside GetShieldBlockValue.  No shield
        // equipped means a block value of zero, so the talent costs nothing to
        // leave on.
        uint32 const blockValue = player->GetShieldBlockValue();
        if (!blockValue)
            return;

        amount = CalculatePct(int32(blockValue), pct);

        if (amount != _lastLogged)
        {
            _lastLogged = amount;
            ACTEST("WAR.CONTROLLEDAGGRESSION", "rank={} blockValue={} pct={} -> extraAP={} totalAP={}",
                GetId(), blockValue, pct, amount, player->GetTotalAttackPowerValue(BASE_ATTACK));
        }
    }

    int32 _lastLogged = std::numeric_limits<int32>::lowest();

    // The heartbeat is what makes pressing Bloodrage or Berserker Rage take
    // effect, and what drops the bonus again when the buff falls off.
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
        DoEffectCalcAmount   += AuraEffectCalcAmountFn(spell_warr_controlled_aggression::CalculateAmount, EFFECT_0, SPELL_AURA_MOD_ATTACK_POWER);
        DoEffectCalcPeriodic += AuraEffectCalcPeriodicFn(spell_warr_controlled_aggression::CalcPeriodic, EFFECT_0, SPELL_AURA_MOD_ATTACK_POWER);
        OnEffectPeriodic     += AuraEffectPeriodicFn(spell_warr_controlled_aggression::HandlePeriodic, EFFECT_0, SPELL_AURA_MOD_ATTACK_POWER);
    }
};

class spell_warr_imp_disciplines_loader : public SpellScriptLoader
{
public:
    spell_warr_imp_disciplines_loader() : SpellScriptLoader("spell_warr_imp_disciplines") { }

    AuraScript* GetAuraScript() const override
    {
        return new spell_warr_imp_disciplines();
    }
};

class spell_warr_puncture_loader : public SpellScriptLoader
{
public:
    spell_warr_puncture_loader() : SpellScriptLoader("spell_warr_puncture") { }

    AuraScript* GetAuraScript() const override
    {
        return new spell_warr_puncture();
    }
};

class spell_warr_controlled_aggression_loader : public SpellScriptLoader
{
public:
    spell_warr_controlled_aggression_loader() : SpellScriptLoader("spell_warr_controlled_aggression") { }

    AuraScript* GetAuraScript() const override
    {
        return new spell_warr_controlled_aggression();
    }
};

void AddSC_war_prot_conversions()
{
    new spell_warr_imp_disciplines_loader();
    new spell_warr_puncture_loader();
    new spell_warr_controlled_aggression_loader();
}
