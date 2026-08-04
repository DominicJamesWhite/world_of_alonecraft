#include "AlonecraftTestLog.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SpellAuraEffects.h"
#include "SpellScript.h"
#include "SpellScriptLoader.h"
#include "Unit.h"

#include <limits>

// Tricks of the Trade (57934) -- Alonecraft self-cast
//
// TODO.md: "Can be self-cast. When self-cast it also increases dodge chance
// by 20% of your crit chance and doubles threat."
//
// The cast itself is unblocked in the DBC (woa_2026_08_03_04.sql clears
// SPELL_ATTR1_EXCLUDE_CASTER).  All this script does is notice that the
// rogue targeted himself and hand him the 200500 buff.
//
// Core's spell_rog_tricks_of_the_trade (spell_rogue.cpp:619) still runs on
// the same spell and is deliberately left alone -- it already tolerates the
// degenerate redirect (rogue redirecting to himself) and nothing here needs
// to interfere with the ally case.

enum TricksOfTheTradeSpells
{
    SPELL_ROGUE_TRICKS_SELF_BUFF = 200500,

    // 20% of the rogue's crit chance, per the TODO.
    TRICKS_SELF_DODGE_PCT_OF_CRIT = 20,
};

// The dodge amount is derived from a live stat, so it has to be recomputed
// as gear and buffs change rather than frozen at apply time.  Same idiom as
// ALONECRAFT_PET_HEARTBEAT in WarlockDemonPets.cpp: attaching a periodic to
// an effect the DBC does not mark periodic is legal, because
// AuraEffect::CalculatePeriodic asks the script.
class spell_rog_tricks_self_buff : public AuraScript
{
    PrepareAuraScript(spell_rog_tricks_self_buff);

    void CalculateAmount(AuraEffect const* /*aurEff*/, int32& amount, bool& /*canBeRecalculated*/)
    {
        amount = 0;

        Player* player = GetUnitOwner() ? GetUnitOwner()->ToPlayer() : nullptr;
        if (!player)
            return;

        float const crit = player->GetFloatValue(PLAYER_CRIT_PERCENTAGE);
        amount = CalculatePct<int32, float>(crit, TRICKS_SELF_DODGE_PCT_OF_CRIT);

        // 2 sec heartbeat -- only log when the value actually moves.
        if (amount != _lastLogged)
        {
            _lastLogged = amount;
            ACTEST("ROG.TRICKS",
                "self-buff meleeCrit={:.2f} pctOfCrit={} -> dodgeBonus={} dodgeField={:.2f}",
                crit, int32(TRICKS_SELF_DODGE_PCT_OF_CRIT), amount,
                player->GetFloatValue(PLAYER_DODGE_PERCENTAGE));
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
        DoEffectCalcAmount   += AuraEffectCalcAmountFn(spell_rog_tricks_self_buff::CalculateAmount, EFFECT_0, SPELL_AURA_MOD_DODGE_PERCENT);
        DoEffectCalcPeriodic += AuraEffectCalcPeriodicFn(spell_rog_tricks_self_buff::CalcPeriodic, EFFECT_0, SPELL_AURA_MOD_DODGE_PERCENT);
        OnEffectPeriodic     += AuraEffectPeriodicFn(spell_rog_tricks_self_buff::HandlePeriodic, EFFECT_0, SPELL_AURA_MOD_DODGE_PERCENT);
    }
};

class spell_rog_tricks_self : public SpellScript
{
    PrepareSpellScript(spell_rog_tricks_self);

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_ROGUE_TRICKS_SELF_BUFF });
    }

    bool Load() override
    {
        return GetCaster()->IsPlayer();
    }

    void HandleAfterHit()
    {
        Unit* caster = GetCaster();
        if (!caster)
            return;

        if (GetHitUnit() != caster)
        {
            ACTEST("ROG.TRICKS", "cast on ally {} -- core redirect only, no self buff",
                Alonecraft::TestLog::N(GetHitUnit()));
            return;
        }

        caster->CastSpell(caster, SPELL_ROGUE_TRICKS_SELF_BUFF, true);

        ACTEST("ROG.TRICKS", "SELF-CAST detected buff={} applied={}",
            uint32(SPELL_ROGUE_TRICKS_SELF_BUFF),
            caster->HasAura(SPELL_ROGUE_TRICKS_SELF_BUFF) ? "yes" : "NO");
    }

    void Register() override
    {
        AfterHit += SpellHitFn(spell_rog_tricks_self::HandleAfterHit);
    }
};

class spell_rog_tricks_self_loader : public SpellScriptLoader
{
public:
    spell_rog_tricks_self_loader() : SpellScriptLoader("spell_rog_tricks_self") { }

    SpellScript* GetSpellScript() const override
    {
        return new spell_rog_tricks_self();
    }
};

class spell_rog_tricks_self_buff_loader : public SpellScriptLoader
{
public:
    spell_rog_tricks_self_buff_loader() : SpellScriptLoader("spell_rog_tricks_self_buff") { }

    AuraScript* GetAuraScript() const override
    {
        return new spell_rog_tricks_self_buff();
    }
};

void AddSC_rog_tricks_self()
{
    new spell_rog_tricks_self_loader();
    new spell_rog_tricks_self_buff_loader();
}
