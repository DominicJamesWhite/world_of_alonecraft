#include "AlonecraftTestLog.h"
#include "ScriptMgr.h"
#include "Player.h"
#include "SpellAuraEffects.h"
#include "SpellAuras.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "SpellScript.h"
#include "SpellScriptLoader.h"
#include "Unit.h"
#include "WarlockShards.h"
#include <algorithm>

using namespace Alonecraft::Warlock;

// Infernal Bargain (redesigned from Molten Skin) -- Warlock Destruction talent
// Single rank: 63349, buff 200405
//
// "Channel the souls you carry into the endless void for 5 sec. The void
//  doubles its price every second -- 1, then 2, 4, 8 and 16 Soul Shards -- and
//  the pact ends the instant you cannot pay. Each payment leaves you with
//  Infernal Bargain stacks equal to the shards spent, for 10 sec. While
//  channeling you are immune to all damage."
//
// The point of the exponential curve is the decision it forces. Going all the
// way costs 31 of the 32 shards you can carry, which is also the whole of
// Nathrezim Foresight (200507 mirrors the live shard count 1:1), so the panic
// button and the burst button are the same button and paying for one spends
// the other. Efficiency DECLINES with depth (1.00 -> 0.52 stacks per shard);
// what depth buys is burst compressed into one 10 sec window, plus more
// seconds of invulnerability.
//
// Handled by DBC (woa_2026_08_04_00.sql) -- almost everything:
//   63349 is a 5 second SELF channel (AttributesEx 64 =
//   SPELL_ATTR1_IS_SELF_CHANNELED, DurationIndex 28 = 5000ms) carrying two
//   auras on the caster:
//     Effect1 = SPELL_AURA_PERIODIC_DUMMY, EffectAmplitude 1000
//               -> exactly 5 ticks, the shard heartbeat driven below.
//     Effect2 = SPELL_AURA_SCHOOL_IMMUNITY, MiscValue 127 (all schools)
//               -> total damage immunity. It lives on the CHANNEL, so it
//                  expires with the channel on its own; no C++ touches it.
//                  Chosen over MOD_DAMAGE_PERCENT_TAKEN at -100 because that
//                  aura is not read by PERIODIC_DAMAGE_PERCENT ticks
//                  (SpellAuraEffects.cpp:6343) or by environmental/fall damage
//                  (Player.cpp:766, 14007), both of which check only
//                  SCHOOL_IMMUNITY.
//   EffectTriggerSpell1 holds the buff id (200405). SPELL_AURA_PERIODIC_DUMMY
//   never auto-triggers it, so it doubles as a data-driven spell -> buff map.
//
//   The buff has StackAmount 16 and fixed per-stack basepoints (+3% damage,
//   +1% spell crit). AuraEffect::CalculateAmount multiplies by the stack count
//   unconditionally (SpellAuraEffects.cpp:580), so putting the stacks on the
//   2^(N-1) curve is the whole of the doubling reward -- no custom basepoints.
//
//   The self-cast lands through our own immunity because
//   Unit::IgnoresSchoolImmunityFromFriendlyCaster (Unit.cpp:9856) exempts
//   friendly casters, and self counts as friendly. That exemption is switched
//   off by SPELL_ATTR1_IMMUNITY_TO_HOSTILE_AND_FRIENDLY_EFFECTS (0x10000), so
//   63349 must never carry it or the buff would silently stop applying.
//
// Handled here, and nothing else:
//   Charging 2^(N-1) shards on tick N, setting the stack count to match,
//   stopping early when the price outruns the hoard, and making sure the
//   reward window is a full 10 seconds however the channel ended.

// Tick N costs 2^(N-1) shards and leaves the buff at exactly 2^(N-1) stacks:
// 1, 2, 4, 8, 16. Total spent after N ticks is 2^N - 1.
static uint32 ShardsForTick(uint8 tick)
{
    return 1u << (tick - 1);
}

// Deepest tick a given hoard can reach: the largest N with 2^N - 1 <= shards.
// Trace only -- the tick handler decides affordability one tick at a time.
static uint8 AffordableTicks(uint32 shards)
{
    uint8 ticks = 0;
    while (ticks < 8 && (1u << (ticks + 1)) - 1u <= shards)
        ++ticks;

    return ticks;
}

class spell_warl_infernal_bargain : public SpellScript
{
    PrepareSpellScript(spell_warl_infernal_bargain);

    SpellCastResult CheckCast()
    {
        Player* player = GetCaster() ? GetCaster()->ToPlayer() : nullptr;
        if (!player)
            return SPELL_FAILED_DONT_REPORT;

        // Only the first tick's price is gated here -- one shard. How much
        // further the pact can go is the player's problem, tick by tick.
        uint32 const shards = GetSoulShardCount(player);
        uint8 const depth = AffordableTicks(shards);
        ACTEST("WARL.BARGAIN", "cast spell={} shards={} maxTicks={} maxStacks={} result={}",
            GetSpellInfo()->Id, shards, uint32(depth),
            depth ? ShardsForTick(depth) : 0u,
            shards ? "OK" : "FAILED_REAGENTS");

        if (!shards)
            return SPELL_FAILED_REAGENTS;

        return SPELL_CAST_OK;
    }

    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_warl_infernal_bargain::CheckCast);
    }
};

class spell_warl_infernal_bargain_aura : public AuraScript
{
    PrepareAuraScript(spell_warl_infernal_bargain_aura);

    bool Validate(SpellInfo const* spellInfo) override
    {
        return ValidateSpellInfo(
            { spellInfo->Effects[EFFECT_0].TriggerSpell });
    }

    // 1-based tick counter. One AuraScript instance is created per Aura
    // (Aura::LoadScripts) and dies with it, so this is per-channel state and
    // needs no keying by GUID.
    uint8 _tick = 0;

    // The buff, carried in the (otherwise inert) trigger spell slot.
    [[nodiscard]] uint32 GetBuffId() const
    {
        return GetSpellInfo()->Effects[EFFECT_0].TriggerSpell;
    }

    void HandleApply(AuraEffect const* /*aurEff*/,
        AuraEffectHandleModes /*mode*/)
    {
        _tick = 0;
    }

    // Pins the buff to an exact stack count. Two traps make this longer than
    // it looks:
    //   SPELLVALUE_AURA_STACK only SETS on a fresh aura -- Spell.cpp:3129 does
    //   `if (!refresh) SetStackAmount(n); else ModStackAmount(n)`, so on an
    //   aura that is already up the same call would ADD n instead.
    //   Neither SetStackAmount nor the SPELLVALUE path clamps to
    //   SpellInfo::StackAmount (SpellAuras.cpp:937); only ModStackAmount does.
    //   Hence the explicit cap, so a DBC/C++ mismatch degrades quietly instead
    //   of stacking past what the tuning assumed.
    void SetBuffStacks(Player* player, uint32 stacks)
    {
        uint32 const buffId = GetBuffId();
        if (!buffId)
            return;

        SpellInfo const* buffInfo = sSpellMgr->GetSpellInfo(buffId);
        uint8 const cap = buffInfo && buffInfo->StackAmount
            ? uint8(buffInfo->StackAmount) : uint8(1);
        uint8 const want = uint8(std::min<uint32>(stacks, cap));

        Aura* buff = player->GetAura(buffId, player->GetGUID());
        if (!buff)
        {
            // Applied at the right size in one go, so the player never sees it
            // flash at 1 stack first. Triggered casts skip
            // SetCurrentCastedSpell, so this does not clobber our own channel.
            player->CastCustomSpell(buffId, SPELLVALUE_AURA_STACK, int32(want), player, true);
            return;
        }

        // Plain cast to refresh the 10 sec window (and the client's aura
        // update), then pin the count -- the cast itself only adds one.
        player->CastSpell(player, buffId, true);
        if (Aura* refreshed = player->GetAura(buffId, player->GetGUID()))
            refreshed->SetStackAmount(want);
    }

    // Ends the channel without the client's red "Interrupted" flash and
    // without refunding the cooldown, then drops the immunity at once instead
    // of letting it ride out the remaining channel duration -- you do not get
    // to keep being invulnerable on a pact you stopped paying for.
    //
    // Safe to call from inside our own tick: AuraEffect::Update advances
    // m_periodicTimer before invoking the handler (SpellAuraEffects.cpp:942),
    // and aura deletion is deferred to _DeleteRemovedAuras (Unit.cpp:4995 ->
    // 4062), so this object outlives the call. The stacks live on a separate
    // aura and are untouched.
    void EndChannel(Unit* caster)
    {
        if (caster && caster->FindCurrentSpellBySpellId(GetId()))
            caster->FinishSpell(CURRENT_CHANNELED_SPELL, true);

        Remove(AURA_REMOVE_BY_EXPIRE);
    }

    void HandlePeriodic(AuraEffect const* /*aurEff*/)
    {
        Unit* target = GetTarget();
        Player* player = target ? target->ToPlayer() : nullptr;
        if (!player)
        {
            EndChannel(target);
            return;
        }

        uint8 const tick = ++_tick;
        uint32 const cost = ShardsForTick(tick);
        uint32 const have = GetSoulShardCount(player);

        // All or nothing, and the check has to happen HERE rather than on
        // ConsumeSoulShards' return value: it clamps the request to what you
        // actually carry and still reports success (WarlockShards.h:86), so
        // asking for 8 with 3 in the bag would eat the 3 and grant a full
        // tick. Harmless when every tick cost 1; an exploit now.
        //
        // Running dry ends the pact early, but everything already bought is
        // kept -- the stacks are applied as we go, not at the end.
        if (have < cost)
        {
            ACTEST("WARL.BARGAIN",
                "tick={} spell={} cost={} have={} CANNOT PAY -- ending channel early",
                uint32(tick), GetId(), cost, have);
            EndChannel(player);
            return;
        }

        ConsumeSoulShards(player, cost, "bargain-tick");

        // Stacks after this tick == shards paid for this tick == 2^(N-1).
        // Set, do not increment: the curve is 1 -> 2 -> 4 -> 8 -> 16.
        SetBuffStacks(player, cost);

        Aura const* buff = player->GetAura(GetBuffId(), player->GetGUID());
        ACTEST("WARL.BARGAIN",
            "tick={} spell={} paid={} buff={} stacks={} dmgDone={} spellCrit={} shardsLeft={}",
            uint32(tick), GetId(), cost, GetBuffId(),
            buff ? uint32(buff->GetStackAmount()) : 0u,
            buff && buff->GetEffect(EFFECT_0) ? buff->GetEffect(EFFECT_0)->GetAmount() : 0,
            buff && buff->GetEffect(EFFECT_1) ? buff->GetEffect(EFFECT_1)->GetAmount() : 0,
            GetSoulShardCount(player));
    }

    // Fires on every exit path -- channel completed, cancelled, moved, or run
    // dry. Stacks refresh the buff as they are applied, so a full channel
    // already ends with the full 10 seconds; a channel cut short at 1 shard
    // would otherwise be left with whatever was ticking down since 1 sec in.
    // This matters more under the exponential curve than it did before: a
    // run-dry exit can now land at any of five depths.
    void HandleRemove(AuraEffect const* /*aurEff*/,
        AuraEffectHandleModes /*mode*/)
    {
        Unit* target = GetTarget();
        if (!target)
            return;

        if (Aura* buff = target->GetAura(GetBuffId(), target->GetGUID()))
        {
            ACTEST("WARL.BARGAIN",
                "channel over buff={} stacks={} durBefore={} refreshedTo={}",
                GetBuffId(), uint32(buff->GetStackAmount()), buff->GetDuration(),
                buff->GetMaxDuration());

            buff->SetDuration(buff->GetMaxDuration());
        }
        else
        {
            ACTEST("WARL.BARGAIN", "channel over buff={} NOT PRESENT", GetBuffId());
        }
    }

    void Register() override
    {
        AfterEffectApply += AuraEffectApplyFn(
            spell_warl_infernal_bargain_aura::HandleApply,
            EFFECT_0, SPELL_AURA_PERIODIC_DUMMY, AURA_EFFECT_HANDLE_REAL);
        OnEffectPeriodic += AuraEffectPeriodicFn(
            spell_warl_infernal_bargain_aura::HandlePeriodic,
            EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
        AfterEffectRemove += AuraEffectRemoveFn(
            spell_warl_infernal_bargain_aura::HandleRemove,
            EFFECT_0, SPELL_AURA_PERIODIC_DUMMY, AURA_EFFECT_HANDLE_REAL);
    }
};

class spell_warl_infernal_bargain_loader : public SpellScriptLoader
{
public:
    spell_warl_infernal_bargain_loader() : SpellScriptLoader("spell_warl_infernal_bargain") { }

    SpellScript* GetSpellScript() const override
    {
        return new spell_warl_infernal_bargain();
    }

    AuraScript* GetAuraScript() const override
    {
        return new spell_warl_infernal_bargain_aura();
    }
};

void AddSC_warl_infernal_bargain()
{
    new spell_warl_infernal_bargain_loader();
}
