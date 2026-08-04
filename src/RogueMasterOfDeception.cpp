#include "AlonecraftTestLog.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SpellAuraEffects.h"
#include "SpellScript.h"
#include "SpellScriptLoader.h"
#include "Unit.h"

#include <limits>

// Master of Deception (13958 / 13970 / 13971) -- Alonecraft
//
// TODO.md: "Your agility is 33/66/100% more effective at increasing your
// dodge chance, and (as today)."
//
// Effect1 (MOD_STEALTH_LEVEL) is left to the DBC; this script only fills in
// the amount of the MOD_DODGE_PERCENT added as effect 2.
//
// No aura multiplies the agility-to-dodge conversion in this core, and
// SPELL_AURA_MOD_RATING_FROM_STAT is not equivalent -- it grants dodge
// *rating*, which is subject to diminishing returns, so "33% more
// effective" would come out as something else entirely.  Instead the
// conversion is asked for directly: Player::GetDodgeFromAgility is public,
// so the ratio table lookup, the level cap and the base/bonus agility split
// all stay in core hands and only the rogue's flat class base has to be
// mirrored here.
//
// Caveat worth knowing: the result is applied as MOD_DODGE_PERCENT, which
// StatSystem.cpp:821 adds to the *non-diminishing* half of dodge, whereas
// bonus agility normally lands in the diminishing half.  The magnitude is
// exactly the percentage asked for; it just does not decay at very high
// gear levels the way scaling the underlying agility would.

// Mirrors dodge_base[CLASS_ROGUE - 1] in Player.cpp:5134.  That table is
// file-local with no accessor.  Keep in sync if it is ever retuned.
// Subtracted out so only the agility-derived dodge is scaled -- the flat
// class base is not something agility contributes to.
static float const ROGUE_DODGE_CLASS_BASE_PCT = 100.0f * 0.020957f;

class spell_rog_master_of_deception : public AuraScript
{
    PrepareAuraScript(spell_rog_master_of_deception);

    void CalculateAmount(AuraEffect const* /*aurEff*/, int32& amount, bool& /*canBeRecalculated*/)
    {
        // The incoming amount is the per-rank percentage from the DBC
        // (33 / 66 / 100).  Capture it before overwriting.
        int32 const pct = amount;
        amount = 0;

        Player* player = GetUnitOwner() ? GetUnitOwner()->ToPlayer() : nullptr;
        if (!player || pct <= 0)
            return;

        float diminishing = 0.0f;
        float nondiminishing = 0.0f;
        player->GetDodgeFromAgility(diminishing, nondiminishing);

        float const fromAgility = diminishing + nondiminishing - ROGUE_DODGE_CLASS_BASE_PCT;
        if (fromAgility <= 0.0f)
            return;

        amount = int32(fromAgility * float(pct) / 100.0f);

        // 2 sec heartbeat -- only log when the value actually moves.
        if (amount != _lastLogged)
        {
            _lastLogged = amount;
            ACTEST("ROG.MOD",
                "rank={} agi={:.1f} dodgeDim={:.4f} dodgeNonDim={:.4f} classBase={:.4f} "
                "fromAgility={:.4f} bonusPct={} -> extraDodge={} totalDodgeField={:.2f}",
                GetId(), player->GetStat(STAT_AGILITY), diminishing, nondiminishing,
                ROGUE_DODGE_CLASS_BASE_PCT, fromAgility, pct, amount,
                player->GetFloatValue(PLAYER_DODGE_PERCENTAGE));
        }
    }

    int32 _lastLogged = std::numeric_limits<int32>::lowest();

    // Agility moves with gear, buffs and food, so the amount cannot be
    // frozen at apply time.  Same heartbeat idiom as
    // ALONECRAFT_PET_HEARTBEAT in WarlockDemonPets.cpp -- marking a
    // non-periodic effect periodic from a script is legal, because
    // AuraEffect::CalculatePeriodic asks the script first.
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
        DoEffectCalcAmount   += AuraEffectCalcAmountFn(spell_rog_master_of_deception::CalculateAmount, EFFECT_1, SPELL_AURA_MOD_DODGE_PERCENT);
        DoEffectCalcPeriodic += AuraEffectCalcPeriodicFn(spell_rog_master_of_deception::CalcPeriodic, EFFECT_1, SPELL_AURA_MOD_DODGE_PERCENT);
        OnEffectPeriodic     += AuraEffectPeriodicFn(spell_rog_master_of_deception::HandlePeriodic, EFFECT_1, SPELL_AURA_MOD_DODGE_PERCENT);
    }
};

class spell_rog_master_of_deception_loader : public SpellScriptLoader
{
public:
    spell_rog_master_of_deception_loader() : SpellScriptLoader("spell_rog_master_of_deception") { }

    AuraScript* GetAuraScript() const override
    {
        return new spell_rog_master_of_deception();
    }
};

void AddSC_rog_master_of_deception()
{
    new spell_rog_master_of_deception_loader();
}
