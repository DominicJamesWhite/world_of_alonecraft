#include "AlonecraftTestLog.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SpellAuraEffects.h"
#include "SpellScript.h"
#include "SpellScriptLoader.h"
#include "Unit.h"
#include "Random.h"
#include "Util.h"

#include <algorithm>

// Spellshield (was Improved Spell Reflection) -- Warrior Protection talent
// Talent ranks: 59088 / 59089 -- woa_2026_08_09_16.sql
//
// TODO.md: "Improved Spell Reflection (Renamed to Spellshield) (0, 3):
//  Redesigned. You have a chance equal to your block chance to reduce damage
//  taken by spells by an amount equal to your block value. (2 ranks)"
//
// Handled by DBC:
//   The talent is SPELL_AURA_SCHOOL_ABSORB on Effect1 with school mask 126 (all
//   magic schools, not physical) and SPELL_AURA_DUMMY on Effect2 carrying the
//   per-rank percentage of block value.  The prose too.
//
// Handled here:
//   The roll and the amount, because neither can be expressed in the DBC.
//
// Why a script.  Two independent reasons, either of which would be enough:
//
//   * Nothing rolls one defensive chance against a different damage type.
//     Block chance is consulted only by the melee attack table
//     (Unit.cpp:3912); it has no bearing on spell damage anywhere in core.
//
//   * SPELL_AURA_MOD_RATING_FROM_STAT (220), the sole conversion aura in
//     3.3.5, reads base STATS and cannot read a rating or a percentage.  That
//     limitation is already written up at the top of
//     WarriorParryConversions.cpp; this is the same wall from the other side.
//
// The absorb pool is deliberately enormous and canBeRecalculated is false, the
// MagicAbsorption.cpp idiom: this is a permanent passive, not a shield with a
// budget, so the pool must never be the thing that runs out.  Every hit is
// gated by the roll instead.
//
// No shield equipped means no absorb, for free: PLAYER_BLOCK_PERCENTAGE is 0
// without one (Player::UpdateBlockPercentage, StatSystem.cpp:633), so the roll
// simply never succeeds.  There is no explicit weapon check here on purpose.
//
// PLAYER_BLOCK_PERCENTAGE rather than
// GetTotalAuraModifier(SPELL_AURA_MOD_BLOCK_PERCENT): the field is the final
// figure the melee roll itself uses, so it already includes defense skill,
// block rating and every aura -- including Shield Block while it is up, which
// is the interaction that makes this talent interesting.

class spell_warr_spellshield : public AuraScript
{
    PrepareAuraScript(spell_warr_spellshield);

    bool Validate(SpellInfo const* spellInfo) override
    {
        return spellInfo->Effects[EFFECT_0].ApplyAuraName == SPELL_AURA_SCHOOL_ABSORB;
    }

    void CalculateAmount(AuraEffect const* /*aurEff*/, int32& amount, bool& canBeRecalculated)
    {
        canBeRecalculated = false;
        amount = 2000000000;
    }

    void Absorb(AuraEffect* /*aurEff*/, DamageInfo& dmgInfo, uint32& absorbAmount)
    {
        absorbAmount = 0;

        Player* player = GetTarget() ? GetTarget()->ToPlayer() : nullptr;
        if (!player)
            return;

        AuraEffect const* pctEff = GetEffect(EFFECT_1);
        int32 const pct = pctEff ? pctEff->GetAmount() : 0;
        if (pct <= 0)
            return;

        float const blockChance = player->GetFloatValue(PLAYER_BLOCK_PERCENTAGE);
        if (blockChance <= 0.0f || !roll_chance_f(blockChance))
            return;

        uint32 const blockValue = player->GetShieldBlockValue();
        absorbAmount = std::min(CalculatePct(blockValue, pct), dmgInfo.GetDamage());

        ACTEST("WAR.SPELLSHIELD", "rank={} blockChance={:.2f} blockValue={} pct={} damage={} -> absorbed={}",
            GetId(), blockChance, blockValue, pct, dmgInfo.GetDamage(), absorbAmount);
    }

    void Register() override
    {
        DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_warr_spellshield::CalculateAmount, EFFECT_0, SPELL_AURA_SCHOOL_ABSORB);
        OnEffectAbsorb     += AuraEffectAbsorbFn(spell_warr_spellshield::Absorb, EFFECT_0);
    }
};

class spell_warr_spellshield_loader : public SpellScriptLoader
{
public:
    spell_warr_spellshield_loader() : SpellScriptLoader("spell_warr_spellshield") { }

    AuraScript* GetAuraScript() const override
    {
        return new spell_warr_spellshield();
    }
};

void AddSC_war_spellshield()
{
    new spell_warr_spellshield_loader();
}
