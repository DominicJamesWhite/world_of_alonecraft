/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license: https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE-AGPL3
 */

/*
 * Warlock (Affliction): Fel Interdiction and Malfeasance.
 *
 * Fel Interdiction (talent 1668, spell 30054) defers half of every hit the
 * warlock takes while Fel Armor is up into Mark of Gul'dan (200520), a
 * stacking self-DoT that bleeds the deferred total back over 10 seconds.
 * Dealing damage with Drain Soul clears a stack; with Malfeasance
 * (200521 / 200522) so do Drain Life, Shadow Bolt and Haunt.
 *
 * This is the Ember Scars mechanic (MoltenArmor.cpp) with different gates.
 * Stacks are a counter, not a multiplier: the whole deferred pool lives in
 * EFFECT_0's amount, and each cleared stack removes its share of it.
 *
 * The one structural difference is the clearing path.  Ember Scars needs a
 * three-hop 200041 -> 200040 -> AuraScript relay because AzerothCore has no
 * "on spell crit" hook.  Drain Soul / Drain Life / Shadow Bolt / Haunt damage
 * is reachable by an ordinary spell_proc, so each talent aura is its own proc
 * carrier and there are no relay spells.
 */

#include "ScriptMgr.h"
#include "Player.h"
#include "SpellAuraEffects.h"
#include "SpellAuras.h"
#include "SpellScript.h"
#include "SpellScriptLoader.h"
#include "Unit.h"

#define MARK_OF_GULDAN_ID       200520
#define FEL_INTERDICTION_TALENT 30054
#define MALFEASANCE_RANK_1      200521
#define MALFEASANCE_RANK_2      200522

// Fel Armor ranks, including the duplicate rank 2 ids the client can hold.
static uint32 const FEL_ARMOR_IDS[] = { 28176, 28189, 44520, 44977, 47892, 47893 };

// Window the deferred damage is spread across, and the stack cap.  Both must
// match spell 200520: DurationIndex 1 is 10s, StackAmount is 10.
static uint32 const MARK_DURATION_SECONDS = 10;
static uint8 const MARK_MAX_STACKS = 10;

static bool HasFelArmor(Player* player)
{
    for (uint32 spellId : FEL_ARMOR_IDS)
        if (player->HasAura(spellId))
            return true;

    return false;
}

/*
 * Fold newly deferred damage into the Mark of Gul'dan pool.
 *
 * The pool is stored as a per-tick amount, so the outstanding total has to be
 * reconstituted (per-tick x remaining seconds) before the new contribution is
 * added and the sum redistributed across a fresh full window.  Damage already
 * ticked is therefore never re-applied.
 */
static void AddOrUpdateMarkOfGuldan(Player* player, uint32 damageToAdd)
{
    Aura* mark = player->GetAura(MARK_OF_GULDAN_ID);
    uint64 outstandingTotal = 0;
    bool newlyApplied = false;

    if (mark)
    {
        uint32 currentTickDamage = 0;
        if (AuraEffect* effect = mark->GetEffect(EFFECT_0))
            currentTickDamage = effect->GetAmount();

        int32 remainingMs = mark->GetDuration();
        if (remainingMs > 0)
        {
            uint32 remainingSec = static_cast<uint32>(remainingMs / 1000);
            if (remainingSec > MARK_DURATION_SECONDS)
                remainingSec = MARK_DURATION_SECONDS;

            outstandingTotal = static_cast<uint64>(currentTickDamage) * remainingSec;
        }
    }
    else
    {
        player->AddAura(MARK_OF_GULDAN_ID, player);
        mark = player->GetAura(MARK_OF_GULDAN_ID);
        newlyApplied = true;
    }

    if (!mark)
        return;

    uint64 combinedTotal = outstandingTotal + static_cast<uint64>(damageToAdd);
    uint32 newTickDamage = static_cast<uint32>(combinedTotal / MARK_DURATION_SECONDS);

    uint8 newStacks = newlyApplied ? 1 : std::min<uint8>(mark->GetStackAmount() + 1, MARK_MAX_STACKS);

    mark->SetStackAmount(newStacks);
    mark->RefreshDuration();

    if (AuraEffect* effect = mark->GetEffect(EFFECT_0))
        effect->ChangeAmount(newTickDamage);
}

/*
 * Clear stacks, and with them their share of the outstanding damage.
 *
 * Every stack holds an equal share of the CURRENT pool, so clearing k of n
 * stacks leaves exactly (n - k) / n of the damage behind: 4 of 10 removes
 * 40%, 1 of 2 removes 50%, and clearing every stack removes all of it.
 *
 * Dividing by the live stack count rather than by MARK_MAX_STACKS is what
 * makes that hold.  A fixed 1/MARK_MAX_STACKS share only lines up while the
 * aura is at full stacks; below that it under-removes, and then the last
 * clear silently wipes whatever is left over -- at 2 stacks the first clear
 * would take 10% and the second the remaining 90%.
 *
 * The share is of the pool, not of the hits that filled it.  Two big hits and
 * ten small ones can hold the same damage, and in both cases one stack is
 * worth one stack's share of it.
 *
 * The clamp stops currentStacks - stacksToRemove underflowing the uint8 -- 2
 * stacks minus a 4-stack Soulshatter would otherwise wrap to 254.
 */
static void RemoveMarkOfGuldanStacks(Player* player, uint8 stacksToRemove = 1)
{
    if (!player || !stacksToRemove)
        return;

    Aura* mark = player->GetAura(MARK_OF_GULDAN_ID);
    if (!mark || mark->GetStackAmount() == 0)
        return;

    AuraEffect* effect = mark->GetEffect(EFFECT_0);
    if (!effect)
        return;

    uint8 currentStacks = mark->GetStackAmount();
    if (stacksToRemove > currentStacks)
        stacksToRemove = currentStacks;

    uint8 newStacks = currentStacks - stacksToRemove;

    // Scale in one step rather than via a percentage: no double rounding, and
    // no assumption that 100 / MARK_MAX_STACKS divides evenly.
    uint32 newTickDamage = static_cast<uint32>(
        (static_cast<uint64>(effect->GetAmount()) * newStacks) / currentStacks);

    if (newStacks == 0 || newTickDamage == 0)
    {
        mark->Remove();
        return;
    }

    mark->SetStackAmount(newStacks);
    effect->ChangeAmount(newTickDamage);
}

class spell_warl_fel_interdiction_damage : public UnitScript
{
public:
    spell_warl_fel_interdiction_damage() : UnitScript("spell_warl_fel_interdiction_damage") { }

    void OnDamage(Unit* attacker, Unit* victim, uint32& damage) override
    {
        // This runs for every damage event on the server -- reject cheaply.
        Player* player = victim->ToPlayer();
        if (!player || player->getClass() != CLASS_WARLOCK)
            return;

        // Mark of Gul'dan's own ticks must not be deferred again.
        if (attacker == player && player->HasAura(MARK_OF_GULDAN_ID))
            return;

        AuraEffect* talent = player->GetAuraEffect(FEL_INTERDICTION_TALENT, EFFECT_0);
        if (!talent)
            return;

        if (!HasFelArmor(player))
            return;

        // The stagger percentage lives in the talent's DBC row, not here.
        uint32 deferredDamage = CalculatePct(damage, talent->GetAmount());
        if (!deferredDamage)
            return;

        damage -= deferredDamage;

        AddOrUpdateMarkOfGuldan(player, deferredDamage);
    }
};

// Mark of Gul'dan (200520): pay out the deferred pool one tick at a time.
class spell_warl_mark_of_guldan_AuraScript : public AuraScript
{
    PrepareAuraScript(spell_warl_mark_of_guldan_AuraScript);

    void OnPeriodicTick(AuraEffect const* aurEff)
    {
        Unit* target = GetTarget();
        Unit* caster = GetCaster();
        if (!target || !caster)
            return;

        uint32 tickDamage = aurEff->GetAmount();
        if (!tickDamage)
        {
            GetAura()->Remove();
            return;
        }

        Unit::DealDamage(caster, target, tickDamage, nullptr, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_SHADOW, aurEff->GetSpellInfo(), false);
    }

    void Register() override
    {
        OnEffectPeriodic += AuraEffectPeriodicFn(spell_warl_mark_of_guldan_AuraScript::OnPeriodicTick, EFFECT_0, SPELL_AURA_PERIODIC_DAMAGE);
    }
};

class spell_warl_mark_of_guldan : public SpellScriptLoader
{
public:
    spell_warl_mark_of_guldan() : SpellScriptLoader("spell_warl_mark_of_guldan") { }

    AuraScript* GetAuraScript() const override
    {
        return new spell_warl_mark_of_guldan_AuraScript();
    }
};

/*
 * One stack per proc.  Which spells qualify, and at what chance, is entirely
 * the spell_proc row: Drain Soul at 100% for Fel Interdiction, Drain Life /
 * Shadow Bolt / Haunt at 10% or 20% for Malfeasance.
 */
class spell_warl_mark_of_guldan_clear_AuraScript : public AuraScript
{
    PrepareAuraScript(spell_warl_mark_of_guldan_clear_AuraScript);

    void OnProc(AuraEffect const* /*aurEff*/, ProcEventInfo& /*eventInfo*/)
    {
        PreventDefaultAction();

        RemoveMarkOfGuldanStacks(GetTarget()->ToPlayer());
    }

    void Register() override
    {
        OnEffectProc += AuraEffectProcFn(spell_warl_mark_of_guldan_clear_AuraScript::OnProc, EFFECT_0, SPELL_AURA_DUMMY);
    }
};

class spell_warl_fel_interdiction : public SpellScriptLoader
{
public:
    spell_warl_fel_interdiction() : SpellScriptLoader("spell_warl_fel_interdiction") { }

    AuraScript* GetAuraScript() const override
    {
        return new spell_warl_mark_of_guldan_clear_AuraScript();
    }
};

class spell_warl_malfeasance : public SpellScriptLoader
{
public:
    spell_warl_malfeasance() : SpellScriptLoader("spell_warl_malfeasance") { }

    AuraScript* GetAuraScript() const override
    {
        return new spell_warl_mark_of_guldan_clear_AuraScript();
    }
};

/*
 * Look up an effect on whichever Malfeasance rank the player has, higher rank
 * first.  Returns nullptr when the talent is untrained.
 */
static AuraEffect* GetMalfeasanceEffect(Player* player, uint8 effIndex)
{
    if (AuraEffect* effect = player->GetAuraEffect(MALFEASANCE_RANK_2, effIndex))
        return effect;

    return player->GetAuraEffect(MALFEASANCE_RANK_1, effIndex);
}

/*
 * Soulshatter clears stacks outright -- 2 with Fel Interdiction, 4 once
 * Malfeasance is taken -- and Malfeasance also cuts its cooldown, 180s base
 * down to 120s at rank 1 and 60s at rank 2.
 *
 * Neither number is hardcoded.  The stack count is EFFECT_1's amount on
 * whichever talent the player has (Malfeasance replaces Fel Interdiction's
 * value rather than adding to it) and the cooldown reduction is EFFECT_2's
 * amount, in seconds, on Malfeasance alone.  Both live in the DBC and are the
 * same values the tooltip renders as $s2 and $s3.
 *
 * The two are looked up independently: a warlock with Fel Interdiction but
 * not Malfeasance gets the 2-stack clear and no cooldown change.
 *
 * Reducing the cooldown from AfterCast is safe because Spell::finish calls
 * SendSpellCooldown() before CallScriptAfterCastHandlers(), so the cooldown
 * already exists by the time this runs.  ModifySpellCooldown also sends
 * SMSG_MODIFY_COOLDOWN, so the client's own timer follows.
 *
 * AfterCast rather than OnEffectHitTarget: core's own spell_warl_soulshatter
 * drops threat only against a target that already has the caster on its
 * threat list, and clearing your own debuff should not depend on that.
 */
class spell_warl_soulshatter_mark : public SpellScript
{
    PrepareSpellScript(spell_warl_soulshatter_mark);

    void HandleAfterCast()
    {
        Player* player = GetCaster()->ToPlayer();
        if (!player)
            return;

        AuraEffect* source = GetMalfeasanceEffect(player, EFFECT_1);
        if (!source)
            source = player->GetAuraEffect(FEL_INTERDICTION_TALENT, EFFECT_1);

        if (source)
            RemoveMarkOfGuldanStacks(player, static_cast<uint8>(source->GetAmount()));

        if (AuraEffect* reduction = GetMalfeasanceEffect(player, EFFECT_2))
            if (int32 seconds = reduction->GetAmount())
                player->ModifySpellCooldown(GetSpellInfo()->Id, -seconds * IN_MILLISECONDS);
    }

    void Register() override
    {
        AfterCast += SpellCastFn(spell_warl_soulshatter_mark::HandleAfterCast);
    }
};

class spell_warl_soulshatter_mark_loader : public SpellScriptLoader
{
public:
    spell_warl_soulshatter_mark_loader() : SpellScriptLoader("spell_warl_soulshatter_mark") { }

    SpellScript* GetSpellScript() const override
    {
        return new spell_warl_soulshatter_mark();
    }
};

void AddSC_warl_fel_interdiction()
{
    new spell_warl_fel_interdiction_damage();
    new spell_warl_mark_of_guldan();
    new spell_warl_fel_interdiction();
    new spell_warl_malfeasance();
    new spell_warl_soulshatter_mark_loader();
}
