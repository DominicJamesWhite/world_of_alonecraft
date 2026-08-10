/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license:
 * https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE-AGPL3
 */

#include "AlonecraftTestLog.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SpellAuraEffects.h"
#include "SpellScript.h"
#include "SpellScriptLoader.h"
#include "Unit.h"

#include <algorithm>

// Barricade (replaces Concussion Blow) -- Warrior Protection talent 152
// Spells: 200651 ability / 200652 buff / 200653 heal -- woa_2026_08_09_31.sql
//
// TODO.md: "Become a living barricade, increasing your shield block chance and
//  value by 10% and converting each 2 additional points of rage into 1
//  additional percent (up to a maximum cost of 80 rage).  When you block with
//  Barricade active, you heal for 25% of your block value."
//
// Handled by DBC:
//   ManaCost 200 enforces the 20 rage floor, RecoveryTime 10000 the cooldown,
//   the shield requirement is EquippedItemClass on the ability, and the buff
//   already carries MOD_BLOCK_PERCENT and MOD_SHIELD_BLOCKVALUE_PCT because it
//   is cloned from Shield Block.  The proc event itself is a spell_proc row.
//
// Handled here:
//   The two numbers nothing static can know -- how much rage was actually
//   spent, and what the block value is at the moment of a block.
//
// This is the Execute pattern, and it is worth being precise about why it
// works.  Core has already deducted the base cost by the time an effect
// handler runs, so GetPower(POWER_RAGE) here is the rage remaining *after* the
// 20 rage floor was paid.  That is exactly what spell_warr_execute
// (spell_warrior.cpp) relies on when it does
// min(300 - CalcPowerCost(), GetPower(POWER_RAGE)), and the same arithmetic
// gives Barricade its overspend.
//
// Because the floor is 20 rage for 10%, the conversion is a flat 1% per 2 rage
// with no special case: the bonus is simply total rage spent / 2.  Halved from
// 1% per rage in woa_2026_08_10_04.sql -- the floor and the cap are unchanged,
// so the same dump buys half as much.

enum BarricadeSpells
{
    SPELL_WARRIOR_BARRICADE      = 200651,
    SPELL_WARRIOR_BARRICADE_BUFF = 200652,
    SPELL_WARRIOR_BARRICADE_HEAL = 200653
};

// 80 rage, in power units -- rage is stored at ten times its displayed value.
static constexpr int32 BARRICADE_MAX_COST = 800;

// Power units of rage per point of bonus percentage.  20 units is 2 displayed
// rage, so the 200-unit floor buys 10% and the 800-unit cap buys 40%.  This is
// the one Barricade number that cannot live in the DBC: it multiplies rage
// spent, which is only known once the effect handler runs.
static constexpr int32 BARRICADE_RAGE_PER_PCT = 20;

// Shield Block's CastKit, read off SpellVisual 3442 -- the shield-raise
// animation.  200651 already carries SpellVisual 3442 (it is cloned from
// Shield Block), but 3442 defines ONLY a CastKit, and the client has no reason
// to play a cast visual for a spell whose single effect is SPELL_EFFECT_DUMMY:
// nothing it can see happens.  Sending the kit explicitly sidesteps that.
static constexpr uint32 SHIELD_BLOCK_CAST_KIT = 3053;

// ---------------------------------------------------------------------------
//  200651: spend the rage, apply the buff
// ---------------------------------------------------------------------------

class spell_warr_barricade : public SpellScript
{
    PrepareSpellScript(spell_warr_barricade);

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_WARRIOR_BARRICADE_BUFF });
    }

    void HandleDummy(SpellEffIndex /*effIndex*/)
    {
        Player* player = GetCaster() ? GetCaster()->ToPlayer() : nullptr;
        if (!player)
            return;

        SpellInfo const* spellInfo = GetSpellInfo();
        int32 const baseCost = spellInfo->CalcPowerCost(player, SpellSchoolMask(spellInfo->SchoolMask));

        // Whatever is left over, up to the cap. Never negative: a base cost at
        // or above the cap would otherwise wrap into a huge refund.
        int32 const overspend = std::max(0, std::min<int32>(BARRICADE_MAX_COST - baseCost,
            int32(player->GetPower(POWER_RAGE))));

        player->SetPower(POWER_RAGE, uint32(std::max(0, int32(player->GetPower(POWER_RAGE)) - overspend)));

        int32 pct = (baseCost + overspend) / BARRICADE_RAGE_PER_PCT;
        if (pct <= 0)
            return;

        ACTEST("WARR.BARRICADE", "cast baseCost={} overspend={} pct={} rageLeft={}",
            baseCost, overspend, pct, player->GetPower(POWER_RAGE));

        // Both block chance and block value take the same value, so the buff
        // gets it twice. Effect 2 is deliberately left as nullptr so it keeps
        // its DBC base points -- that is where the heal percentage lives.
        player->CastCustomSpell(player, SPELL_WARRIOR_BARRICADE_BUFF, &pct, &pct, nullptr, true);
        player->SendPlaySpellVisual(SHIELD_BLOCK_CAST_KIT);
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_warr_barricade::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
    }
};

// ---------------------------------------------------------------------------
//  200652: heal for block value on every block
// ---------------------------------------------------------------------------

class spell_warr_barricade_block_heal : public AuraScript
{
    PrepareAuraScript(spell_warr_barricade_block_heal);

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_WARRIOR_BARRICADE_HEAL });
    }

    void HandleProc(AuraEffect const* aurEff, ProcEventInfo& /*eventInfo*/)
    {
        PreventDefaultAction();

        Player* player = GetTarget() ? GetTarget()->ToPlayer() : nullptr;
        if (!player)
            return;

        // Reads the CURRENT block value, so it already includes this very
        // buff's MOD_SHIELD_BLOCKVALUE_PCT -- GetShieldBlockValue multiplies by
        // m_auraBasePctMod[SHIELD_BLOCK_VALUE] (Player.cpp:5115). An 80 rage
        // Barricade therefore heals for 40% more per block as well as blocking
        // more often, which is the intended pay-off for dumping the rage.
        //
        // That double-dip is why this is a fraction of block value and not all
        // of it. The percentage lives in Effect3's base points so it can be
        // retuned without touching code.
        int32 const amount = CalculatePct(int32(player->GetShieldBlockValue()), aurEff->GetAmount());
        if (amount <= 0)
            return;

        ACTEST("WARR.BARRICADE", "block heal amount={}", amount);
        player->CastCustomSpell(player, SPELL_WARRIOR_BARRICADE_HEAL, &amount, nullptr, nullptr, true, nullptr, aurEff);
    }

    void Register() override
    {
        OnEffectProc += AuraEffectProcFn(spell_warr_barricade_block_heal::HandleProc, EFFECT_2, SPELL_AURA_DUMMY);
    }
};

class spell_warr_barricade_loader : public SpellScriptLoader
{
public:
    spell_warr_barricade_loader() : SpellScriptLoader("spell_warr_barricade") { }

    SpellScript* GetSpellScript() const override
    {
        return new spell_warr_barricade();
    }
};

class spell_warr_barricade_block_heal_loader : public SpellScriptLoader
{
public:
    spell_warr_barricade_block_heal_loader() : SpellScriptLoader("spell_warr_barricade_block_heal") { }

    AuraScript* GetAuraScript() const override
    {
        return new spell_warr_barricade_block_heal();
    }
};

void AddSC_war_barricade()
{
    new spell_warr_barricade_loader();
    new spell_warr_barricade_block_heal_loader();
}
