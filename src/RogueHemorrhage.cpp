#include "AlonecraftTestLog.h"
#include "ScriptMgr.h"
#include "SpellAuraEffects.h"
#include "SpellScript.h"
#include "SpellScriptLoader.h"
#include "Unit.h"

// Hemorrhage (16511 / 17347 / 17348 / 26864 / 48660) -- Alonecraft
//
// TODO.md: "Physical damage increase no longer has charges and is increased
// by attack power."
//
// The charge removal is pure DBC (ProcCharges 10 -> 0, woa_2026_08_03_08).
// This script only supplies the attack-power half.
//
// Hemorrhage's debuff is effect 3 of the strike itself -- MOD_DAMAGE_TAKEN
// with EffectMiscValue 1 (physical), a flat amount added to every physical
// hit the target takes.  There is no coefficient field for that aura, so
// the amount is topped up here.

enum HemorrhageData
{
    // Share of the rogue's melee attack power folded into the debuff.
    // This is the tuning knob for the talent -- roughly +200 per hit at
    // 4000 AP, against a rank-5 base of 75.
    HEMORRHAGE_AP_PCT = 5,
};

class spell_rog_hemorrhage : public AuraScript
{
    PrepareAuraScript(spell_rog_hemorrhage);

    void CalculateAmount(AuraEffect const* /*aurEff*/, int32& amount, bool& canBeRecalculated)
    {
        Unit* caster = GetCaster();
        if (!caster)
            return;

        int32 const dbcBase = amount;
        int32 const ap = int32(caster->GetTotalAttackPowerValue(BASE_ATTACK));
        int32 const fromAp = CalculatePct(ap, HEMORRHAGE_AP_PCT);

        amount += fromAp;

        // Snapshot at application, like the rest of the debuff: the value
        // should not drift if the rogue's attack power changes mid-duration.
        canBeRecalculated = false;

        ACTEST("ROG.HEMO",
            "hemorrhage={} target={} dbcBase={} ap={} apPct={} fromAp={} total={}",
            GetId(), Alonecraft::TestLog::N(GetUnitOwner()), dbcBase, ap,
            int32(HEMORRHAGE_AP_PCT), fromAp, amount);
    }

    void Register() override
    {
        DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_rog_hemorrhage::CalculateAmount, EFFECT_2, SPELL_AURA_MOD_DAMAGE_TAKEN);
    }
};

class spell_rog_hemorrhage_loader : public SpellScriptLoader
{
public:
    spell_rog_hemorrhage_loader() : SpellScriptLoader("spell_rog_hemorrhage") { }

    AuraScript* GetAuraScript() const override
    {
        return new spell_rog_hemorrhage();
    }
};

void AddSC_rog_hemorrhage()
{
    new spell_rog_hemorrhage_loader();
}
