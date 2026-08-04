#include "AlonecraftTestLog.h"
#include "ScriptMgr.h"
#include "Pet.h"
#include "Player.h"
#include "SpellAuraEffects.h"
#include "SpellAuras.h"
#include "SpellInfo.h"
#include "SpellScript.h"
#include "SpellScriptLoader.h"
#include "Unit.h"
#include "WarlockShards.h"
#include <algorithm>

using namespace Alonecraft::Warlock;

// Sacrifice the Weak (redesigned from Empowered Imp) -- Warlock Destruction
// Single rank: 47220 (talent 2045; ranks 2/3 were dropped in
// woa_2026_08_03_02.sql because every rank granted the same 1% per shard).
//
// "Sacrifice your demon to a new master in the Twisting Nether. In return
//  they grant you Nathrezim Foresight, reducing damage taken and increasing
//  the critical effect chance of your spells by 1% for each Soul Shard in
//  your possession."
//
// Handled by DBC:
//   The talent becomes an ACTIVE instant self-cast whose Effect1 is
//   SPELL_EFFECT_DUMMY.
//   Spell 200403 "Nathrezim Pact" is the permanent, hidden driver aura:
//     Effect3 = PERIODIC_DUMMY, 1s amplitude -- the resync heartbeat.
//   Spell 200507 "Nathrezim Foresight" is the VISIBLE stacking buff:
//     StackAmount 32, permanent
//     Effect1 = MOD_DAMAGE_PERCENT_TAKEN, -1 per stack
//     Effect2 = MOD_SPELL_CRIT_CHANCE,    +1 per stack
//
// Two auras rather than one because an aura cannot sit at zero stacks, and
// because a driver that must survive a shardless moment cannot be the same
// object as a buff that must disappear when the last shard is spent.
//
// Per-stack scaling is free: AuraEffect::CalculateAmount ends with
// `amount *= GetBase()->GetStackAmount()` (SpellAuraEffects.cpp:580), and
// Aura::SetStackAmount recalculates every effect and pushes a client update
// (SpellAuras.cpp:937).  17 shards -> 17 stacks -> -17% taken / +17% crit.
//
// Handled by this script:
//   1. Sacrificing the pet and applying the pact.
//   2. Keeping the buff's stack count in step with the LIVE shard count.
//      Prior art: spell_ahn_kahet_swarmer_aura (boss_elder_nadox.cpp) --
//      apply-if-missing via SPELLVALUE_AURA_STACK, else SetStackAmount, else
//      remove at zero.
//
// A 1 Hz poll rather than an item hook: PlayerScript has hooks for items
// entering the bags but none for an item being destroyed, so spending a
// shard is only observable by looking.
//
// NOTE: RemovePet's third parameter is `returnreagent`.  It MUST stay false
// -- Player::RemovePet refunds the summoning spell's reagents (i.e. a Soul
// Shard) when true, which would turn this talent into a shard printer.

enum SacrificeTheWeakSpells
{
    SACRIFICE_THE_WEAK    = 47220,
    NATHREZIM_PACT        = 200403,
    NATHREZIM_FORESIGHT   = 200507,
};

// -- The cast: sacrifice the demon -------------------------------------------

class spell_warl_sacrifice_the_weak : public SpellScript
{
    PrepareSpellScript(spell_warl_sacrifice_the_weak);

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ NATHREZIM_PACT });
    }

    SpellCastResult CheckCast()
    {
        Player* player = GetCaster() ? GetCaster()->ToPlayer() : nullptr;
        if (!player)
            return SPELL_FAILED_DONT_REPORT;

        Pet* pet = player->GetPet();
        ACTEST("WARL.STW", "cast attempt pet={} result={}",
            pet ? pet->GetName() : "none", pet ? "OK" : "FAILED_NO_PET");

        if (!pet)
            return SPELL_FAILED_NO_PET;

        return SPELL_CAST_OK;
    }

    void HandleDummy(SpellEffIndex /*effIndex*/)
    {
        Player* player = GetCaster() ? GetCaster()->ToPlayer() : nullptr;
        if (!player)
            return;

        Pet* pet = player->GetPet();
        if (!pet)
            return;

        uint32 const shardsBefore = GetSoulShardCount(player);
        std::string const petName = pet->GetName();

        // returnreagent = false: the demon is given away, not dismissed --
        // no Soul Shard comes back.
        player->RemovePet(pet, PET_SAVE_NOT_IN_SLOT, false);

        player->CastSpell(player, NATHREZIM_PACT, true);

        // shardsAfter MUST equal shardsBefore: RemovePet with returnreagent
        // true would refund a shard and turn this talent into a shard printer.
        ACTEST("WARL.STW", "sacrificed pet={} shardsBefore={} shardsAfter={} pact={}",
            petName, shardsBefore, GetSoulShardCount(player),
            player->HasAura(NATHREZIM_PACT) ? "applied" : "MISSING");
    }

    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_warl_sacrifice_the_weak::CheckCast);
        OnEffectHit += SpellEffectFn(spell_warl_sacrifice_the_weak::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
    }
};

class spell_warl_sacrifice_the_weak_loader : public SpellScriptLoader
{
public:
    spell_warl_sacrifice_the_weak_loader() : SpellScriptLoader("spell_warl_sacrifice_the_weak") { }

    SpellScript* GetSpellScript() const override
    {
        return new spell_warl_sacrifice_the_weak();
    }
};

// -- The pact: keep the visible buff's stacks on the live shard count --------

class spell_warl_nathrezim_pact : public AuraScript
{
    PrepareAuraScript(spell_warl_nathrezim_pact);

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ NATHREZIM_FORESIGHT });
    }

    static void DropForesight(Player* player)
    {
        player->RemoveAurasDueToSpell(NATHREZIM_FORESIGHT);
    }

    void HandlePeriodic(AuraEffect const* /*aurEff*/)
    {
        Unit* target = GetTarget();
        if (!target)
            return;

        Player* player = target->ToPlayer();
        if (!player)
        {
            Remove();
            return;
        }

        // Losing the talent breaks the pact.  ActivateSpec only strips auras
        // belonging to talent spells (Player.cpp:15343) -- the pact is cast by
        // the talent, not owned by it, so a spec swap leaves it (and with it
        // Nathrezim Foresight) running for a spec that no longer has the
        // talent.  Same for a plain talent reset.  Worst case the buff lingers
        // one heartbeat.
        if (!player->HasTalent(SACRIFICE_THE_WEAK, player->GetActiveSpec()))
        {
            ACTEST("WARL.STW", "talent no longer active (spec {}) -- Nathrezim Pact broken",
                uint32(player->GetActiveSpec()) + 1);
            Remove();
            return;
        }

        // Taking a new demon breaks the pact.
        if (player->GetPet())
        {
            ACTEST("WARL.STW", "new demon summoned -- Nathrezim Pact broken");
            Remove();
            return;
        }

        uint32 const shards = std::min(GetSoulShardCount(player), SOUL_SHARD_MAX);
        Aura* buff = player->GetAura(NATHREZIM_FORESIGHT, player->GetGUID());

        // No shards, no foresight -- an aura cannot sit at zero stacks.
        if (!shards)
        {
            if (buff)
            {
                ACTEST("WARL.STW", "shards=0 -- Nathrezim Foresight removed");
                DropForesight(player);
            }
            return;
        }

        if (!buff)
        {
            // SPELLVALUE_AURA_STACK applies it at the right size in one go,
            // so the player never sees it flash at 1 stack first.
            player->CastCustomSpell(NATHREZIM_FORESIGHT, SPELLVALUE_AURA_STACK, int32(shards), player, true);
            buff = player->GetAura(NATHREZIM_FORESIGHT, player->GetGUID());

            ACTEST("WARL.STW", "Nathrezim Foresight applied shards={} stacks={}",
                shards, buff ? uint32(buff->GetStackAmount()) : 0u);
            return;
        }

        // Only touch the aura when the count actually moved -- a 1 Hz tick
        // with a stable shard count should cost essentially nothing.
        uint32 const before = buff->GetStackAmount();
        if (before == shards)
            return;

        buff->SetStackAmount(uint8(shards));

        ACTEST("WARL.STW", "Nathrezim Foresight restacked {} -> {} (shards={} damageTaken={} crit={})",
            before, uint32(buff->GetStackAmount()), shards,
            buff->GetEffect(EFFECT_0) ? buff->GetEffect(EFFECT_0)->GetAmount() : 0,
            buff->GetEffect(EFFECT_1) ? buff->GetEffect(EFFECT_1)->GetAmount() : 0);
    }

    // The buff must never outlive the pact -- logout, a new demon, or a
    // manual cancel all land here.
    void HandleRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        if (Unit* target = GetTarget())
            if (Player* player = target->ToPlayer())
                DropForesight(player);
    }

    void Register() override
    {
        OnEffectPeriodic += AuraEffectPeriodicFn(spell_warl_nathrezim_pact::HandlePeriodic, EFFECT_2, SPELL_AURA_PERIODIC_DUMMY);
        AfterEffectRemove += AuraEffectRemoveFn(spell_warl_nathrezim_pact::HandleRemove, EFFECT_2, SPELL_AURA_PERIODIC_DUMMY, AURA_EFFECT_HANDLE_REAL);
    }
};

class spell_warl_nathrezim_foresight_loader : public SpellScriptLoader
{
public:
    spell_warl_nathrezim_foresight_loader() : SpellScriptLoader("spell_warl_nathrezim_foresight") { }

    AuraScript* GetAuraScript() const override
    {
        return new spell_warl_nathrezim_pact();
    }
};

void AddSC_warl_sacrifice_the_weak()
{
    new spell_warl_sacrifice_the_weak_loader();
    new spell_warl_nathrezim_foresight_loader();
}
