/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license:
 * https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE-AGPL3
 */

#include "SimRunner.h"

#include "CharacterCache.h"
#include "Config.h"
#include "Creature.h"
#include "CreatureAI.h"
#include "DBCStores.h"
#include "DatabaseEnv.h"
#include "Item.h"
#include "GameTime.h"
#include "Log.h"
#include "Map.h"
#include "MapMgr.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Opcodes.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "CombatAI.h"
#include "PassiveAI.h"
#include "Pet.h"
#include "Player.h"
#include "Random.h"
#include "ScriptMgr.h"
#include "SpellAuraEffects.h"
#include "SpellAuras.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "StringFormat.h"
#include "TemporarySummon.h"
#include "Unit.h"
#include "UnitScript.h"
#include "World.h"
#include "WorldScript.h"

#ifdef MOD_PLAYERBOTS
#include "AiFactory.h"
#include "AiObjectContext.h"
#include "PlayerbotAI.h"
#include "PlayerbotAIConfig.h"
#include "PlayerbotFactory.h"
#include "PlayerbotMgr.h"
#include "RandomPlayerbotMgr.h"
#endif

#include <algorithm>
#include <cctype>
#include <fstream>
#include <map>
#include <set>
#include <vector>

namespace
{
    // -- Configuration, parsed from argv ------------------------------------
    //
    // sConfigMgr->GetArguments() is the process's raw command line, so the
    // simulator reads its own flags without the core needing to know they
    // exist.  Main.cpp registers only --sim (and parses with allow_unregistered),
    // which keeps the core's knowledge of the simulator down to supplying a
    // clock.

    struct SimConfig
    {
        bool        active      = false;
        std::string character;
        uint32      seconds     = 60;
        uint32      seed        = 1;
        uint32      targetEntry = 0;
        std::string outPath     = "sim_result.json";

        // Actor configuration. Empty spec means "use the character exactly as it
        // is stored", which is only ever useful for reproducing a hand-made
        // setup -- a balance run always names a spec.
        std::string spec;
        uint32      level       = 0;   // 0 = keep the character's own level

        // A fixed equipment set, replacing the one PlayerbotFactory rolls.
        //   item[:enchant[:gem,gem,gem]];item...
        // See EquipFixedGear for why this exists and what the ids mean.
        std::string gear;

        // Iterations run inside one process. Boot is ~28 wall seconds and a
        // 60-virtual-second fight is ~1.5, so a second process per iteration
        // would spend 95% of its life loading the world.
        uint32      iterations  = 1;

        // Virtual seconds between the reset and the next fight, for the bot to
        // re-apply its own buffs. Cheap at 80x realtime; see SIM_BETWEEN.
        uint32      buffSeconds = 30;

        // GM Island. Verified to have maps, vmaps *and* mmaps extracted --
        // Development Land (451) has none of the three, so it cannot be used.
        uint32 mapId = 1;
        float  x = 16226.6f, y = 16257.0f, z = 13.2f, o = 0.0f;
        // How far from the actor the target is summoned. Five yards suits melee
        // and casters alike, but it is inside a hunter's minimum range: a hunter
        // at five yards cannot use a single one of its shots, and does not fall
        // back to melee either -- the first full matrix measured all three
        // hunter specs at 63-92 DPS, every point of it from the pet. Overridden
        // per spec by tools/sim_specs.py.
        float  range = 5.0f;
    };

    SimConfig s_cfg;

    enum SimState
    {
        SIM_IDLE,
        SIM_LOGIN_REQUESTED,
        SIM_AWAIT_LOGIN,
        SIM_AWAIT_ARENA,
        SIM_RUNNING,
        SIM_BETWEEN,
        SIM_DONE
    };

    // One fight. Kept per iteration rather than summed, so tools/sim.py can see
    // the spread and an iteration that measured nothing is visible as a zero
    // rather than quietly dragging the mean down.
    // How a fight ended. Distinguishing these is the whole of solo-clear
    // viability, and a mean that mixes them is meaningless -- "died at 40s" and
    // "killed it at 40s" are opposite results with the same duration.
    enum FightOutcome : uint8
    {
        OUTCOME_TIMEOUT     = 0,   // ran the clock out, nobody died
        OUTCOME_ACTOR_DIED  = 1,
        OUTCOME_TARGET_DIED = 2
    };

    // -- Per-ability attribution -------------------------------------------
    //
    // Only one damage hook carries a SpellInfo (ModifySpellDamageTaken), and it
    // fires *before* absorb and resist, while the authoritative final amount
    // arrives later in OnDamage with no spell attached. Neither is sufficient
    // alone, so the Modify* hooks latch "what is about to be dealt, and by what"
    // and OnDamage consumes that latch.
    //
    // The latch is only consumed when the attacker and victim match, so a
    // nested cast that interleaves cannot silently steal another spell's
    // attribution -- it falls through to spell id 0 and is counted as
    // unattributed. That count is reported: a breakdown that quietly misfiles
    // damage is worse than one that admits it could not tell.

    enum DamageKind : uint8
    {
        DK_UNKNOWN  = 0,
        DK_MELEE    = 1,
        DK_SPELL    = 2,
        DK_PERIODIC = 3
    };

    // Damage and healing share the latch, so a latched entry has to say which it
    // is. Without that discriminator they collide: SpellAuraEffects.cpp:6654
    // calls ModifyPeriodicDamageAurasTick on a *heal* tick, one line before
    // ModifyHealReceived, so Renew and Rejuvenation arrive at the damage hook.
    enum EventKind : uint8
    {
        EK_DAMAGE = 0,
        EK_HEAL   = 1
    };

    struct DamageContext
    {
        ObjectGuid attacker;
        ObjectGuid victim;
        uint32     spellId = 0;
        uint32     ms      = 0;
        uint32     raw     = 0;   // the amount as the Modify* hook saw it
        uint8      kind    = DK_UNKNOWN;
        uint8      evKind  = EK_DAMAGE;
        bool       valid   = false;
    };

    // Pushes that evicted a slot still holding a live, unconsumed latch. Nonzero
    // means attribution may have been lost and the ring wants more slots.
    uint32 s_latchOverflow = 0;

    // The virtual clock, advanced once per world tick in OnUpdate. Declared here
    // rather than with the rest of the run state because the latch below stamps
    // and expires against it.
    uint32 s_elapsedMs = 0;

    // A ring, not a single slot.
    //
    // Unit::CalcAbsorbResist recurses into Unit::DealDamage for split damage
    // (Unit.cpp:2607, 2680) and shared damage (Unit.cpp:1116), so a nested
    // OnDamage for a *third* unit lands before the outer event's. With one slot
    // that nested event unconditionally invalidated the latch, silently turning
    // the parent into unattributed damage -- Soul Link is exactly this shape, and
    // demonology is a spec we tune.
    //
    // Take() is newest-first and invalidates only the slot it consumes, so an
    // event we do not count can no longer destroy an event we do.
    //
    // A latch is only honoured within the tick that pushed it. That bound is
    // exact rather than approximate: a Modify* hook and the DealDamage it
    // precedes are the same call stack, and s_elapsedMs only advances in
    // OnUpdate, so a surviving latch from an earlier tick is by construction not
    // this event's. It has to be enforced, because ModifyMeleeDamage fires 17
    // lines before RollMeleeOutcomeAgainst (Unit.cpp:1798 vs 1815) -- every miss,
    // dodge and parry leaves a latch that no OnDamage will ever consume, and
    // without the tick bound a later spell would claim one and be filed as
    // melee. Unattributed damage is admitted; misattributed damage is not.
    struct DamageLatch
    {
        static constexpr std::size_t SLOTS = 4;

        DamageContext slot[SLOTS];
        std::size_t   next = 0;

        void Push(ObjectGuid attacker, ObjectGuid victim, uint32 spellId,
            uint8 kind, uint8 evKind, uint32 raw)
        {
            // Only an eviction of a latch that could still have been consumed is
            // worth counting; a stale one from an earlier tick is already dead.
            if (slot[next].valid && slot[next].ms == s_elapsedMs)
                ++s_latchOverflow;

            slot[next] = { attacker, victim, spellId, s_elapsedMs, raw, kind, evKind, true };
            next = (next + 1) % SLOTS;
        }

        bool Take(ObjectGuid attacker, ObjectGuid victim, uint8 evKind,
            uint32& spellId, uint8& kind, uint32& raw)
        {
            for (std::size_t n = 0; n < SLOTS; ++n)
            {
                DamageContext& c = slot[(next + SLOTS - 1 - n) % SLOTS];

                if (!c.valid || c.ms != s_elapsedMs || c.evKind != evKind ||
                    c.attacker != attacker || c.victim != victim)
                    continue;

                spellId = c.spellId;
                kind    = c.kind;
                raw     = c.raw;
                c.valid = false;
                return true;
            }

            return false;
        }

        // Was anything latched for this victim this tick, whoever the attacker
        // was? Distinguishes "no hook fired" from "the hook named someone else".
        bool SawThisTick(ObjectGuid victim, uint8 evKind) const
        {
            for (DamageContext const& c : slot)
                if (c.valid && c.ms == s_elapsedMs && c.evKind == evKind &&
                    c.victim == victim)
                    return true;

            return false;
        }

        void Clear()
        {
            for (DamageContext& c : slot)
                c.valid = false;

            next = 0;
        }
    };

    DamageLatch s_latch;

    // The DamageEffectType of the event currently being dealt.
    //
    // UnitScript::DealDamage is called from Unit::DealDamage at Unit.cpp:1013,
    // fifteen lines before OnDamage at 1028 and in the same call stack, so this
    // is the one signal that is unambiguously about *this* event. Unlike the
    // Modify* hooks it fires for every damage event without exception, which is
    // exactly what is needed to say what the unattributed damage actually is
    // rather than guessing at it.
    //
    // It is dispatched to every UnitScript unconditionally (UnitScript.cpp:52
    // has no CALL_ENABLED_HOOKS gate), so there is no hook enum to register.
    struct DealContext
    {
        ObjectGuid attacker;
        ObjectGuid victim;
        uint32     ms    = 0;
        uint8      type  = 0;   // DamageEffectType
        bool       valid = false;
    };

    DealContext s_dealType;

    // Unattributed damage, split by what the core said the event was. A single
    // total says "the breakdown is incomplete"; this says which mechanism is
    // missing, which is the difference between a known gap and a mystery.
    uint64 s_unattributedByType[6] = { 0, 0, 0, 0, 0, 0 };

    // -- The combat log ----------------------------------------------------
    //
    // Read the same thing Warcraft Logs reads: the server's own combat-log
    // packets. They carry what no script hook does -- the crit flag, the exact
    // absorb and resist, and the spell id -- for melee, spells, periodic ticks
    // and heals alike, in one uniform place.
    //
    // Why this is reachable at all: WorldSession::SendPacket calls
    // sScriptMgr->OnPlayerbotPacketSent at WorldSession.cpp:300, *before* the
    // `if (!m_Socket) return;` at :302. The ordinary ServerScript::CanPacketSend
    // hook sits after that return, so it never fires for a socketless bot --
    // which is every actor this simulator runs.
    //
    // Ordering is in our favour: SendAttackStateUpdate is sent at Unit.cpp:2847
    // and DealMeleeDamage runs at :2851; SendSpellNonMeleeDamageLog is sent at
    // Spell.cpp:2859 and DealSpellDamage at :2866. The log record therefore
    // always arrives *before* the OnDamage that consumes it.
    struct LogRecord
    {
        ObjectGuid attacker;
        ObjectGuid victim;
        uint32     spellId = 0;
        uint32     amount  = 0;
        uint32     absorb  = 0;
        uint32     resist  = 0;
        uint32     ms      = 0;
        uint8      kind    = DK_UNKNOWN;
        uint8      evKind  = EK_DAMAGE;
        bool       crit    = false;
        bool       valid   = false;
    };

    // Eight rather than the latch's four: a multi-school melee swing and a
    // split-damage cascade can both put several records in one tick, and unlike
    // the latch these are cheap to keep.
    struct CombatLogRing
    {
        static constexpr std::size_t SLOTS = 8;

        LogRecord   slot[SLOTS];
        std::size_t next = 0;

        void Push(LogRecord const& r)
        {
            slot[next] = r;
            slot[next].valid = true;
            next = (next + 1) % SLOTS;
        }

        // Newest-first, same-tick only, and preferring an exact amount match
        // when several records share an attacker and victim in one tick.
        LogRecord const* Take(ObjectGuid attacker, ObjectGuid victim,
            uint8 evKind, uint32 amount)
        {
            LogRecord* fallback = nullptr;

            for (std::size_t n = 0; n < SLOTS; ++n)
            {
                LogRecord& r = slot[(next + SLOTS - 1 - n) % SLOTS];

                if (!r.valid || r.ms != s_elapsedMs || r.evKind != evKind ||
                    r.attacker != attacker || r.victim != victim)
                    continue;

                if (r.amount == amount)
                {
                    r.valid = false;
                    return &r;
                }

                if (!fallback)
                    fallback = &r;
            }

            if (fallback)
                fallback->valid = false;

            return fallback;
        }

        void Clear()
        {
            for (LogRecord& r : slot)
                r.valid = false;

            next = 0;
        }
    };

    CombatLogRing s_log;

    // How much of the breakdown the combat log actually backed, so "0 crits"
    // can be told apart from "the log never matched".
    uint32 s_logMatched = 0;
    uint32 s_logUnmatched = 0;

    // Incoming absorb and resist, exact, from the log. The shield ledger built
    // from aura sampling measures the same thing a different way; the two
    // agreeing is a real cross-check, and disagreeing is a real signal.
    uint64 s_absorbedOnActor = 0;
    uint64 s_resistedOnActor = 0;

    struct AbilityStat
    {
        uint64 damage = 0;
        uint32 count  = 0;
        uint8  kind   = DK_UNKNOWN;
        bool   fromPet = false;

        uint32 minHit = 0xFFFFFFFF;
        uint32 maxHit = 0;

        // Real crits, off the combat log -- not inferred from min/max. No script
        // hook carries the flag (every damage hook fires before the roll), but
        // the server's own SMSG_ATTACKERSTATEUPDATE / SPELLNONMELEEDAMAGELOG /
        // PERIODICAURALOG / SPELLHEALLOG all do, and a module can read them.
        uint32 crits     = 0;
        uint32 critDamage = 0;

        // How many of `count` a log record actually backed. crits is only a rate
        // out of this, never out of count, or a decode that stops matching
        // silently reads as "this ability never crits".
        uint32 logged = 0;

        // Healing is the other half of a healer's output, and five of the eight
        // specs built so far are healers. Overheal is the number that says
        // whether the sustain is real or wasted.
        uint64 healing   = 0;
        uint32 healCount = 0;
        uint32 healCrits = 0;
        uint64 overheal  = 0;

        // Latches pushed for this ability. For melee, attempts - count is the
        // miss/dodge/parry count, free, because the hook fires before the
        // outcome roll. For spells the two are equal by construction, since
        // CalculateSpellDamageTaken is only reached on a hit.
        uint32 attempts = 0;

        void AddHit(uint32 amount, bool crit = false)
        {
            damage += amount;
            count  += 1;
            minHit  = std::min(minHit, amount);
            maxHit  = std::max(maxHit, amount);

            if (crit)
            {
                crits      += 1;
                critDamage += amount;
            }
        }
    };

    // -- Per-event series --------------------------------------------------
    //
    // The three views a report needs -- the ability table, damage over time, the
    // damage-taken spikes -- are all projections of one event list. Keeping the
    // projections instead would mean three things that can disagree; keeping the
    // events means Python owns every definition, and a definition can change
    // without a rebuild. That is the same reasoning that already put TMI in
    // tools/sim.py rather than here.
    enum EventFlags : uint16
    {
        EF_FROM_PET     = 0x0001,   // attacker is owned by the actor
        EF_INCOMING     = 0x0002,   // the actor is the victim
        EF_UNATTRIBUTED = 0x0004,   // no latch matched; spellId is 0, not a guess
        EF_SELF         = 0x0008,   // healer == target
        EF_CRIT         = 0x0010,   // from the combat log, not inferred
        EF_LOGGED       = 0x0020    // a combat-log record matched this event, so
                                    // crit/absorb/resist are real rather than 0
    };

    // 28 bytes, trivially copyable, no per-event allocation.
    struct CombatEvent
    {
        uint32 ms;             // s_elapsedMs at the hook; tick-quantised
        uint32 spellId;        // 0 = melee, or unattributed (see EF_UNATTRIBUTED)
        uint32 amount;         // the authoritative applied amount: post-mitigation
                               // for damage, *effective* (post-overheal) for heals
        uint32 absorb;         // exact, from the combat log. Not sampled.
        uint32 resist;         // likewise
        uint32 preMitigation;  // the latched Modify* value.
                               //
                               // NOT gross damage. ModifySpellDamageTaken fires
                               // before armor (Unit.cpp:1552), before the crit
                               // multiplier (1650), before resilience and before
                               // resist -- not merely before absorb. For a DoT
                               // tick it fires before the damage bonus is applied
                               // at all. Absorb is measured at the shield instead.
                               //
                               // For heals it IS meaningful: preMitigation -
                               // amount is overheal (plus heal absorb, which no
                               // solo Alonecraft parse contains).
        uint16 flags;          // EventFlags
        uint8  kind;           // EventKind
        uint8  damageKind;     // DamageKind
    };
    static_assert(sizeof(CombatEvent) == 28, "CombatEvent must stay packed");

    std::vector<CombatEvent> s_events;
    uint32 s_eventsDropped = 0;

    // Death timestamps, which the once-per-tick alive poll cannot give: it
    // reports one enum for the whole iteration and misses a pet entirely. A BM
    // hunter whose pet dies at 90s has a damage cliff that nothing else explains.
    enum DeathRole : uint8
    {
        DR_ACTOR  = 0,
        DR_TARGET = 1,
        DR_PET    = 2
    };

    std::vector<std::pair<uint32, uint8>> s_deaths;   // (ms, DeathRole)

    // The run-level ability table keeps its exact existing meaning, because
    // tools/sim.py prints it and a matrix compares against it. The per-iteration
    // one is new: without it there is no way to see a spec whose rotation fell
    // apart in only one fight, because the totals average it away.
    std::map<std::pair<uint32, bool>, AbilityStat> s_iterAbilities;

    uint64 s_healing         = 0;
    uint64 s_overheal        = 0;
    uint32 s_unattributedHeals = 0;

    // Why a latch failed to match, which is the difference between "no hook
    // fired" and "the hook fired for a different unit". Unit::CalculateSpellDamageTaken
    // is called on the *owner* rather than the caster for some pet spells
    // (Spell.cpp:2810-2816), so an attacker mismatch is a real, findable cause
    // and not the same problem as an event with no hook at all.
    uint32 s_latchMissNoLatch        = 0;   // nothing pushed this tick at all
    uint32 s_latchMissAttacker       = 0;   // a latch existed, different attacker
    uint32 s_latchMissAlreadyTaken   = 0;   // pushed this tick but already consumed

    // -- Aura uptime -------------------------------------------------------
    //
    // Sampled rather than hooked, and that is deliberate. There is no hook for a
    // stack *decrement* -- Aura::ModStackAmount fires OnAuraApply only on the way
    // up (Unit.cpp:4699) -- and stacks are exactly what the redesigned proc
    // buttons are about. Sampling also needs no separate reconciliation when the
    // actor dies or the clock runs out, because it never assumed an interval was
    // still open.
    //
    // The error is bounded, uniform and reported: aura_sample_ms is in the
    // result, so Python knows the quantum rather than guessing at it.
    struct AuraUptime
    {
        uint32 samplesPresent = 0;
        uint64 stackSum       = 0;   // -> mean stacks while present
        uint8  maxStacks      = 0;
        uint32 firstMs        = 0;
        uint32 lastMs         = 0;

        // Rising edges. More useful than uptime alone for anything with a
        // cooldown: 100% uptime on a two-minute buff means it was pressed three
        // times, while 40% uptime over sixty applications is a rotation
        // spamming it.
        uint32 applications   = 0;

        uint32 lastSample     = 0;   // presence and edge detection
        int32  lastAmount     = 0;   // absorb ledger: shield remaining
        uint8  lastStacks     = 0;

        // The filter verdict, cached on first sighting. Without this the poll
        // does a GetSpellInfo lookup for every aura on every sample -- 315k per
        // iteration rather than ~105.
        bool   known          = false;
        bool   tracked        = false;
        bool   series         = false;
        bool   absorb         = false;
    };

    std::map<uint32, AuraUptime> s_actorAuras;
    std::map<uint32, AuraUptime> s_targetAuras;
    uint32 s_auraSamples   = 0;
    uint32 s_aurasFiltered = 0;

    // Change-only, so a stack series over 300 seconds is ~250 transitions rather
    // than 3000 samples. Python reconstructs the step function with a scan.
    std::map<uint32, std::vector<std::pair<uint32, uint8>>> s_auraStacks;
    uint32 s_auraStacksDropped = 0;
    uint32 constexpr AURA_STACK_MAX = 24;

    // -- Absorb ------------------------------------------------------------
    //
    // Measured at the shield, not at the damage event, because the damage event
    // cannot tell you. The pre-mitigation value latched by ModifySpellDamageTaken
    // is not gross damage -- it precedes armor, the crit multiplier, resilience
    // and resist as well as absorb -- so a gross-minus-net difference would be a
    // lump of five things wearing an absorb label.
    //
    // An absorb AuraEffect's amount IS the remaining shield: CalcAbsorbResist
    // decrements it as it eats damage. Summing the decrements the poll observes
    // gives absorbed per shield spell, which is the number that says whether
    // Ice Barrier or Mana Shield is actually carrying a spec.
    struct ShieldStat
    {
        uint64 absorbed     = 0;
        uint32 applications = 0;
        uint32 expiredFull  = 0;   // dropped with shield left: overcast
        uint32 consumed     = 0;   // dropped at zero: right-sized or too small
    };

    std::map<uint32, ShieldStat> s_shields;

    // -- Resources ---------------------------------------------------------
    struct ResourceSample
    {
        uint32 ms;
        uint32 actorHp;
        uint32 actorPower;
        uint32 targetHp;
        uint8  powerType;
        uint8  pad[3];
    };

    std::vector<ResourceSample> s_resources;

    // Read once at startup and cached, so no hook ever touches sConfigMgr.
    uint32 s_auraSampleMs = 100;
    uint32 s_resSampleMs  = 1000;
    uint32 s_auraSampleDue = 0;
    uint32 s_resSampleDue  = 0;

    // 200k events is 4 MB, roughly 44x the volume of a 300-second fight. It is a
    // runaway guard -- an AoE spec against forty adds, or a fight that never
    // ends -- not a routine limit. A cap that trips in normal use is a silent
    // data-quality problem; this one should never trip, and says so if it does.
    uint32 constexpr EVENT_CAP = 200000;

    void PushEvent(CombatEvent const& ev)
    {
        if (s_events.size() >= EVENT_CAP)
        {
            ++s_eventsDropped;
            return;
        }

        s_events.push_back(ev);
    }

    // A permanent passive has 100% uptime by construction and says nothing about
    // whether a button was pressed. Dropping it from the uptime table is not
    // hiding it: the end-of-fight snapshot still lists every aura, so "did the
    // talent apply at all" stays answerable.
    //
    // IsPermanent() is the load-bearing test and it cuts the right way on both
    // sides. Molten, Frost and Mage Armor carry a 30-minute duration rather than
    // -1, so they stay in the table at 100% -- which is exactly the "is it being
    // maintained" question. Talent passives and item-set procs are -1 and go.
    //
    // Deliberately not filtering on SPELL_ATTR1_NO_AURA_ICON: that is a client
    // aura-bar hint, and several genuinely interesting internal proc trackers
    // carry it.
    bool TracksForUptime(Aura const* aura)
    {
        if (!aura || aura->IsPermanent())
            return false;

        SpellInfo const* info = aura->GetSpellInfo();

        return info && !info->IsPassive() &&
               !info->HasAttribute(SPELL_ATTR0_DO_NOT_DISPLAY);
    }

    // Remaining shield on an absorb aura, or -1 when it is not one.
    int32 ShieldRemaining(Aura const* aura)
    {
        for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
            if (AuraEffect const* eff = aura->GetEffect(i))
                if (eff->GetAuraType() == SPELL_AURA_SCHOOL_ABSORB ||
                    eff->GetAuraType() == SPELL_AURA_MANA_SHIELD)
                    return eff->GetAmount();

        return -1;
    }

    void AccumulateAura(std::map<uint32, AuraUptime>& acc, uint32 spellId,
        Aura const* aura, uint8 stacks, bool wantAbsorb)
    {
        AuraUptime& u = acc[spellId];

        if (!u.known)
        {
            u.known   = true;
            u.tracked = TracksForUptime(aura);

            if (!u.tracked)
            {
                ++s_aurasFiltered;
                return;
            }

            // A stack series is only worth keeping for something that stacks or
            // procs. Reuses the same SpellProcEntry predicate the end-of-fight
            // snapshot already reports.
            SpellInfo const* info = aura->GetSpellInfo();
            u.series = (info && info->StackAmount > 1) ||
                       sSpellMgr->GetSpellProcEntry(spellId) != nullptr;

            if (u.series && s_auraStacks.size() >= AURA_STACK_MAX)
            {
                u.series = false;
                ++s_auraStacksDropped;
            }

            if (wantAbsorb)
            {
                int32 const remaining = ShieldRemaining(aura);
                u.absorb = remaining >= 0;
            }
        }

        if (!u.tracked)
            return;

        bool const wasPresent = u.lastSample + 1 == s_auraSamples && u.samplesPresent;

        if (!wasPresent)
        {
            ++u.applications;

            if (!u.samplesPresent)
                u.firstMs = s_elapsedMs;

            if (u.absorb)
            {
                ++s_shields[spellId].applications;
                u.lastAmount = ShieldRemaining(aura);
            }
        }
        else if (u.absorb)
        {
            // The shield's own amount is what CalcAbsorbResist decrements, so a
            // fall between samples is exactly what it ate.
            int32 const remaining = ShieldRemaining(aura);

            if (remaining >= 0 && remaining < u.lastAmount)
                s_shields[spellId].absorbed += uint32(u.lastAmount - remaining);

            u.lastAmount = remaining;
        }

        ++u.samplesPresent;
        u.stackSum  += stacks;
        u.maxStacks  = std::max(u.maxStacks, stacks);
        u.lastMs     = s_elapsedMs;
        u.lastSample = s_auraSamples;

        if (u.series && stacks != u.lastStacks)
        {
            s_auraStacks[spellId].emplace_back(s_elapsedMs, stacks);
            u.lastStacks = stacks;
        }
    }

    // Auras that were present last sample and are not now: close their stack
    // series at zero, and settle the shield ledger. Only walks tracked entries,
    // which is ~10-20 rather than ~105.
    void CloseVanishedAuras(std::map<uint32, AuraUptime>& acc)
    {
        for (auto& [spellId, u] : acc)
        {
            if (!u.tracked || u.lastSample != s_auraSamples - 1 || !u.samplesPresent)
                continue;

            if (u.series && u.lastStacks)
            {
                s_auraStacks[spellId].emplace_back(s_elapsedMs, 0);
                u.lastStacks = 0;
            }

            if (u.absorb)
            {
                ShieldStat& sh = s_shields[spellId];

                // Gone with shield left means it was overcast or dispelled;
                // gone at zero means it was consumed. The distinction is the
                // difference between "this shield is the wrong size" and "this
                // shield is doing its job", which is a tuning decision.
                if (u.lastAmount > 0)
                {
                    // It cannot be known whether the last sample's remainder was
                    // eaten in the gap or expired unused, so it is not counted as
                    // absorbed. Under-reporting is the safe direction.
                    ++sh.expiredFull;
                }
                else
                    ++sh.consumed;

                u.lastAmount = 0;
            }
        }
    }

    void SampleAuras(Player* actor, Unit* target)
    {
        ++s_auraSamples;

        for (auto const& [spellId, aurApp] : actor->GetAppliedAuras())
            if (aurApp && aurApp->GetBase())
                AccumulateAura(s_actorAuras, spellId, aurApp->GetBase(),
                               aurApp->GetBase()->GetStackAmount(), true);

        CloseVanishedAuras(s_actorAuras);

        if (target)
        {
            for (auto const& [spellId, aura] : target->GetOwnedAuras())
                if (aura)
                    AccumulateAura(s_targetAuras, spellId, aura,
                                   aura->GetStackAmount(), false);

            CloseVanishedAuras(s_targetAuras);
        }
    }

    void SampleResources(Player* actor, Unit* target)
    {
        // Absolutes, not percentages. The maxima are already in the header, and
        // an absolute reveals a mid-fight max-health change that a percentage
        // silently hides.
        Powers const power = actor->getPowerType();

        s_resources.push_back({
            s_elapsedMs,
            uint32(actor->GetHealth()),
            uint32(actor->GetPower(power)),
            uint32(target ? target->GetHealth() : 0),
            uint8(power), { 0, 0, 0 } });
    }

    // -- Incoming mitigation and avoidance ---------------------------------
    //
    // Damage taken answers "how much got through". It cannot answer "what
    // stopped the rest", and for a project tuning survivability that is the
    // more useful half: two specs that both end a fight on 60% health, one by
    // dodging a third of the swings and one by absorbing them, are not the same
    // spec and should not be tuned the same way.
    //
    // Every field here is counted from SMSG_ATTACKERSTATEUPDATE, so it is what
    // the server told the client happened, not a reconstruction. The one
    // derived number, avoided damage, is computed in Python from the mean
    // landing swing and labelled an estimate, because the damage an avoided
    // swing *would* have done is not a fact the server ever computes.
    struct MitigationStat
    {
        uint32 swings   = 0;   // melee attacks resolved against the actor
        uint32 misses   = 0;
        uint32 dodges   = 0;
        uint32 parries  = 0;
        uint32 deflects = 0;
        uint32 immune   = 0;
        uint32 blocks   = 0;   // partial blocks; the swing still landed
        uint32 crits    = 0;
        uint32 glancing = 0;
        uint32 crushing = 0;

        uint64 blocked  = 0;   // damage removed by block, exact
        uint64 absorbed = 0;   // by shields; s_shields has the per-spell split
        uint64 resisted = 0;
        uint64 landed   = 0;   // reached the health bar
    };

    MitigationStat s_mitigation;

    void RecordIncomingSwing(uint32 hitInfo, uint8 targetState, uint32 fullDamage,
                             uint32 absorb, uint32 resist, uint32 blocked)
    {
        MitigationStat& m = s_mitigation;

        ++m.swings;

        if (hitInfo & HITINFO_MISS)      ++m.misses;
        if (hitInfo & HITINFO_BLOCK)     ++m.blocks;
        if (hitInfo & HITINFO_CRITICALHIT) ++m.crits;
        if (hitInfo & HITINFO_GLANCING)  ++m.glancing;
        if (hitInfo & HITINFO_CRUSHING)  ++m.crushing;

        switch (targetState)
        {
            case VICTIMSTATE_DODGE:     ++m.dodges;   break;
            case VICTIMSTATE_PARRY:     ++m.parries;  break;
            case VICTIMSTATE_DEFLECTS:  ++m.deflects; break;
            case VICTIMSTATE_IS_IMMUNE: ++m.immune;   break;
            default: break;
        }

        // fullDamage is the post-mitigation total the client is shown, so it is
        // already net of block, absorb and resist -- adding them back is how the
        // gross swing is recovered, and subtracting nothing is how "landed" is.
        m.blocked  += blocked;
        m.absorbed += absorb;
        m.resisted += resist;
        m.landed   += fullDamage;
    }

    // What the actor's defences were set to, as distinct from what they did.
    //
    // The pairing is the point: 34% dodge chance next to 33% of swings dodged
    // says the roll is working, and 34% next to 4% says something is wrong with
    // how the swings are arriving. Neither number means much alone.
    struct DefenseSnapshot
    {
        uint32 armor        = 0;
        float  dodgePct     = 0.0f;
        float  parryPct     = 0.0f;
        float  blockPct     = 0.0f;
        uint32 blockValue   = 0;
        uint32 defenseSkill = 0;
        uint32 resilience   = 0;

        // Applied auras that reduce damage taken, with the amount each carries.
        // This is the "which talent" half of the question, and it is a statement
        // of what was up rather than a measurement of what it prevented -- the
        // core applies these as a multiplier inside the damage calculation and
        // never reports the difference.
        std::vector<std::pair<uint32, int32>> reductionAuras;  // spell, amount
    };

    DefenseSnapshot s_defenseAtStart;

    DefenseSnapshot SnapshotDefense(Player* actor)
    {
        DefenseSnapshot d;

        if (!actor)
            return d;

        d.armor        = uint32(actor->GetArmor());
        d.dodgePct     = actor->GetUnitDodgeChance();
        d.parryPct     = actor->GetUnitParryChance();
        d.blockPct     = actor->GetUnitBlockChance();
        d.blockValue   = uint32(actor->GetShieldBlockValue());
        d.defenseSkill = actor->GetDefenseSkillValue();
        d.resilience   = uint32(actor->GetMeleeCritDamageReduction(100));

        // The aura types that actually reduce incoming damage. Anything outside
        // this list is left out rather than guessed at: a list that quietly
        // includes an aura type whose MiscValue means something else produces a
        // confident wrong attribution, which is worse than an absent row.
        static AuraType const REDUCTION_TYPES[] = {
            SPELL_AURA_MOD_DAMAGE_PERCENT_TAKEN,
            SPELL_AURA_MOD_ATTACKER_MELEE_CRIT_DAMAGE,
            SPELL_AURA_MOD_ATTACKER_RANGED_CRIT_DAMAGE,
            SPELL_AURA_MOD_SCHOOL_CRIT_DMG_TAKEN,
            SPELL_AURA_MOD_MECHANIC_DAMAGE_TAKEN_PERCENT
        };

        for (AuraType type : REDUCTION_TYPES)
            for (AuraEffect const* eff : actor->GetAuraEffectsByType(type))
                if (eff && eff->GetAmount())
                    d.reductionAuras.emplace_back(eff->GetId(), eff->GetAmount());

        return d;
    }

    // The pet as it stood at the first millisecond of a fight.
    //
    // A pet is a second actor with its own stats, and until this existed
    // nothing in the result could tell "this spec is weak" from "the pet lost
    // its scaling" -- both read as one low damage number. Entry and level catch
    // a pet that was re-summoned or re-levelled between iterations; attackPower
    // and the passive count catch one that kept its identity and lost its
    // passives, which is the failure that actually happened.
    struct PetSnapshot
    {
        uint32 entry       = 0;
        uint32 level       = 0;
        uint32 attackPower = 0;
        uint32 maxHealth   = 0;
        uint32 passives    = 0;   // applied auras flagged passive
        uint32 auras       = 0;   // applied auras, total
    };

    PetSnapshot s_petAtStart;

    PetSnapshot SnapshotPet(Player* actor)
    {
        PetSnapshot s;

        // GetGuardianPet, not GetPet. Player::GetPet (Player.cpp:9001) requires
        // the pet guid to be of pet *type* and returns null otherwise, so a
        // permanent pet that is attached as a plain guardian is invisible to it
        // -- which is not hypothetical: hunter_surv fought iterations 2 and 3
        // with a pet contributing ~20% of the damage while GetPet() reported
        // none. GetGuardianPet resolves through GetCreatureOrPetOrVehicle and
        // finds both, which is why the playerbot AI uses it throughout.
        Guardian* pet = actor ? actor->GetGuardianPet() : nullptr;
        if (!pet)
            return s;

        s.entry       = pet->GetEntry();
        s.level       = pet->GetLevel();
        s.attackPower = uint32(pet->GetTotalAttackPowerValue(BASE_ATTACK));
        s.maxHealth   = uint32(pet->GetMaxHealth());

        for (auto const& [spellId, aurApp] : pet->GetAppliedAuras())
        {
            if (!aurApp || !aurApp->GetBase())
                continue;

            ++s.auras;
            if (aurApp->GetBase()->IsPassive())
                ++s.passives;
        }

        return s;
    }

    struct IterationResult
    {
        uint32 index;
        uint32 seed;
        uint64 damage;
        uint32 durationMs;
        uint64 actorEvents;

        uint8  outcome        = OUTCOME_TIMEOUT;
        uint64 damageTaken    = 0;
        uint32 actorHpPct     = 0;   // at end of fight
        uint32 targetHpPct    = 0;

        // (millisecond, amount) for every hit the actor took, which is what TMI
        // needs: it is a soft maximum over trailing 6-second windows, so it
        // cannot be recovered from a total. Python owns the arithmetic.
        std::vector<std::pair<uint32, uint32>> incoming;

        // Auras on the target when the fight ended, cast by the actor.
        //
        // The damage table says which abilities landed. It cannot say whether a
        // debuff the rotation depends on was ever applied, and that is a
        // different failure with the same symptom. The fire mage cast Scorch
        // 157 times and Fireball never, because its bot AI re-Scorches until the
        // target carries Improved Scorch (22959) -- so the question "did 22959
        // land?" decides whether the fault is in the rotation or in the spell,
        // and nothing else in the result answers it.
        std::vector<std::pair<uint32, uint8>> targetAuras;  // spell id, stacks

        // And on the actor, for the same reason from the other side: a passive
        // talent that never applied, a seal that dropped, a self-buff the bot
        // forgot, all look like a tuning problem in the damage table alone.
        std::vector<std::pair<uint32, uint8>> actorAuras;

        // Every damage and healing event of this fight, in order. The ability
        // table, the damage-over-time curve and the incoming spikes are all
        // projections of this one list.
        std::vector<CombatEvent> events;
        uint32 eventsDropped = 0;

        std::vector<std::pair<uint32, uint8>> deaths;   // (ms, DeathRole)

        // This fight's own ability table, which the run-level one averages away.
        std::map<std::pair<uint32, bool>, AbilityStat> abilities;

        uint64 healing  = 0;
        uint64 overheal = 0;

        // Incoming absorb/resist for this fight, exact, from the combat log.
        // damageTaken above is what reached the health bar; these are what the
        // enemy actually threw and something ate.
        uint64 absorbedOnActor = 0;
        uint64 resistedOnActor = 0;

        std::map<uint32, AuraUptime> actorUptime;
        std::map<uint32, AuraUptime> targetUptime;
        std::map<uint32, std::vector<std::pair<uint32, uint8>>> auraStacks;
        std::map<uint32, ShieldStat> shields;
        std::vector<ResourceSample>  resources;

        uint32 auraSamples       = 0;
        uint32 aurasFiltered     = 0;
        uint32 auraStacksDropped = 0;

        PetSnapshot     pet;
        MitigationStat  mitigation;
        DefenseSnapshot defense;
    };

    std::vector<IterationResult> s_results;
    uint32     s_iteration   = 0;

    // Keyed by spell id; melee and unattributed both land on 0, told apart by
    // kind. Pet damage is keyed separately so a BM hunter's breakdown shows the
    // split rather than merging it into the player's line.
    std::map<std::pair<uint32, bool>, AbilityStat> s_abilities;
    uint64 s_unattributedDamage = 0;

    // Damage the actor took this iteration, timestamped against the virtual
    // clock. Feeds TTD and TMI.
    std::vector<std::pair<uint32, uint32>> s_incoming;
    uint64 s_damageTaken = 0;
    uint8  s_outcome     = OUTCOME_TIMEOUT;

    SimState   s_state       = SIM_IDLE;
    ObjectGuid s_actorGuid;
    ObjectGuid s_targetGuid;
    uint64     s_damage      = 0;
    // Every damage event seen anywhere, before filtering to actor->target. Tells
    // "the bot did nothing" apart from "the filter threw everything away".
    uint64     s_anyDamageEvents = 0;
    uint64     s_actorDamageEvents = 0;

    // Background combat is not a curiosity: everything on the world thread draws
    // from the same thread-local RNG stream, so any other fight interleaves with
    // ours and desyncs a seeded run. Attribute it before trying to remove it.
    std::map<uint32, uint32> s_foreignDamageByEntry;   // creature entry -> events
    std::map<uint32, uint32> s_foreignDamageByMap;     // map id -> events
    // Recorded after configuration, so the result reports what was actually
    // simmed rather than what was asked for.
    uint32     s_actorLevel  = 0;
    uint32     s_actorMaxHealth = 0;
    // Spells the actor knows but cannot cast, after repair. Non-zero means the
    // run measured a crippled actor and the number is a floor, not a result.
    uint32     s_uncastableSpells = 0;
    uint32     s_talentSpellsLearned = 0;
    // Fixed-gear outcome. A run geared 15/17 is not comparable with one geared
    // 17/17, so both numbers go in the result rather than only the log.
    uint32     s_gearEquipped = 0;
    uint32     s_gearFailed   = 0;
    uint32     s_gearIlvl     = 0;   // mean ItemLevel over filled equipment slots
    uint32     s_talentTabs[3] = { 0, 0, 0 };
    uint32     s_actorAttackPower = 0;
    uint32     s_actorSpellPower  = 0;
    float      s_actorCrit        = 0.0f;
    uint32     s_waitedMs    = 0;
    uint32     s_ticks       = 0;

    // Real OS clock, not GameTime: used only to report how much faster than
    // realtime the simulation ran.
    std::chrono::steady_clock::time_point s_wallStart;

    std::string Arg(std::vector<std::string> const& args, std::string const& key)
    {
        std::string const prefix = key + "=";

        for (std::size_t i = 0; i < args.size(); ++i)
        {
            // Both "--key value" and "--key=value" are accepted.
            if (args[i] == key && i + 1 < args.size())
                return args[i + 1];

            if (args[i].compare(0, prefix.size(), prefix) == 0)
                return args[i].substr(prefix.size());
        }

        return "";
    }

    bool HasArg(std::vector<std::string> const& args, std::string const& key)
    {
        std::string const prefix = key + "=";

        for (auto const& a : args)
            if (a == key || a.compare(0, prefix.size(), prefix) == 0)
                return true;

        return false;
    }

    void ParseArgs()
    {
        auto const& args = sConfigMgr->GetArguments();

        if (!HasArg(args, "--sim"))
            return;

        s_cfg.active    = true;
        s_cfg.character = Arg(args, "--sim-char");

        if (std::string v = Arg(args, "--sim-seconds"); !v.empty())
            s_cfg.seconds = uint32(std::stoul(v));
        if (std::string v = Arg(args, "--sim-seed"); !v.empty())
            s_cfg.seed = uint32(std::stoul(v));
        if (std::string v = Arg(args, "--sim-target"); !v.empty())
            s_cfg.targetEntry = uint32(std::stoul(v));
        if (std::string v = Arg(args, "--sim-out"); !v.empty())
            s_cfg.outPath = v;
        if (std::string v = Arg(args, "--sim-spec"); !v.empty())
            s_cfg.spec = v;
        if (std::string v = Arg(args, "--sim-range"); !v.empty())
            s_cfg.range = std::stof(v);
        if (std::string v = Arg(args, "--sim-gear"); !v.empty())
            s_cfg.gear = v;
        if (std::string v = Arg(args, "--sim-level"); !v.empty())
            s_cfg.level = uint32(std::stoul(v));
        if (std::string v = Arg(args, "--sim-iterations"); !v.empty())
            s_cfg.iterations = std::max(1u, uint32(std::stoul(v)));
        if (std::string v = Arg(args, "--sim-buff-seconds"); !v.empty())
            s_cfg.buffSeconds = uint32(std::stoul(v));

        // Where the fight happens, which decides whether mod-autobalance is even
        // consulted: every scaling path in ABAllCreatureScript is gated on
        // map->IsDungeon(), and the default arena is GM Island on map 1, a
        // continent. So a run that wants to measure what a solo player actually
        // meets -- a boss scaled to a party of one -- has to move to an instance.
        //
        // All four are set together or not at all. A map without its coordinates
        // is a teleport into terrain that does not exist, and the actor simply
        // never arrives.
        if (std::string v = Arg(args, "--sim-map"); !v.empty())
            s_cfg.mapId = uint32(std::stoul(v));
        if (std::string v = Arg(args, "--sim-x"); !v.empty())
            s_cfg.x = std::stof(v);
        if (std::string v = Arg(args, "--sim-y"); !v.empty())
            s_cfg.y = std::stof(v);
        if (std::string v = Arg(args, "--sim-z"); !v.empty())
            s_cfg.z = std::stof(v);
    }

    std::string Lower(std::string s)
    {
        std::transform(s.begin(), s.end(), s.begin(),
            [](unsigned char c) { return char(std::tolower(c)); });
        return s;
    }

    void Fail(std::string const& why)
    {
        LOG_ERROR("server.worldserver", "Simulator: {}", why);
        s_state = SIM_DONE;
        World::StopNow(ERROR_EXIT_CODE);
    }

    // -- Result ------------------------------------------------------------
    //
    // Hand-rolled JSON: the core bundles no JSON library, and a result this
    // small does not justify adding one.  tools/sim.py consumes it.

    // Every string in the result goes through this, including the quotes.
    //
    // Three of the five strings emitted here used to be written raw, so a spec
    // name or a failure note containing a quote produced a file tools/sim.py
    // could not parse -- and that failure looks like a crashed simulator rather
    // than a quoting bug, which is the expensive kind of wrong. Control
    // characters are escaped too: a DBC spell name can legally contain them.
    std::string JsonStr(std::string_view s)
    {
        std::string out;
        out.reserve(s.size() + 8);
        out += '"';

        for (unsigned char c : s)
        {
            switch (c)
            {
                case '"':  out += "\\\""; break;
                case '\\': out += "\\\\"; break;
                case '\n': out += "\\n";  break;
                case '\r': out += "\\r";  break;
                case '\t': out += "\\t";  break;
                default:
                    if (c < 0x20)
                        out += Acore::StringFormat("\\u{:04x}", uint32(c));
                    else
                        out += char(c);
                    break;
            }
        }

        out += '"';
        return out;
    }

    // SpellName[0] is a char const* and can be null; constructing a std::string
    // from it takes the process down at the exact moment it writes its results.
    std::string SpellName(uint32 spellId, char const* zeroName = "melee")
    {
        if (!spellId)
            return zeroName;

        SpellInfo const* info = sSpellMgr->GetSpellInfo(spellId);
        if (info && info->SpellName[0])
            return info->SpellName[0];

        return "?";
    }

    // One comma-and-bracket implementation for every series. That is the whole
    // abstraction budget -- the emitter below reads as linear prose and a
    // JsonWriter class would cost more clarity than it saves.
    template <typename Container, typename Fn>
    void WriteArray(std::ostream& out, Container const& c, Fn&& emit)
    {
        out << '[';
        bool first = true;

        for (auto const& e : c)
        {
            if (!first)
                out << ',';

            first = false;
            emit(out, e);
        }

        out << ']';
    }

    void WriteResult(bool ok, std::string const& note)
    {
        uint64 totalDamage = 0;
        uint64 totalActorEvents = 0;
        uint32 totalMs = 0;

        for (auto const& r : s_results)
        {
            totalDamage      += r.damage;
            totalActorEvents += r.actorEvents;
            totalMs          += r.durationMs;
        }

        float const seconds = float(totalMs) / 1000.0f;
        float const dps     = seconds > 0.0f ? float(totalDamage) / seconds : 0.0f;

        // The genuine OS clock, deliberately: this is the one measurement in the
        // simulator that must not be virtual. Virtual seconds simulated per
        // wall-clock second is what decides every iteration budget from here on.
        float const wallSeconds = float(std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - s_wallStart).count()) / 1000.0f;

        float const realtimeFactor = wallSeconds > 0.0f ? seconds / wallSeconds : 0.0f;

        std::ofstream out(s_cfg.outPath, std::ios::out | std::ios::trunc);
        if (!out.is_open())
        {
            LOG_ERROR("server.worldserver", "Simulator: cannot write {}", s_cfg.outPath);
            return;
        }

        out << "{\n"
            // /1 adds per-event series, healing, per-iteration ability tables and
            // death timestamps. Every /0 key keeps its exact name and meaning --
            // damage_taken in particular is still post-absorb net, because that
            // is what TMI needs -- so an older reader degrades rather than breaks.
            << "  \"schema\": \"alonecraft.sim.result/1\",\n"
            << "  \"ok\": " << (ok ? "true" : "false") << ",\n"
            << "  \"note\": " << JsonStr(note) << ",\n"
            << "  \"character\": " << JsonStr(s_cfg.character) << ",\n"
            << "  \"spec\": " << JsonStr(s_cfg.spec) << ",\n"
            << "  \"level\": " << s_actorLevel << ",\n"
            << "  \"uncastable_spells\": " << s_uncastableSpells << ",\n"
            // Gear parity, so a matrix can be checked rather than trusted.
            << "  \"gear_equipped\": " << s_gearEquipped << ",\n"
            << "  \"gear_failed\": " << s_gearFailed << ",\n"
            << "  \"gear_ilvl\": " << s_gearIlvl << ",\n"
            << "  \"attack_power\": " << s_actorAttackPower << ",\n"
            << "  \"spell_power\": " << s_actorSpellPower << ",\n"
            << "  \"crit_pct\": " << s_actorCrit << ",\n"
            << "  \"talent_tabs\": [" << s_talentTabs[0] << "," << s_talentTabs[1]
            << "," << s_talentTabs[2] << "],\n"
            // TMI's denominator: damage taken in a window is only meaningful as
            // a fraction of the health bar it is eating.
            << "  \"actor_max_health\": " << s_actorMaxHealth << ",\n"
            << "  \"seed\": " << s_cfg.seed << ",\n"
            << "  \"target_entry\": " << s_cfg.targetEntry << ",\n"
            << "  \"tick_ms\": " << sConfigMgr->GetOption<int32>("Alonecraft.Sim.TickMs", 25) << ",\n"
            << "  \"ticks\": " << s_ticks << ",\n"
            << "  \"duration_s\": " << seconds << ",\n"
            << "  \"wall_s\": " << wallSeconds << ",\n"
            << "  \"realtime_factor\": " << realtimeFactor << ",\n"
            << "  \"damage\": " << totalDamage << ",\n"
            << "  \"dps\": " << dps << ",\n"
            << "  \"damage_events_seen\": " << s_anyDamageEvents << ",\n"
            << "  \"damage_events_from_actor\": " << totalActorEvents << ",\n";

        // Per iteration, not just the aggregate: a spec with one dead iteration
        // and three good ones has a mean that looks merely disappointing, and a
        // list that says exactly what happened.
        out << "  \"iterations\": [\n";
        for (std::size_t i = 0; i < s_results.size(); ++i)
        {
            IterationResult const& r = s_results[i];
            float const s = float(r.durationMs) / 1000.0f;

            static char const* const outcomeKeys[] = { "timeout", "actor_died", "target_died" };

            out << "    {\"i\": " << r.index
                << ", \"seed\": " << r.seed
                << ", \"duration_s\": " << s
                << ", \"damage\": " << r.damage
                << ", \"dps\": " << (s > 0.0f ? float(r.damage) / s : 0.0f)
                << ", \"damage_events_from_actor\": " << r.actorEvents
                << ", \"outcome\": \"" << outcomeKeys[r.outcome < 3 ? r.outcome : 0] << "\""
                << ", \"damage_taken\": " << r.damageTaken
                << ", \"actor_hp_pct\": " << r.actorHpPct
                << ", \"target_hp_pct\": " << r.targetHpPct
                << ", \"target_auras\": ";

            auto writeAuras = [&out](std::vector<std::pair<uint32, uint8>> const& auras)
            {
                WriteArray(out, auras, [](std::ostream& o, std::pair<uint32, uint8> const& a)
                {
                    // "proc": whether a SpellProcEntry exists for this exact
                    // spell id. Aura::GetProcEffectMask bails immediately when
                    // there is none, and the lookup is by id with no rank
                    // fallback -- so a rank-3 talent whose spell_proc row was
                    // written as "-<rank1>" has one only if spell_ranks chains
                    // the ranks, or the auto-generation pass caught it.
                    o << "{\"spell\": " << a.first
                      << ", \"stacks\": " << uint32(a.second)
                      << ", \"proc\": "
                      << (sSpellMgr->GetSpellProcEntry(a.first) ? "true" : "false")
                      << ", \"name\": " << JsonStr(SpellName(a.first, "?")) << "}";
                });
            };

            writeAuras(r.targetAuras);
            out << ", \"actor_auras\": ";
            writeAuras(r.actorAuras);

            // The raw (ms, amount) series. TMI is a soft maximum over trailing
            // 6-second windows, so it cannot be recomputed from a total, and the
            // arithmetic belongs in Python where the definition can change
            // without a rebuild.
            out << ", \"incoming\": ";
            WriteArray(out, r.incoming, [](std::ostream& o, std::pair<uint32, uint32> const& e)
            {
                o << '[' << e.first << ',' << e.second << ']';
            });

            out << ", \"healing\": " << r.healing
                << ", \"overheal\": " << r.overheal
                << ", \"absorbed_on_actor\": " << r.absorbedOnActor
                << ", \"resisted_on_actor\": " << r.resistedOnActor
                << ", \"events_dropped\": " << r.eventsDropped;

            // Always written, zeroed when the actor has no pet, so a consumer
            // can compare across iterations without special-casing absence.
            out << ", \"pet\": {\"entry\": " << r.pet.entry
                << ", \"level\": " << r.pet.level
                << ", \"attack_power\": " << r.pet.attackPower
                << ", \"max_health\": " << r.pet.maxHealth
                << ", \"auras\": " << r.pet.auras
                << ", \"passives\": " << r.pet.passives << "}";

            // Counted off SMSG_ATTACKERSTATEUPDATE, so "swings" is every melee
            // attack resolved against the actor including the ones that dealt
            // nothing -- which is the point, since those are exactly what
            // damage_taken cannot see.
            MitigationStat const& m = r.mitigation;
            out << ", \"mitigation\": {\"swings\": " << m.swings
                << ", \"misses\": "   << m.misses
                << ", \"dodges\": "   << m.dodges
                << ", \"parries\": "  << m.parries
                << ", \"deflects\": " << m.deflects
                << ", \"immune\": "   << m.immune
                << ", \"blocks\": "   << m.blocks
                << ", \"crits\": "    << m.crits
                << ", \"glancing\": " << m.glancing
                << ", \"crushing\": " << m.crushing
                << ", \"blocked\": "  << m.blocked
                << ", \"absorbed\": " << m.absorbed
                << ", \"resisted\": " << m.resisted
                << ", \"landed\": "   << m.landed << "}";

            out << ", \"defense\": {\"armor\": " << r.defense.armor
                << ", \"dodge_pct\": "    << r.defense.dodgePct
                << ", \"parry_pct\": "    << r.defense.parryPct
                << ", \"block_pct\": "    << r.defense.blockPct
                << ", \"block_value\": "  << r.defense.blockValue
                << ", \"defense_skill\": " << r.defense.defenseSkill
                << ", \"resilience\": "   << r.defense.resilience
                << ", \"reduction_auras\": ";
            WriteArray(out, r.defense.reductionAuras,
                [](std::ostream& o, std::pair<uint32, int32> const& a)
                {
                    o << "{\"spell\": " << a.first
                      << ", \"amount\": " << a.second
                      << ", \"name\": " << JsonStr(SpellName(a.first, "?")) << "}";
                });
            out << "}";

            // (ms, DeathRole). The once-per-tick alive poll reports one enum for
            // the whole fight and cannot see a pet die at all.
            out << ", \"deaths\": ";
            WriteArray(out, r.deaths, [](std::ostream& o, std::pair<uint32, uint8> const& d)
            {
                o << '[' << d.first << ',' << uint32(d.second) << ']';
            });

            // This fight's own ability table. Compact array form rather than
            // objects: it repeats per iteration and the run-level table below
            // already names every column.
            // [spell, pet, kind, count, attempts, damage, min, max,
            //  healCount, healing, overheal, crits, critDamage, logged,
            //  healCrits]
            out << ", \"abilities\": ";
            WriteArray(out, r.abilities, [](std::ostream& o, auto const& e)
            {
                AbilityStat const& st = e.second;
                o << '[' << e.first.first << ',' << (e.first.second ? 1 : 0)
                  << ',' << uint32(st.kind) << ',' << st.count << ',' << st.attempts
                  << ',' << st.damage
                  << ',' << (st.count ? st.minHit : 0) << ',' << st.maxHit
                  << ',' << st.healCount << ',' << st.healing << ',' << st.overheal
                  << ',' << st.crits << ',' << st.critDamage << ',' << st.logged
                  << ',' << st.healCrits
                  << ']';
            });

            // Every damage and healing event, in order:
            // [ms, spell, amount, absorb, resist, preMitigation, flags, kind,
            //  damageKind]
            out << ", \"events\": ";
            WriteArray(out, r.events, [](std::ostream& o, CombatEvent const& e)
            {
                o << '[' << e.ms << ',' << e.spellId << ',' << e.amount << ','
                  << e.absorb << ',' << e.resist << ',' << e.preMitigation << ','
                  << e.flags << ',' << uint32(e.kind)
                  << ',' << uint32(e.damageKind) << ']';
            });

            // Uptime, applications and stacks per aura -- the "was this button
            // pressed, and kept up" table.
            // [spell, name, samplesPresent, applications, meanStacks, maxStacks,
            //  firstMs, lastMs]
            auto writeUptime = [&out](std::map<uint32, AuraUptime> const& acc)
            {
                out << '[';
                bool first = true;

                for (auto const& [spellId, u] : acc)
                {
                    if (!u.tracked || !u.samplesPresent)
                        continue;

                    if (!first)
                        out << ',';

                    first = false;
                    out << '[' << spellId << ',' << JsonStr(SpellName(spellId, "?"))
                        << ',' << u.samplesPresent << ',' << u.applications
                        << ',' << (double(u.stackSum) / double(u.samplesPresent))
                        << ',' << uint32(u.maxStacks)
                        << ',' << u.firstMs << ',' << u.lastMs << ']';
                }

                out << ']';
            };

            out << ", \"aura_samples\": " << r.auraSamples
                << ", \"auras_filtered\": " << r.aurasFiltered
                << ", \"aura_stacks_dropped\": " << r.auraStacksDropped
                << ", \"actor_uptime\": ";
            writeUptime(r.actorUptime);
            out << ", \"target_uptime\": ";
            writeUptime(r.targetUptime);

            // Change-only stack series: {"spell": [[ms, stacks], ...]}
            out << ", \"aura_stacks\": {";
            {
                bool first = true;
                for (auto const& [spellId, series] : r.auraStacks)
                {
                    if (!first)
                        out << ',';

                    first = false;
                    out << '"' << spellId << "\": ";
                    WriteArray(out, series,
                        [](std::ostream& o, std::pair<uint32, uint8> const& p)
                        {
                            o << '[' << p.first << ',' << uint32(p.second) << ']';
                        });
                }
            }
            out << '}';

            // [spell, name, absorbed, applications, consumed, expiredWithShield]
            out << ", \"absorb\": ";
            WriteArray(out, r.shields, [](std::ostream& o, auto const& e)
            {
                o << '[' << e.first << ',' << JsonStr(SpellName(e.first, "?"))
                  << ',' << e.second.absorbed << ',' << e.second.applications
                  << ',' << e.second.consumed << ',' << e.second.expiredFull << ']';
            });

            // [ms, actorHp, actorPower, targetHp, powerType]
            out << ", \"resources\": ";
            WriteArray(out, r.resources, [](std::ostream& o, ResourceSample const& s)
            {
                o << '[' << s.ms << ',' << s.actorHp << ',' << s.actorPower
                  << ',' << s.targetHp << ',' << uint32(s.powerType) << ']';
            });

            out << "}" << (i + 1 < s_results.size() ? "," : "") << "\n";
        }
        out << "  ],\n";

        // Per ability, biggest first. This is the number that can be argued
        // with: a redesigned talent that never appears here was never cast, and
        // a DPS figure built on it is measuring a phantom.
        std::vector<std::pair<std::pair<uint32, bool>, AbilityStat>> abilities(
            s_abilities.begin(), s_abilities.end());
        std::sort(abilities.begin(), abilities.end(),
            [](auto const& a, auto const& b) { return a.second.damage > b.second.damage; });

        static char const* const kindNames[] = { "unknown", "melee", "spell", "periodic" };

        uint64 totalHealing = 0, totalOverheal = 0;
        for (auto const& r : s_results)
        {
            totalHealing  += r.healing;
            totalOverheal += r.overheal;
        }

        out << "  \"unattributed_damage\": " << s_unattributedDamage << ",\n"
            // What the unattributed damage actually was, by the core's own
            // DamageEffectType. A single total says the breakdown is incomplete;
            // this says which mechanism is missing, which is the difference
            // between a known gap and a mystery.
            << "  \"unattributed_by_type\": {"
            << "\"direct\": "       << s_unattributedByType[DIRECT_DAMAGE]
            << ", \"spell\": "      << s_unattributedByType[SPELL_DIRECT_DAMAGE]
            << ", \"dot\": "        << s_unattributedByType[DOT]
            << ", \"heal\": "       << s_unattributedByType[HEAL]
            << ", \"nodamage\": "   << s_unattributedByType[NODAMAGE]
            << ", \"self\": "       << s_unattributedByType[SELF_DAMAGE]
            << "},\n"
            << "  \"unattributed_heals\": " << s_unattributedHeals << ",\n"
            // Why the attribution missed, rather than only how much.
            << "  \"latch_miss\": {\"no_latch\": " << s_latchMissNoLatch
            << ", \"attacker_mismatch\": " << s_latchMissAttacker << "},\n"
            // The quantum of every uptime and stack figure, so Python reports a
            // bounded number instead of implying an exact one.
            << "  \"aura_sample_ms\": " << s_auraSampleMs << ",\n"
            << "  \"resource_sample_ms\": " << s_resSampleMs << ",\n"
            << "  \"healing\": " << totalHealing << ",\n"
            << "  \"overheal\": " << totalOverheal << ",\n"
            // Nonzero means the attribution ring ran out of slots and a latch was
            // evicted before its event arrived. Emitted even at zero: a missing
            // key and a zero are different claims.
            << "  \"latch_overflow\": " << s_latchOverflow << ",\n"
            // Crits are real, read off the server's own combat-log packets --
            // no script hook carries the flag, but SMSG_ATTACKERSTATEUPDATE and
            // its siblings do, and a module can observe them via
            // PlayerbotScript::OnPlayerbotPacketSent.
            << "  \"crit_known\": true,\n"
            // How many damage events the log actually backed. A crit rate is
            // only meaningful out of `logged`, never out of `count`: if a packet
            // layout changes upstream and the decode stops matching, an
            // unguarded rate would quietly read as "nothing ever crits".
            << "  \"log_matched\": " << s_logMatched << ",\n"
            << "  \"log_unmatched\": " << s_logUnmatched << ",\n"
            << "  \"abilities\": [\n";

        for (std::size_t i = 0; i < abilities.size(); ++i)
        {
            uint32 const spellId = abilities[i].first.first;
            AbilityStat const& st = abilities[i].second;

            out << "    {\"spell\": " << spellId
                << ", \"name\": " << JsonStr(SpellName(spellId))
                << ", \"kind\": \"" << kindNames[st.kind < 4 ? st.kind : 0] << "\""
                << ", \"pet\": " << (st.fromPet ? "true" : "false")
                << ", \"count\": " << st.count
                << ", \"damage\": " << st.damage
                << ", \"share\": "
                << (totalDamage ? double(st.damage) / double(totalDamage) : 0.0)
                // attempts - count is the miss/dodge/parry count for melee, and
                // zero by construction for spells.
                << ", \"attempts\": " << st.attempts
                << ", \"min\": " << (st.count ? st.minHit : 0)
                << ", \"max\": " << st.maxHit
                << ", \"heal_count\": " << st.healCount
                << ", \"healing\": " << st.healing
                << ", \"overheal\": " << st.overheal
                << ", \"crits\": " << st.crits
                << ", \"crit_damage\": " << st.critDamage
                << ", \"heal_crits\": " << st.healCrits
                // Denominator for the crit rate; see log_matched above.
                << ", \"logged\": " << st.logged
                << "}" << (i + 1 < abilities.size() ? "," : "") << "\n";
        }

        out << "  ]\n"
            << "}\n";

        LOG_INFO("server.worldserver",
            "Simulator: {} iteration(s), {} damage over {:.1f}s = {:.1f} DPS "
            "({:.1f}x realtime) -> {}",
            s_results.size(), totalDamage, seconds, dps, realtimeFactor, s_cfg.outPath);

        // Who else was fighting, and where. This is the determinism blocker.
        for (auto const& [mapId, count] : s_foreignDamageByMap)
            LOG_INFO("server.worldserver",
                "Simulator: foreign damage events on map {}: {}", mapId, count);

        std::vector<std::pair<uint32, uint32>> top(
            s_foreignDamageByEntry.begin(), s_foreignDamageByEntry.end());
        std::sort(top.begin(), top.end(),
            [](auto const& a, auto const& b) { return a.second > b.second; });

        for (std::size_t i = 0; i < top.size() && i < 8; ++i)
        {
            CreatureTemplate const* ct = sObjectMgr->GetCreatureTemplate(top[i].first);
            LOG_INFO("server.worldserver",
                "Simulator: foreign attacker entry {} ({}) -> {} events",
                top[i].first, ct ? ct->Name : "?", top[i].second);
        }
    }

    // -- Actor / target ----------------------------------------------------

    bool RequestLogin()
    {
#ifndef MOD_PLAYERBOTS
        Fail("built without mod-playerbots; the simulator needs it for rotations");
        return false;
#else
        if (s_cfg.character.empty())
        {
            Fail("--sim-char <name> is required");
            return false;
        }

        ObjectGuid guid = sCharacterCache->GetCharacterGuidByName(s_cfg.character);
        if (!guid)
        {
            Fail(Acore::StringFormat("no character named '{}'", s_cfg.character));
            return false;
        }

        s_actorGuid = guid;

        // Asynchronous: AddPlayerBot queues a query holder and completes later,
        // which is why this is a state machine rather than straight-line code.
        // masterAccountId 0 takes the random-bot path, which needs no master.
        sRandomPlayerbotMgr.AddPlayerBot(guid, 0);
        return true;
#endif
    }

    // A bot logs in wherever it last logged out -- which is somewhere in the
    // populated world, on an arbitrary map. That is wrong twice over: the target
    // would be summoned at arena coordinates the actor is nowhere near, and every
    // tick would update the grids of a busy zone rather than an empty island.
    bool SendToArena(Player* actor)
    {
        if (actor->GetMapId() == s_cfg.mapId &&
            actor->GetExactDist2d(s_cfg.x, s_cfg.y) < 5.0f)
            return true;   // already there

        return actor->TeleportTo(s_cfg.mapId, s_cfg.x, s_cfg.y, s_cfg.z, s_cfg.o);
    }

    bool InArena(Player* actor)
    {
        return actor->IsInWorld()
            && !actor->IsBeingTeleported()
            && actor->GetMapId() == s_cfg.mapId
            && actor->GetExactDist2d(s_cfg.x, s_cfg.y) < 20.0f;
    }

    // Put the actor in a canonical state, independent of whatever it was doing
    // when it last logged out.
    //
    // This matters more than it looks. A bot is saved on logout, so a second run
    // loads the state the first run left behind -- and resetting only health and
    // cooldowns left mana carried over, which for a caster directly changes what
    // the rotation can afford to cast. Two runs on the same seed then diverge
    // because the actor did not start from the same place, not because the RNG
    // differed.
    void ResetActor(Player* actor)
    {
        actor->CombatStop(true);

        // NOT RemoveAllAuras. That removes passives too, and a talent tree is
        // mostly passives -- Ignite, Fire Power, Improved Scorch, Master of
        // Elements. Stripping them between fights left every actor fighting
        // with the active half of its spec and none of the scaling, which is
        // both a large silent understatement of every number and, for anything
        // whose rotation depends on a passive's effect, a rotation that never
        // advances.
        //
        // Form-dependent passives are re-applied when the form is re-entered,
        // which is why a shadow priest looked fine (Improved Shadowform came
        // back with Shadowform) while a fire mage did not.
        actor->RemoveAppliedAuras([](AuraApplication const* app)
            { return !app->GetBase()->IsPassive(); });
        actor->RemoveOwnedAuras([](Aura const* aura)
            { return !aura->IsPassive(); });

        actor->RemoveAllSpellCooldown();

        actor->SetHealth(actor->GetMaxHealth());

        // Resources start where a fight would actually start them: full for the
        // pools you open with, empty for the ones you build up.
        actor->SetPower(POWER_MANA,   actor->GetMaxPower(POWER_MANA));
        actor->SetPower(POWER_ENERGY, actor->GetMaxPower(POWER_ENERGY));
        actor->SetPower(POWER_FOCUS,  actor->GetMaxPower(POWER_FOCUS));
        actor->SetPower(POWER_RAGE,        0);
        actor->SetPower(POWER_RUNIC_POWER, 0);

        actor->ClearComboPoints();

        // Death knight runes are a resource too: a fight started on cooldown
        // runes is a different fight. Guarded by class -- SetRuneCooldown
        // dereferences m_runes, which only death knights have.
        if (actor->getClass() == CLASS_DEATH_KNIGHT)
            for (uint8 i = 0; i < MAX_RUNES; ++i)
                actor->SetRuneCooldown(i, 0);

        // Passives are spared here for the same reason they are spared on the
        // actor above, and the consequence of not sparing them is larger.
        //
        // A pet's passives are not merely scaling multipliers -- they ARE its
        // scaling. Pet::addSpell (Pet.cpp:1866) applies every passive pet spell
        // with a single CastSpell at learn time and nothing re-applies it, so
        // Hunter Pet Scaling 01-04 (34902-34904, 61017), which is what carries
        // the owner's attack power, stamina and resistances onto the pet, is a
        // one-shot aura. RemoveAllAuras deleted it permanently: from that
        // iteration on the pet fought on its own base creature stats.
        //
        // Measured, matrix-20260814-163331: pet melee landed either 570-725 per
        // hit or 104-138, flat for a whole iteration and never transitioning
        // mid-fight, and the low state was missing Alonecraft's own pet passives
        // (Bite Back 200752, Taste for Blood 200746) entirely -- because those
        // are passives too and went the same way. That 4x split, appearing in a
        // different iteration for each hunter spec, is the whole of the "hunter
        // pet variance" that made all three hunter specs untrustworthy (CV
        // 17-47% against 8% or less for every spec without a pet).
        // GetGuardianPet for the reason given in SnapshotPet: GetPet() misses a
        // permanent pet attached as a plain guardian, and missing it here means
        // the pet starts the fight on whatever health the last one left it.
        if (Guardian* pet = actor->GetGuardianPet())
        {
            pet->RemoveAppliedAuras([](AuraApplication const* app)
                { return !app->GetBase()->IsPassive(); });
            pet->RemoveOwnedAuras([](Aura const* aura)
                { return !aura->IsPassive(); });

            pet->SetHealth(pet->GetMaxHealth());
            pet->SetPower(POWER_FOCUS, pet->GetMaxPower(POWER_FOCUS));
        }
    }

    // -- Talents and gear --------------------------------------------------
    //
    // Without this the actor fights in whatever it logged out wearing, which for
    // a fresh sim character is nothing: 15,050 HP at level 80 and a DPS number
    // that measures the absence of gear rather than the spec. Everything here
    // goes through PlayerbotFactory, so the actor is configured by exactly the
    // code that configures a live random bot -- talents, gear, glyphs, enchants,
    // gems, ammo, consumables and pet, in that order and with those rules.

#ifdef MOD_PLAYERBOTS
    // Specs are named in playerbots.conf (AiPlayerbot.PremadeSpecName.<class>.<n>)
    // rather than numbered here, so "--sim-spec 'shadow pve'" survives the list
    // being reordered and a typo names the alternatives instead of silently
    // simming the wrong tree.
    int ResolveSpec(uint8 cls, std::string const& want)
    {
        if (!want.empty() && want.find_first_not_of("0123456789") == std::string::npos)
            return std::stoi(want);

        std::string const needle = Lower(want);
        int fallback = -1;

        for (int i = 0; i < MAX_SPECNO; ++i)
        {
            std::string const name = Lower(sPlayerbotAIConfig.premadeSpecName[cls][i]);
            if (name.empty())
                continue;

            if (name == needle)
                return i;
            if (fallback < 0 && name.find(needle) != std::string::npos)
                fallback = i;
        }

        return fallback;
    }

    void LogAvailableSpecs(uint8 cls)
    {
        for (int i = 0; i < MAX_SPECNO; ++i)
            if (!sPlayerbotAIConfig.premadeSpecName[cls][i].empty())
                LOG_ERROR("server.worldserver", "Simulator:   --sim-spec '{}'  (index {})",
                    sPlayerbotAIConfig.premadeSpecName[cls][i], i);
    }

    // Re-learn every spell the actor knows but cannot cast.
    //
    // `character_spell.specMask` is a bitmask of the talent specs a spell is
    // active in, and 0 is a deliberate tombstone: Player.cpp:3381 keeps talent
    // spells in the table "when not available in any spec" rather than deleting
    // them. A tombstoned spell fails HasSpell, so the bot genuinely does not
    // have it -- while the row still sits in the database looking learned.
    //
    // PlayerbotFactory::Randomize creates these. It calls resetTalents() first,
    // which tombstones each talent's rank-1 spell, and the trainer pass that
    // follows cannot teach rank 2 of anything whose rank 1 is tombstoned -- so
    // an entire rank chain stays dead. Measured on a shadow priest: 17 of 239
    // spell rows at specMask 0, comprising every rank of Mind Flay and Vampiric
    // Touch plus Vampiric Embrace, Shadowform and Dispersion. The bot fell
    // through its whole rotation to Smite and measured 574 DPS instead of 1171.
    //
    // This is not the simulator's bug to fix properly -- it is upstream, in the
    // interaction between resetTalents and the factory's trainer pass. But a
    // simulator that silently measures a crippled actor is worse than useless,
    // so it repairs what it can and says loudly what it could not.
    // Talents that were taken but whose spell was never learned.
    //
    // A different failure from the tombstone below, with the same symptom and a
    // worse hiding place. `character_talent` records the talent; `m_spells`
    // never gets the spell; and RepairSpecMask cannot see it, because it walks
    // the *spell* map and a spell that was never added is not in it. So the
    // uncastable count reads zero and the run looks clean.
    //
    // Measured on a fire mage re-specced from frost: 27 talents recorded,
    // Improved Scorch rank 3 among them, and 12873 absent from the spell map
    // entirely. The consequence was total -- the bot has a rule that says
    // "Scorch until the target has Improved Scorch (22959)", the passive that
    // applies it was never on the mage, so it cast Scorch 157 times in two
    // minutes and Fireball not once.
    //
    // Everything a talent grants goes through the same path, so this is not one
    // talent's problem: it is every passive in the tree.
    uint32 RepairTalentSpells(Player* actor)
    {
        uint8 const activeSpec = actor->GetActiveSpecMask();

        std::vector<uint32> missing;
        for (auto const& [spellId, talent] : actor->GetTalentMap())
        {
            if (!talent || talent->State == PLAYERSPELL_REMOVED)
                continue;
            if (!(talent->specMask & activeSpec))
                continue;
            if (actor->HasSpell(spellId))
                continue;

            missing.push_back(spellId);
        }

        for (uint32 spellId : missing)
            actor->learnSpell(spellId);

        if (!missing.empty())
            LOG_WARN("server.worldserver",
                "Simulator: learned {} talent spell(s) that were taken but never "
                "granted. Without this the passives of the spec simply do not exist.",
                missing.size());

        return uint32(missing.size());
    }

    uint32 RepairSpecMask(Player* actor)
    {
        uint8 const activeSpec = actor->GetActiveSpecMask();

        // specMask == 0 exactly, NOT "does not match the active spec".
        //
        // Those are very different sets, and the difference was silently
        // undoing the talent reset on every run. resetTalents removes a talent's
        // auras but keeps passive talent *spells* in the spell map, clearing the
        // active bit from their specMask (Player.cpp:3779 -- removeSpell is
        // called only for spells that go in the spell book and are not passive).
        // A cleared bit is the core saying "this belongs to a spec you are not
        // in", and it is the correct, deliberate way to deactivate a tree.
        //
        // Testing !(specMask & activeSpec) selected exactly that set and
        // re-learned it, and learnSpell on a passive re-applies its aura. So the
        // core deactivated the old tree and this function reactivated it,
        // cumulatively, on a character that is saved between runs.
        //
        // Measured: a fire mage at talent tabs 13/58/0 carrying Ice Shards,
        // Shatter, Arctic Winds, Piercing Ice, Improved Frostbolt, Convection
        // and Catabatic Winds -- the whole frost tree, with no points in it.
        // Beyond inflating every number, AiFactory branches the mage rotation on
        // HasAura(SPELL_ICE_SHARDS), so the bot cast Frostfire Bolt for 45% of
        // its damage on a build that cannot support it.
        //
        // specMask 0 is the real defect this was written for and is still
        // repaired: no spec at all can cast it, which is a tombstone rather than
        // a deactivation.
        //
        // AND THAT REASONING WAS STILL WRONG, for the case the paragraph above
        // does not cover: a character with ONE spec. resetTalents calls
        // removeSpell(id, GetActiveSpecMask()) (Player.cpp:3782), which clears
        // the active bit -- and when that is the only bit, the result is
        // specMask 0. So on a single-spec actor, deactivation and tombstone are
        // the same value and the mask cannot tell them apart. The fix above
        // narrowed the set and did not eliminate it.
        //
        // Every sim actor is single-spec, and there is one actor per CLASS
        // shared by all three of its specs (tools/sim_specs.py), so each run
        // deactivated the previous spec's tree and this function learned it
        // straight back.
        //
        // Measured: warrior_fury at tabs 18/53/0 -- zero points in Protection --
        // carrying Shield Specialization, Shield Mastery, Improved Revenge,
        // Improved Thunder Clap, Anticipation and Defiance. Two of those are
        // avoidance, and the actor parried 14 of 18 incoming swings for a
        // reported 89% avoidance on a spec that has none. 53 spells were
        // "repaired" on that run.
        //
        // The mask cannot distinguish the two cases, so ask a different
        // question: does the actor actually have this talent *now*? A talent
        // spell that is not in the current talent map belongs to a tree that was
        // deliberately deactivated, whatever its mask says.
        std::set<uint32> currentTalentSpells;
        for (auto const& [spellId, talent] : actor->GetTalentMap())
        {
            if (!talent || talent->State == PLAYERSPELL_REMOVED)
                continue;
            if (!(talent->specMask & activeSpec))
                continue;

            currentTalentSpells.insert(spellId);

            // A talent can teach further spells, which are talent spells by
            // GetTalentSpellPos but are not themselves keys in the talent map.
            if (SpellInfo const* info = sSpellMgr->GetSpellInfo(spellId))
                for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
                    if (info->Effects[i].Effect == SPELL_EFFECT_LEARN_SPELL)
                        currentTalentSpells.insert(info->Effects[i].TriggerSpell);
        }

        std::vector<uint32> broken;
        std::vector<uint32> deactivated;

        for (auto const& [spellId, spell] : actor->GetSpellMap())
        {
            if (!spell || spell->State == PLAYERSPELL_REMOVED)
                continue;
            if (spell->specMask != 0)
                continue;

            // Not a talent spell at all -- a genuine tombstone, repair it.
            if (!GetTalentSpellPos(spellId))
            {
                broken.push_back(spellId);
                continue;
            }

            if (currentTalentSpells.count(spellId))
                broken.push_back(spellId);
            else
                deactivated.push_back(spellId);
        }

        if (!deactivated.empty())
            LOG_INFO("server.worldserver",
                "Simulator: left {} deactivated talent spell(s) alone -- they "
                "belong to a tree this spec has no points in. Re-learning them is "
                "what previously gave a 0-point Protection warrior its avoidance.",
                deactivated.size());

        uint32 repaired = 0;
        std::vector<uint32> stubborn;

        for (uint32 spellId : broken)
        {
            actor->learnSpell(spellId);

            if (actor->HasSpell(spellId))
                ++repaired;
            else
                stubborn.push_back(spellId);
        }

        if (repaired)
            LOG_WARN("server.worldserver",
                "Simulator: repaired {} spell(s) that were known but not castable "
                "(specMask 0). This is a PlayerbotFactory::Randomize defect, not a "
                "balance result -- without the repair the actor cannot use its spec.",
                repaired);

        for (uint32 spellId : stubborn)
        {
            SpellInfo const* info = sSpellMgr->GetSpellInfo(spellId);
            LOG_ERROR("server.worldserver",
                "Simulator: spell {} ({}) is known but NOT castable and could not be "
                "repaired -- any DPS measured here is a floor, not a measurement.",
                spellId, (info && info->SpellName[0]) ? info->SpellName[0] : "?");
        }

        return uint32(stubborn.size());
    }

    // -- Fixed gear --------------------------------------------------------
    //
    // PlayerbotFactory gears by score out of whatever the loot tables yield at
    // the actor's level. That is right for a random bot and wrong for a balance
    // matrix: two specs measured in two different sets differ by the gear as
    // much as by the spec, and the factory's choice is not even stable between
    // runs of the same spec.
    //
    // --sim-gear replaces the entire equipped set with a named one:
    //
    //     item[:enchant[:gem,gem,gem]];item[:enchant[:gems]];...
    //
    // Enchant ids are SpellItemEnchantment ids. Gems are gem *item* ids resolved
    // through GemProperties, exactly as CMSG_SOCKET_GEMS does -- so a set can be
    // copied verbatim out of a planner without translating anything.
    // tools/sim.py builds the string from sims/gear/*.json.
    //
    // The old set is destroyed rather than swapped: equipping into an occupied
    // slot needs somewhere to put the displaced item, and a full bag would
    // silently leave the old one on -- which reads as a gear bug in the results
    // long after the run. An empty slot afterwards is a *reported* failure,
    // which is the honest outcome: a spec that could equip only 15 of its 17
    // pieces should not be compared against one that equipped all 17.

    struct GearPiece
    {
        uint32 item    = 0;
        uint32 enchant = 0;
        uint32 gems[3] = { 0, 0, 0 };
    };

    std::vector<std::string> Split(std::string const& s, char sep)
    {
        std::vector<std::string> out;
        std::string cur;

        for (char c : s)
        {
            if (c == sep)
            {
                out.push_back(cur);
                cur.clear();
            }
            else
                cur += c;
        }

        out.push_back(cur);
        return out;
    }

    std::vector<GearPiece> ParseGear(std::string const& text)
    {
        std::vector<GearPiece> out;

        for (std::string const& entry : Split(text, ';'))
        {
            if (entry.empty())
                continue;

            std::vector<std::string> const parts = Split(entry, ':');
            GearPiece piece;

            piece.item = uint32(std::stoul(parts[0]));

            if (parts.size() > 1 && !parts[1].empty())
                piece.enchant = uint32(std::stoul(parts[1]));

            if (parts.size() > 2)
            {
                std::vector<std::string> const gems = Split(parts[2], ',');
                for (std::size_t i = 0; i < gems.size() && i < 3; ++i)
                    if (!gems[i].empty())
                        piece.gems[i] = uint32(std::stoul(gems[i]));
            }

            out.push_back(piece);
        }

        return out;
    }

    void ApplyGems(Player* actor, Item* item, GearPiece const& piece)
    {
        ItemTemplate const* proto = item->GetTemplate();
        if (!proto)
            return;

        for (uint8 i = 0; i < MAX_ITEM_PROTO_SOCKETS; ++i)
        {
            if (!piece.gems[i])
                continue;

            ItemTemplate const* gem = sObjectMgr->GetItemTemplate(piece.gems[i]);
            GemPropertiesEntry const* props =
                gem ? sGemPropertiesStore.LookupEntry(gem->GemProperties) : nullptr;

            if (!props || !props->spellitemenchantement)
            {
                LOG_WARN("server.worldserver",
                    "Simulator: gem {} on item {} has no enchantment; socket left empty",
                    piece.gems[i], piece.item);
                continue;
            }

            EnchantmentSlot const slot = EnchantmentSlot(SOCK_ENCHANTMENT_SLOT + i);
            actor->ApplyEnchantment(item, slot, false);
            item->SetEnchantment(slot, props->spellitemenchantement, 0, 0, actor->GetGUID());
            actor->ApplyEnchantment(item, slot, true);
        }

        // The socket bonus, decided by the core's own predicate rather than by
        // counting colours here.
        //
        // This used to hand-roll the test -- count coloured sockets, count gems
        // whose colour bit-ANDs the socket's, activate if the two match. That
        // gets the common case right and diverges on the rest: Item::GemsFitSockets
        // also rejects a socket whose enchantment row does not resolve, and
        // handles prismatic and meta colours, which a plain bitwise AND does not.
        // It is what WorldSession::HandleSocketOpcode uses, so using it here is
        // what makes a simulated set of gear the same as a played one.
        bool const fits = item->GemsFitSockets();
        if (proto->socketBonus)
        {
            actor->ApplyEnchantment(item, BONUS_ENCHANTMENT_SLOT, false);
            item->SetEnchantment(BONUS_ENCHANTMENT_SLOT, fits ? proto->socketBonus : 0,
                0, 0, actor->GetGUID());
            actor->ApplyEnchantment(item, BONUS_ENCHANTMENT_SLOT, true);
        }
    }

    // Everything wearable that is not currently worn.
    //
    // Alonecraft's item upgrade variants share an ItemLimitCategory with their
    // base item, which is what stops a player carrying five copies of the same
    // trinket at five upgrade tiers. It also means a *variant sitting in a bag*
    // blocks the base item from being equipped, with EQUIP_ERR_CANT_CARRY_MORE_
    // OF_THIS -- and PlayerbotFactory fills the bags with spare gear. Ten of the
    // first thirty-one runs lost a trinket this way, all of them silently until
    // the failure was counted.
    //
    // Consumables, reagents and ammo are left alone: the rotation uses them, and
    // they cannot collide with equipment.
    void ClearSpareEquipment(Player* actor)
    {
        std::vector<Item*> spare;

        auto consider = [&spare](Item* item)
        {
            ItemTemplate const* proto = item ? item->GetTemplate() : nullptr;
            if (proto && (proto->Class == ITEM_CLASS_WEAPON || proto->Class == ITEM_CLASS_ARMOR))
                spare.push_back(item);
        };

        for (uint8 slot = INVENTORY_SLOT_ITEM_START; slot < INVENTORY_SLOT_ITEM_END; ++slot)
            consider(actor->GetItemByPos(INVENTORY_SLOT_BAG_0, slot));

        for (uint8 bag = INVENTORY_SLOT_BAG_START; bag < INVENTORY_SLOT_BAG_END; ++bag)
            if (Bag* container = actor->GetBagByPos(bag))
                for (uint32 slot = 0; slot < container->GetBagSize(); ++slot)
                    consider(container->GetItemByPos(slot));

        // Collected first: DestroyItem mutates the containers being walked.
        for (Item* item : spare)
            actor->DestroyItem(item->GetBagSlot(), item->GetSlot(), true);
    }

    void EquipFixedGear(Player* actor, std::string const& text)
    {
        std::vector<GearPiece> const pieces = ParseGear(text);
        if (pieces.empty())
        {
            Fail("--sim-gear was given but parsed to nothing");
            return;
        }

        for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; ++slot)
            if (actor->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
                actor->DestroyItem(INVENTORY_SLOT_BAG_0, slot, true);

        ClearSpareEquipment(actor);

        s_gearEquipped = 0;
        s_gearFailed   = 0;

        // Two passes. A failure is not always about the item: a fury warrior's
        // off-hand two-hander is refused until Titan's Grip is on, and a weapon
        // can be refused because of what is in the slot it was offered. Anything
        // that fails once is retried after every other piece is settled, and
        // only a second failure is counted.
        std::vector<GearPiece> retry;

        for (int pass = 0; pass < 2; ++pass)
        {
            std::vector<GearPiece> const& batch = pass ? retry : pieces;

            for (GearPiece const& piece : batch)
            {
                // Profession-gated pieces. BiS lists are written for players who
                // have the profession -- the tier-7 sets include Jewelcrafting
                // figurines (RequiredSkill 755) for three specs -- and a bot has
                // no professions, so the item is refused and the spec quietly
                // fights a trinket short. Granting the skill is the honest fix:
                // the comparison is between rotations in a fixed set of gear,
                // and "this spec is missing a trinket because its actor cannot
                // craft" is not a balance result.
                if (ItemTemplate const* proto = sObjectMgr->GetItemTemplate(piece.item))
                    if (proto->RequiredSkill && !actor->HasSkill(proto->RequiredSkill))
                        actor->SetSkill(proto->RequiredSkill, 0,
                            proto->RequiredSkillRank, proto->RequiredSkillRank);

                uint16 dest = 0;
                InventoryResult const can = actor->CanEquipNewItem(NULL_SLOT, dest, piece.item, false);

                if (can != EQUIP_ERR_OK)
                {
                    if (pass == 0)
                    {
                        retry.push_back(piece);
                        continue;
                    }

                    LOG_ERROR("server.worldserver",
                        "Simulator: cannot equip item {} (InventoryResult {}) -- slot left empty",
                        piece.item, uint32(can));
                    ++s_gearFailed;
                    continue;
                }

                Item* item = actor->EquipNewItem(dest, piece.item, true);
                if (!item)
                {
                    LOG_ERROR("server.worldserver",
                        "Simulator: EquipNewItem failed for item {} -- slot left empty", piece.item);
                    ++s_gearFailed;
                    continue;
                }

                if (piece.enchant)
                {
                    actor->ApplyEnchantment(item, PERM_ENCHANTMENT_SLOT, false);
                    item->SetEnchantment(PERM_ENCHANTMENT_SLOT, piece.enchant, 0, 0, actor->GetGUID());
                    actor->ApplyEnchantment(item, PERM_ENCHANTMENT_SLOT, true);
                }

                ApplyGems(actor, item, piece);
                ++s_gearEquipped;
            }
        }

        // Meta gems are inert until this runs, and every P1 set has one.
        //
        // WorldSession::HandleSocketOpcode ends with exactly this call, because
        // a meta gem's enchantment is conditional -- "requires at least N red
        // gems", and so on -- and the condition is only evaluated here. Socketing
        // without it leaves the gem in the item, visible and counted, granting
        // nothing.
        //
        // Called once after the whole set is on rather than per item, which is
        // the difference from the handler: it socketes one item at a time and
        // excludes the one in hand, whereas by this point every piece and every
        // gem is in place and the conditions can be judged against the finished
        // set. EQUIPMENT_SLOT_END is passed as the exception slot precisely
        // because there is no item to exclude.
        actor->ToggleMetaGemsActive(EQUIPMENT_SLOT_END, true);

        // Mean item level over what is actually worn, excluding shirt and
        // tabard, which have none. This is the number that says whether two
        // specs were compared on equal footing, so it is reported rather than
        // assumed from the name of the set.
        uint32 total = 0;
        uint32 count = 0;

        for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; ++slot)
        {
            if (slot == EQUIPMENT_SLOT_BODY || slot == EQUIPMENT_SLOT_TABARD)
                continue;

            Item* item = actor->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
            if (!item || !item->GetTemplate())
                continue;

            total += item->GetTemplate()->ItemLevel;
            ++count;
        }

        s_gearIlvl = count ? total / count : 0;

        LOG_INFO("server.worldserver",
            "Simulator: fixed gear applied -- {} equipped, {} failed, mean ilvl {}",
            s_gearEquipped, s_gearFailed, s_gearIlvl);
    }

    bool ConfigureActor(Player* actor)
    {
        if (s_cfg.spec.empty())
        {
            LOG_WARN("server.worldserver",
                "Simulator: no --sim-spec given -- running '{}' as stored (level {}, {} HP). "
                "Pass --sim-spec to gear and spec it.",
                s_cfg.character, uint32(actor->GetLevel()), actor->GetMaxHealth());

            // Still worth checking: the actor may carry tombstones from an
            // earlier configured run.
            s_uncastableSpells = RepairSpecMask(actor);
            return true;
        }

        uint8 const cls = actor->getClass();

        int const spec = ResolveSpec(cls, s_cfg.spec);
        if (spec < 0 || spec >= MAX_SPECNO ||
            sPlayerbotAIConfig.premadeSpecName[cls][spec].empty())
        {
            LOG_ERROR("server.worldserver",
                "Simulator: no spec matching '{}' for class {}. Available:", s_cfg.spec, uint32(cls));
            LogAvailableSpecs(cls);
            Fail("unknown --sim-spec");
            return false;
        }

        uint32 const level = s_cfg.level ? s_cfg.level : actor->GetLevel();

        // PlayerbotFactory chooses the tree by weighted roll over
        // randomClassSpecProb, with no parameter to override it. Pinning the
        // weights is what turns that roll into a choice -- and it has to happen
        // before Randomize(), because gear is picked to suit the talents it just
        // assigned. Restored immediately after, so nothing else on this process
        // inherits the pin.
        uint32 savedProb[MAX_SPECNO];
        for (int i = 0; i < MAX_SPECNO; ++i)
        {
            savedProb[i] = sPlayerbotAIConfig.randomClassSpecProb[cls][i];
            sPlayerbotAIConfig.randomClassSpecProb[cls][i] = (i == spec) ? 100 : 0;
        }

        LOG_INFO("server.worldserver", "Simulator: configuring '{}' as level {} '{}'...",
            s_cfg.character, level, sPlayerbotAIConfig.premadeSpecName[cls][spec]);

        // Pinning the weights is not enough on its own.
        // PlayerbotFactory::InitTalentsTree applies its template with reset =
        // false, so the template's points are spent out of whatever is *free* on
        // top of the talents the character already has. A character stored as
        // holy stays holy: the retribution template has nothing left to spend.
        //
        // This is silent and it is not subtle in the results. Of the first
        // thirty-one specs run, every paladin measured a holy rotation (60 DPS
        // for "retribution", topped by Holy Shock), every druid measured
        // balance -- "cat" and "bear" included, casting Starfire -- and both
        // healing priests measured shadow. The class's stored spec, three times
        // over, wearing three different sets of gear.
        //
        // resetTalents first is the whole fix. It has to happen before
        // Randomize rather than after, because gear is chosen to suit the
        // talents, and it is why RepairSpecMask below exists: resetting is what
        // tombstones the rank-1 spells.
        actor->resetTalents(true);

        PlayerbotFactory factory(actor, level);
        factory.Randomize(false);

        for (int i = 0; i < MAX_SPECNO; ++i)
            sPlayerbotAIConfig.randomClassSpecProb[cls][i] = savedProb[i];

        // After Randomize, so it overwrites the rolled set rather than being
        // overwritten by it. InitAmmo has to run again afterwards: it stocks
        // arrows or bullets to match the ranged weapon, and the weapon just
        // changed -- a hunter holding a bow and a bag of bullets does not shoot.
        if (!s_cfg.gear.empty())
        {
            EquipFixedGear(actor, s_cfg.gear);
            if (!s_cfg.active || s_state == SIM_DONE)
                return false;

            factory.InitAmmo();
        }

        // After Randomize, before anything measures the actor. Order matters:
        // learning a missing talent spell can itself land as a tombstone, so
        // the specMask repair has to run second.
        s_talentSpellsLearned = RepairTalentSpells(actor);
        s_uncastableSpells    = RepairSpecMask(actor);

        if (PlayerbotAI* botAI = sPlayerbotsMgr.GetPlayerbotAI(actor))
            botAI->Reset(true);

        // The numbers that make a DPS figure interpretable. If attack power and
        // spell power are both near their unbuffed floor, the gearing step did
        // not take and the run is measuring nothing.
        // Points per talent tab, so the result can be checked against the spec
        // that was asked for instead of trusting that it took. This is the
        // evidence that would have caught the reset bug above on the first run
        // rather than after thirty-one of them.
        std::map<uint8, uint32> const tabs = AiFactory::GetPlayerSpecTabs(actor);
        for (uint8 i = 0; i < 3; ++i)
            s_talentTabs[i] = tabs.count(i) ? tabs.at(i) : 0;

        LOG_INFO("server.worldserver", "Simulator: talent points per tab: {}/{}/{}",
            s_talentTabs[0], s_talentTabs[1], s_talentTabs[2]);

        s_actorAttackPower = uint32(actor->GetTotalAttackPowerValue(BASE_ATTACK));
        s_actorSpellPower  = uint32(actor->GetBaseSpellPowerBonus());
        s_actorCrit        = actor->GetFloatValue(PLAYER_CRIT_PERCENTAGE);

        LOG_INFO("server.worldserver",
            "Simulator: actor configured -- level {} hp {} mana {} AP {} SP(arcane..) {} crit {:.2f}%",
            uint32(actor->GetLevel()), actor->GetMaxHealth(), actor->GetMaxPower(POWER_MANA),
            actor->GetTotalAttackPowerValue(BASE_ATTACK),
            actor->GetBaseSpellPowerBonus(),
            actor->GetFloatValue(PLAYER_CRIT_PERCENTAGE));

        return true;
    }
#else
    bool ConfigureActor(Player*) { return true; }
#endif

    // -- Engagement --------------------------------------------------------
    //
    // A playerbot runs its rotation only in BOT_STATE_COMBAT, and it reaches
    // that state through AttackAction, which is normally driven by a
    // target-selection strategy scanning for something worth grinding.  None of
    // that applies in an empty arena.
    //
    // The first version of this made the dummy AGGRESSIVE so the bot acquired it
    // through the ordinary threat path.  That worked, but it bought engagement
    // with two costs: the target fights back, so it pollutes the very number
    // being measured (a priest starts healing itself, a warrior gains rage it
    // would not otherwise have), and it is *unreliable* -- one run in two
    // measured 0 damage because the AttackStart handshake did not take.
    //
    // Doing exactly what AttackAction::Attack does, minus the target scan, is
    // both cleaner and deterministic. The three lines below are the whole of it:
    // selection, the AI's "current target" value, and the engine switch.
    bool EngageActor(Player* actor, Creature* target)
    {
#ifdef MOD_PLAYERBOTS
        PlayerbotAI* botAI = sPlayerbotsMgr.GetPlayerbotAI(actor);
        if (!botAI)
        {
            // Without a rotation the actor only swings its weapon, so a caster
            // reads as ~0 DPS and looks like a balance problem rather than a
            // missing bot AI. Fail rather than publish that number.
            Fail(Acore::StringFormat(
                "'{}' has no PlayerbotAI -- it would not use a rotation, and the "
                "resulting DPS would be meaningless", s_cfg.character));
            return false;
        }

        actor->SetSelection(target->GetGUID());
        botAI->GetAiObjectContext()->GetValue<Unit*>("current target")->Set(target);
        botAI->ChangeEngine(BOT_STATE_COMBAT);
#else
        actor->SetSelection(target->GetGUID());
#endif

        actor->Attack(target, true);
        return true;
    }

    bool SpawnTargetAndPosition(Player* actor)
    {
        if (!s_cfg.targetEntry)
        {
            Fail("--sim-target <creature entry> is required");
            return false;
        }

        Map* map = actor->GetMap();
        if (!map)
        {
            Fail("actor has no map");
            return false;
        }

        // Relative to where the actor actually stands, not to the configured
        // arena centre -- the teleport lands on real terrain height, which is
        // not necessarily the configured z.
        Position pos(actor->GetPositionX() + s_cfg.range, actor->GetPositionY(),
            actor->GetPositionZ(), s_cfg.o);

        Creature* target = actor->SummonCreature(s_cfg.targetEntry, pos,
            TEMPSUMMON_MANUAL_DESPAWN);

        if (!target)
        {
            Fail(Acore::StringFormat("could not summon creature {}", s_cfg.targetEntry));
            return false;
        }

        // Whether mod-autobalance can see this fight at all, reported rather
        // than assumed. It needs BOTH map->IsDungeon() and a non-zero instance
        // id (ABAllCreatureScript.cpp:98-104, ABUtils.cpp ShouldMapBeEnabled),
        // and neither is visible from the command line -- a raw TeleportTo can
        // land on an instanceable map's *base* map, where the id is 0 and every
        // creature is silently left at raid-tuned numbers.
        //
        // The alternative was inferring it from the `instance` table, which is a
        // false negative: an in-memory instance is only persisted once something
        // binds to it.
        if (Map* m = target->GetMap())
            LOG_INFO("server.worldserver",
                "Simulator: arena map {} instanceId={} isDungeon={} -- "
                "autobalance {}.",
                m->GetId(), m->GetInstanceId(), m->IsDungeon() ? 1 : 0,
                (m->IsDungeon() && m->GetInstanceId()) ? "can apply"
                                                       : "CANNOT apply");

        // Two kinds of target: one that fights back and one that does not.
        //
        // An inert dummy is rooted and passive and contributes nothing but a
        // health bar and an armour value -- its sim_target_dummy script gives it
        // NullCreatureAI, whose no-op EnterEvadeMode is what stops a target that
        // is attacked but never attacks back from resetting the fight halfway
        // through. That is what a clean DPS number needs.
        //
        // Anything else keeps its own AI, script, abilities and aggression,
        // because time-to-die, TMI and solo-clear are meaningless against
        // something that does not fight back.
        //
        // This used to test the entry range 2000100-2000199 instead, which
        // conflated "is a simulator fixture" with "is inert". The sparring dummy
        // (2000110) is both a fixture and a fighter, and inside the old test it
        // was silently rooted and set passive: three 75-second fights, actor
        // ending on 100% health, damage_taken of exactly 0. The template said
        // 2.0x damage and the creature never swung once.
        //
        // The script name is the honest discriminator, because it is the thing
        // that actually makes a target inert.
        uint32 const inertScriptId = sObjectMgr->GetScriptId("sim_target_dummy");
        bool const isDummy = target->GetCreatureTemplate()->ScriptID == inertScriptId
            && inertScriptId != 0;

        if (isDummy)
        {
            target->SetReactState(REACT_PASSIVE);
            target->SetControlled(true, UNIT_STATE_ROOT);
        }
        else
        {
            target->SetReactState(REACT_AGGRESSIVE);
            if (target->AI())
                target->AI()->AttackStart(actor);
        }

        s_targetGuid = target->GetGUID();

        // NOTE: the actor is deliberately *not* reset here. Reset happens in
        // PrepareIteration, before the buff window, so that the buffs the bot
        // re-applies during that window survive into the fight.
        if (!EngageActor(actor, target))
            return false;

        LOG_INFO("server.worldserver",
            "Simulator: actor class={} level={} hp={} | target entry={} level={} hp={} armor={}",
            uint32(actor->getClass()), uint32(actor->GetLevel()), actor->GetMaxHealth(),
            s_cfg.targetEntry, uint32(target->GetLevel()), target->GetMaxHealth(),
            target->GetArmor());

        s_actorLevel     = actor->GetLevel();
        s_actorMaxHealth = uint32(actor->GetMaxHealth());

        return true;
    }

    void MaintainEngagement()
    {
        Player* actor = ObjectAccessor::FindConnectedPlayer(s_actorGuid);
        if (!actor || !actor->IsInWorld())
            return;

        Unit* target = ObjectAccessor::GetUnit(*actor, s_targetGuid);
        if (!target || !target->IsAlive())
            return;

        // The target never attacks, so nothing external keeps the bot engaged:
        // if its combat engine drops the target for any reason -- an interrupted
        // cast, a lapsed combat timer -- it stays dropped. Re-asserting once a
        // virtual second is cheap and is what makes a 60-second measurement
        // actually 60 seconds of fighting.
#ifdef MOD_PLAYERBOTS
        if (PlayerbotAI* botAI = sPlayerbotsMgr.GetPlayerbotAI(actor))
        {
            if (botAI->GetAiObjectContext()->GetValue<Unit*>("current target")->Get() != target)
                botAI->GetAiObjectContext()->GetValue<Unit*>("current target")->Set(target);

            if (botAI->GetState() != BOT_STATE_COMBAT)
                botAI->ChangeEngine(BOT_STATE_COMBAT);
        }
#endif

        if (actor->GetVictim() != target)
        {
            actor->SetSelection(target->GetGUID());
            actor->Attack(target, true);
        }
    }

    // -- The target --------------------------------------------------------
    //
    // NullCreatureAI is the whole implementation, and every one of its no-ops is
    // load-bearing here: AttackStart and UpdateAI mean the target never hits
    // back, and EnterEvadeMode means a target that is attacked for sixty seconds
    // without ever attacking cannot decide the fight is over and reset. It keeps
    // real health and real armour, so the damage taken is computed exactly as it
    // would be against any other creature -- unlike core's npc_training_dummy,
    // which zeroes damage and would report every spec at 0 DPS.
    class sim_target_dummy : public CreatureScript
    {
    public:
        sim_target_dummy() : CreatureScript("sim_target_dummy") { }

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new NullCreatureAI(creature);
        }
    };

    // -- The sparring target -----------------------------------------------
    //
    // The solo-clear pass needs the opposite of the dummy above in one respect
    // and the same in another: it must fight back, and it must still never
    // evade.
    //
    // Stock AI resets a creature whose target gets too far away or breaks line
    // of sight, and a solo caster does both constantly -- Frost Nova, then back
    // up and keep casting. Measured, that produced an iteration where the target
    // sat at 100% health, the actor recorded zero damage events across the whole
    // 120 seconds, and the run's mean was quietly computed over a fight that
    // never happened. The sim's own zero-damage guard caught it, which is the
    // only reason it was not believed.
    //
    // So: AggressorAI for real attacks, with EnterEvadeMode suppressed. The
    // fight ends when something dies or the clock runs out, which are the only
    // two outcomes solo-clear is defined over.
    class sim_sparring_ai : public AggressorAI
    {
    public:
        explicit sim_sparring_ai(Creature* creature) : AggressorAI(creature) { }

        void EnterEvadeMode(EvadeReason /*why*/) override { }
    };

    class sim_sparring_dummy : public CreatureScript
    {
    public:
        sim_sparring_dummy() : CreatureScript("sim_sparring_dummy") { }

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new sim_sparring_ai(creature);
        }
    };

    // -- Iteration lifecycle -----------------------------------------------

    // Spawn, engage, reseed, zero the counters. Everything that must be true at
    // the first millisecond of a fight lives here, so iteration N starts from
    // the same place as iteration 0 rather than from wherever N-1 finished.
    // Everything that must be true at the first millisecond of a fight, applied
    // *after* the buff window rather than before it.
    //
    // The distinction is not pedantic. ResetActor tops the actor up and then the
    // bot spends the next 30 virtual seconds casting buffs, so by the time the
    // fight starts its mana is no longer full -- and not equally not-full each
    // time. Measured at iteration start: 14143 mana, then 13558, against a
    // 15388 maximum. A caster that begins one fight with 8% less mana than
    // another is not running the same fight, and at 300 seconds, where this spec
    // is mana-limited enough to spend a quarter of its damage wanding, that
    // difference goes straight into the result.
    void TopUpForFight(Player* actor)
    {
        actor->SetHealth(actor->GetMaxHealth());
        actor->SetPower(POWER_MANA,   actor->GetMaxPower(POWER_MANA));
        actor->SetPower(POWER_ENERGY, actor->GetMaxPower(POWER_ENERGY));
        actor->SetPower(POWER_FOCUS,  actor->GetMaxPower(POWER_FOCUS));
        actor->SetPower(POWER_RAGE,        0);
        actor->SetPower(POWER_RUNIC_POWER, 0);
        actor->ClearComboPoints();

        if (actor->getClass() == CLASS_DEATH_KNIGHT)
            for (uint8 i = 0; i < MAX_RUNES; ++i)
                actor->SetRuneCooldown(i, 0);

        // Buffing costs cooldowns too, and a summon like Shadowfiend -- which
        // both deals damage and restores mana on a 5 minute timer -- is
        // available in the first fight and not the second unless this runs here.
        actor->RemoveAllSpellCooldown();

        // Dying breaks equipment, and broken equipment gives no stats at all.
        // Against a lethal target this compounds: five deaths to Patchwerk left
        // 16 of 19 equipped items at zero durability, and because the actor is
        // saved to the database it stayed broken -- a later dummy run measured
        // 855 DPS where the same actor had measured 1191, with a spurious 13%
        // upward "drift" as the survivors degraded at different rates.
        //
        // Free, because a simulator has no economy and the alternative is
        // measuring a character that gets quietly worse the more it is used.
        actor->DurabilityRepairAll(false, 0.0f, false);

        if (Pet* pet = actor->GetPet())
        {
            pet->SetHealth(pet->GetMaxHealth());
            pet->SetPower(POWER_FOCUS, pet->GetMaxPower(POWER_FOCUS));
        }
    }

    bool StartIteration(Player* actor)
    {
        // Alive at the first millisecond, checked here rather than trusted from
        // PrepareIteration.
        //
        // The running outcome test is `!actor->IsAlive()`, which reads the death
        // *state*, not health. PrepareIteration resurrects and ResetActor sets
        // full health, but those are a whole buff window earlier; if the state
        // is anything but Alive when the clock starts, the very first tick
        // scores the fight as a loss.
        //
        // Measured on an arcane mage whose previous iteration ended in a real
        // death: "iteration 3/3 = 0.0 DPS (0 damage, 0 events) -- actor died
        // after 0.0s, took 0 damage, actor 100% / target 100% hp". A fight that
        // has run for zero milliseconds and dealt and taken nothing cannot have
        // been lost, and that phantom loss took the spec's solo-clear from 2/3
        // to 1/3 -- a 33% clear rate that was really 67%.
        //
        // Loud, because a resurrect needed here means PrepareIteration's did not
        // take, and that is worth knowing rather than papering over.
        if (!actor->IsAlive())
        {
            LOG_WARN("server.worldserver",
                "Simulator: actor was still not alive at the start of iteration {} "
                "-- resurrecting. The previous iteration's cleanup did not take.",
                s_iteration + 1);
            actor->ResurrectPlayer(1.0f);
            actor->SpawnCorpseBones();
        }

        TopUpForFight(actor);

        if (!SpawnTargetAndPosition(actor))
            return false;

        // Freeze the rest of the world before seeding. Other continents never
        // stop fighting, and every roll they make comes out of the same
        // thread-local RNG stream as ours -- so without this we both pay for
        // their combat in wall time and lose any hope of a reproducible run.
        sMapMgr->SetSimArenaMapId(actor->GetMapId());

        // Seed HERE, not at startup. One reseed covers every combat roll --
        // irand/urand/frand/rand_norm and the roll_chance_* family all route
        // through the same thread-local engine -- but only if it happens at a
        // reproducible point. Seeding at OnStartup left ~2 minutes of world
        // loading and an *asynchronous* bot login drawing from the stream
        // first, so combat began at a different offset every run.
        SetRandomSeed(s_cfg.seed + s_iteration);

        s_damage           = 0;
        s_actorDamageEvents = 0;
        s_damageTaken      = 0;
        s_incoming.clear();
        s_latch.Clear();
        s_log.Clear();
        s_dealType.valid   = false;
        s_absorbedOnActor  = 0;
        s_resistedOnActor  = 0;
        s_events.clear();
        s_eventsDropped    = 0;
        s_deaths.clear();
        s_iterAbilities.clear();
        s_healing          = 0;
        s_overheal         = 0;
        s_actorAuras.clear();
        s_targetAuras.clear();
        s_auraStacks.clear();
        s_auraSamples      = 0;
        s_aurasFiltered    = 0;
        s_auraStacksDropped = 0;
        s_shields.clear();
        s_resources.clear();
        s_auraSampleDue    = 0;
        s_resSampleDue     = 0;

        // Reserve rather than grow: ~15 events a second is the observed rate, so
        // this is one allocation instead of a doubling cascade inside the hooks.
        s_events.reserve(std::min<uint32>(EVENT_CAP, s_cfg.seconds * 32 + 64));
        s_outcome          = OUTCOME_TIMEOUT;
        s_elapsedMs        = 0;
        s_state            = SIM_RUNNING;

        // Aura count is the cheap, class-agnostic proxy for "did the buff window
        // work". If it differs between iterations, they are not comparable and
        // the drift check in tools/sim.py has something to bite on.
        LOG_INFO("server.worldserver",
            "Simulator: iteration {}/{} starting (seed {}), {}s, {} aura(s) up, {} mana.",
            s_iteration + 1, s_cfg.iterations, s_cfg.seed + s_iteration, s_cfg.seconds,
            uint32(actor->GetAppliedAuras().size()), actor->GetPower(POWER_MANA));

        // Taken after the reset and the buff window, so it is the pet that
        // actually fights this iteration. Logged on its own line rather than
        // appended to the one above because the absence of a pet is itself
        // worth being able to see at a glance.
        s_petAtStart     = SnapshotPet(actor);
        s_defenseAtStart = SnapshotDefense(actor);
        s_mitigation     = MitigationStat();

        if (s_petAtStart.entry)
            LOG_INFO("server.worldserver",
                "Simulator: pet entry={} level={} ap={} hp={} auras={} ({} passive).",
                s_petAtStart.entry, s_petAtStart.level, s_petAtStart.attackPower,
                s_petAtStart.maxHealth, s_petAtStart.auras, s_petAtStart.passives);

        return true;
    }

    // Returns true when every iteration is done.
    bool EndIteration()
    {
        float const seconds = float(s_elapsedMs) / 1000.0f;

        IterationResult r;
        r.index       = s_iteration;
        r.seed        = s_cfg.seed + s_iteration;
        r.damage      = s_damage;
        r.durationMs  = s_elapsedMs;
        r.actorEvents = s_actorDamageEvents;
        r.outcome     = s_outcome;
        r.damageTaken = s_damageTaken;
        r.incoming    = s_incoming;
        r.events      = std::move(s_events);
        r.eventsDropped = s_eventsDropped;
        r.deaths      = s_deaths;
        r.abilities   = s_iterAbilities;
        r.healing     = s_healing;
        r.overheal    = s_overheal;
        r.absorbedOnActor = s_absorbedOnActor;
        r.resistedOnActor = s_resistedOnActor;
        r.actorUptime = s_actorAuras;
        r.targetUptime = s_targetAuras;
        r.auraStacks  = s_auraStacks;
        r.shields     = s_shields;
        r.resources   = s_resources;
        r.auraSamples = s_auraSamples;
        r.aurasFiltered = s_aurasFiltered;
        r.auraStacksDropped = s_auraStacksDropped;
        r.pet         = s_petAtStart;
        r.mitigation  = s_mitigation;
        r.defense     = s_defenseAtStart;

        if (Player* actor = ObjectAccessor::FindConnectedPlayer(s_actorGuid))
        {
            r.actorHpPct = actor->GetMaxHealth()
                ? uint32(100 * actor->GetHealth() / actor->GetMaxHealth()) : 0;

            // GetAppliedAuras, not GetOwnedAuras. Unit::GetProcAurasTriggeredOnEvent
            // iterates the *applied* map, so an aura that is owned but not
            // applied is invisible to the proc system while still looking
            // present in any dump that reads ownership -- which is exactly the
            // blind spot that made Improved Scorch inexplicable.
            for (auto const& [spellId, aurApp] : actor->GetAppliedAuras())
                if (aurApp && aurApp->GetBase())
                    r.actorAuras.emplace_back(spellId, aurApp->GetBase()->GetStackAmount());

            if (Unit* target = ObjectAccessor::GetUnit(*actor, s_targetGuid))
            {
                r.targetHpPct = target->GetMaxHealth()
                    ? uint32(100 * target->GetHealth() / target->GetMaxHealth()) : 0;

                // Everything, not only what this actor applied: a debuff that
                // landed from the wrong caster, or a self-buff on the target, is
                // exactly the sort of thing worth seeing rather than filtering
                // out on an assumption.
                for (auto const& [spellId, aura] : target->GetOwnedAuras())
                    if (aura)
                        r.targetAuras.emplace_back(spellId, aura->GetStackAmount());
            }
        }

        s_results.push_back(std::move(r));

        static char const* const outcomeNames[] = { "timeout", "actor died", "target died" };

        LOG_INFO("server.worldserver",
            "Simulator: iteration {}/{} = {:.1f} DPS ({} damage, {} events) -- {} "
            "after {:.1f}s, took {} damage, actor {}% / target {}% hp.",
            s_iteration + 1, s_cfg.iterations,
            seconds > 0.0f ? float(s_damage) / seconds : 0.0f,
            s_damage, s_actorDamageEvents,
            outcomeNames[s_outcome < 3 ? s_outcome : 0], seconds,
            s_damageTaken, s_results.back().actorHpPct, s_results.back().targetHpPct);

        ++s_iteration;
        return s_iteration >= s_cfg.iterations;
    }

    // Put the actor back to a canonical pre-fight state and hand it to its
    // non-combat engine so it can re-buff.
    //
    // This runs before *every* iteration including the first. Previously the
    // first was measured with whatever buffs the bot applied while logging in
    // and travelling, ResetActor then stripped them, and iterations 2+ fought
    // without them -- a shadow priest missing Shadowform and Inner Fire is a
    // different character.
    //
    // On the evidence: this was prompted by a run that fell 1345.6 -> 1122.2 ->
    // 1021.9 DPS, read at the time as the actor degrading. Six iterations then
    // gave 1181.8 / 982.3 / 1223.2 / 1375.6 / 1086.5 / 1127.5 -- no trend, peak
    // in the middle. The "decline" was three noisy samples landing in order. So
    // this is kept because identical iterations are *correct*, not because the
    // effect was measurable; the aura count logged at each iteration start is
    // what makes the claim checkable rather than assumed.
    void PrepareIteration(Player* actor)
    {
        if (Creature* target = ObjectAccessor::GetCreature(*actor, s_targetGuid))
            target->DespawnOrUnsummon();

        s_targetGuid.Clear();

        // Against a real boss the actor may well be dead, and a corpse measures
        // nothing. Resurrect before the reset so the buff window has a live
        // player to buff.
        if (!actor->IsAlive())
        {
            actor->ResurrectPlayer(1.0f);
            actor->SpawnCorpseBones();
        }

        actor->AttackStop();
        actor->CombatStop(true);

        // Temporary guardians -- Shadowfiend, Water Elemental, Ghouls, Fire
        // Elemental -- would otherwise survive into the next fight, so
        // iteration N+1 starts with a damage source iteration N had to summon.
        // The permanent pet is spared: that one is part of the spec, not a
        // cooldown.
        //
        // "Spared" has to be spelled with GetGuardianPet, and the identity test
        // has to happen at all, because Pet derives from Guardian -> Minion ->
        // TempSummon: a permanent pet IS a TempSummon and ToTempSummon finds it
        // like any other. The old test used GetPet(), which returns null when
        // the pet guid is not of pet type (Player.cpp:9001), and in exactly
        // that case this loop despawned the hunter's own pet -- the opposite of
        // what the comment claimed.
        //
        // That is what made the pet-scaling bug below bimodal rather than
        // constant. A despawned pet gets re-called during the buff window and
        // comes back with its passives freshly applied; a surviving one reached
        // ResetActor and had them stripped. Same run, same spec, two states,
        // depending on nothing the spec controls.
        //
        // Collected first: UnSummon mutates m_Controlled, so despawning while
        // iterating it invalidates the iterator.
        Guardian* permanentPet = actor->GetGuardianPet();

        std::vector<TempSummon*> guardians;
        for (Unit* controlled : actor->m_Controlled)
            if (controlled && controlled != permanentPet)
                if (TempSummon* summon = controlled->ToTempSummon())
                    guardians.push_back(summon);

        for (TempSummon* summon : guardians)
            summon->UnSummon();

        ResetActor(actor);

#ifdef MOD_PLAYERBOTS
        // The non-combat engine is what runs the buff strategies. Without this
        // the bot stays in its combat engine with nothing to fight and never
        // re-applies anything.
        if (PlayerbotAI* botAI = sPlayerbotsMgr.GetPlayerbotAI(actor))
        {
            botAI->GetAiObjectContext()->GetValue<Unit*>("current target")->Set(nullptr);
            botAI->ChangeEngine(BOT_STATE_NON_COMBAT);
        }
#endif
    }

    // -- Hooks -------------------------------------------------------------

    class SimDamageCollector : public UnitScript
    {
    public:
        SimDamageCollector() : UnitScript("SimDamageCollector", true, {
            UNITHOOK_ON_DAMAGE,
            UNITHOOK_ON_HEAL,
            UNITHOOK_MODIFY_HEAL_RECEIVED,
            UNITHOOK_MODIFY_SPELL_DAMAGE_TAKEN,
            UNITHOOK_MODIFY_PERIODIC_DAMAGE_AURAS_TICK,
            UNITHOOK_MODIFY_MELEE_DAMAGE,
            UNITHOOK_ON_UNIT_DEATH }) { }

        // Not in the hook list above, and not an oversight: ScriptMgr::DealDamage
        // (UnitScript.cpp:52) walks every UnitScript with no enabled-hook gate,
        // so there is no enum to register. Must return the damage unchanged.
        uint32 DealDamage(Unit* attacker, Unit* victim, uint32 damage,
            DamageEffectType damagetype) override
        {
            if (Alonecraft::Sim::Active() && attacker && victim)
                s_dealType = { attacker->GetGUID(), victim->GetGUID(),
                               s_elapsedMs, uint8(damagetype), true };

            return damage;
        }

        void ModifySpellDamageTaken(Unit* target, Unit* attacker, int32& damage,
            SpellInfo const* spellInfo) override
        {
            Note(attacker, target, spellInfo ? spellInfo->Id : 0, DK_SPELL,
                 damage > 0 ? uint32(damage) : 0);
        }

        void ModifyPeriodicDamageAurasTick(Unit* target, Unit* attacker, uint32& damage,
            SpellInfo const* spellInfo) override
        {
            // SpellAuraEffects.cpp:6654 calls this hook on a periodic *heal*
            // tick, one line before ModifyHealReceived. Without this guard a
            // Renew tick latches as periodic damage under the heal's spell id.
            if (!spellInfo || !HasPeriodicDamage(spellInfo))
                return;

            Note(attacker, target, spellInfo->Id, DK_PERIODIC, damage);
        }

        void ModifyMeleeDamage(Unit* target, Unit* attacker, uint32& damage) override
        {
            Note(attacker, target, 0, DK_MELEE, damage);
        }

        void OnDamage(Unit* attacker, Unit* victim, uint32& damage) override
        {
            if (!Alonecraft::Sim::Active() || !attacker || !victim)
                return;

            Alonecraft::Sim::RecordDamage(attacker, victim, damage);
        }

        // Healing needs both hooks, because neither is sufficient alone.
        //
        // ModifyHealReceived (Unit.cpp:8391, HoT ticks at
        // SpellAuraEffects.cpp:6655) carries the SpellInfo and the amount before
        // overheal is clipped. OnHeal (Unit.cpp:8108) carries the *effective*
        // gain -- health actually restored -- but no SpellInfo. Pairing them is
        // what makes overhealing visible, and overhealing is the number that
        // says whether a healer's sustain is real or wasted.
        void ModifyHealReceived(Unit* target, Unit* healer, uint32& heal,
            SpellInfo const* spellInfo) override
        {
            if (!Alonecraft::Sim::Active() || !healer || !target)
                return;

            // Only healing by the actor or its pet can be consumed.
            if (healer->GetGUID() != s_actorGuid &&
                healer->GetCharmerOrOwnerGUID() != s_actorGuid)
                return;

            s_latch.Push(healer->GetGUID(), target->GetGUID(),
                         spellInfo ? spellInfo->Id : 0, DK_SPELL, EK_HEAL, heal);
        }

        void OnHeal(Unit* healer, Unit* receiver, uint32& gain) override
        {
            // Unit.cpp:8094 guards on `healer` being non-null, so it can be.
            if (!Alonecraft::Sim::Active() || !healer || !receiver)
                return;

            Alonecraft::Sim::RecordHeal(healer, receiver, gain);
        }

        void OnUnitDeath(Unit* unit, Unit* /*killer*/) override
        {
            if (!Alonecraft::Sim::Active() || !unit)
                return;

            Alonecraft::Sim::RecordDeath(unit);
        }

    private:
        static bool HasPeriodicDamage(SpellInfo const* spellInfo)
        {
            for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
                switch (spellInfo->Effects[i].ApplyAuraName)
                {
                    case SPELL_AURA_PERIODIC_DAMAGE:
                    case SPELL_AURA_PERIODIC_DAMAGE_PERCENT:
                    case SPELL_AURA_PERIODIC_LEECH:
                        return true;
                    default:
                        break;
                }

            return false;
        }

        static void Note(Unit* attacker, Unit* target, uint32 spellId, uint8 kind,
            uint32 raw)
        {
            if (!Alonecraft::Sim::Active() || !attacker || !target)
                return;

            // Only latch what could ever be consumed. This hook fires for every
            // damage event on the map, and the boss's swings at us outnumbered
            // our own casts -- pushing those filled the ring and evicted the
            // latches we actually needed, which showed up as 711 overflows and
            // *more* unattributed damage, not less.
            ObjectGuid const attackerGuid = attacker->GetGUID();
            ObjectGuid const victimGuid   = target->GetGUID();

            bool const fromActor = attackerGuid == s_actorGuid ||
                                   attacker->GetCharmerOrOwnerGUID() == s_actorGuid;

            if (!(fromActor && victimGuid == s_targetGuid))
                return;

            // Counted here rather than on the landing event, because that is the
            // point of it: this hook fires 17 lines before
            // RollMeleeOutcomeAgainst (Unit.cpp:1798 vs 1815), so for melee
            // `attempts - count` is the miss, dodge and parry count, free.
            bool const fromPet = attackerGuid != s_actorGuid;
            s_abilities[{ spellId, fromPet }].attempts     += 1;
            s_iterAbilities[{ spellId, fromPet }].attempts += 1;

            s_latch.Push(attackerGuid, victimGuid, spellId, kind, EK_DAMAGE, raw);
        }
    };

    // Decodes the four combat-log packets. Layouts are read straight off their
    // builders in Unit.cpp, cited per case; if one of those changes upstream
    // this silently stops matching, which is why s_logUnmatched is reported.
    class SimCombatLog : public PlayerbotScript
    {
    public:
        SimCombatLog() : PlayerbotScript("SimCombatLog") { }

        void OnPlayerbotPacketSent(Player* player, WorldPacket const* packet) override
        {
            if (!Alonecraft::Sim::Active() || !player || !packet ||
                player->GetGUID() != s_actorGuid)
                return;

            // A copy, because the packet is const and still owned by the caller.
            // Sixty-odd bytes; the alternative is positional reads that would
            // have to re-derive every pack-GUID length by hand.
            WorldPacket data(*packet);

            try
            {
                switch (packet->GetOpcode())
                {
                    case SMSG_ATTACKERSTATEUPDATE:      ReadMelee(data);    break;
                    case SMSG_SPELLNONMELEEDAMAGELOG:   ReadSpell(data);    break;
                    case SMSG_PERIODICAURALOG:          ReadPeriodic(data); break;
                    case SMSG_SPELLHEALLOG:             ReadHeal(data);     break;
                    case SMSG_SPELLDAMAGESHIELD:        ReadShield(data);   break;
                    default: break;
                }
            }
            catch (ByteBufferException const&)
            {
                // A layout that no longer matches must not take the run down;
                // it shows up as unmatched events instead.
                ++s_logUnmatched;
            }
        }

    private:
        // Unit::SendAttackStateUpdate, Unit.cpp:6941-6971.
        static void ReadMelee(WorldPacket& data)
        {
            uint32 hitInfo;
            ObjectGuid attacker, victim;
            uint32 fullDamage, overkill;
            uint8  count;

            data >> hitInfo;
            data >> attacker.ReadAsPacked();
            data >> victim.ReadAsPacked();
            data >> fullDamage >> overkill >> count;

            for (uint8 i = 0; i < count; ++i)
            {
                uint32 school, sub;
                float  subFloat;
                data >> school >> subFloat >> sub;
            }

            uint32 absorb = 0, resist = 0;

            if (hitInfo & (HITINFO_FULL_ABSORB | HITINFO_PARTIAL_ABSORB))
                for (uint8 i = 0; i < count; ++i)
                {
                    uint32 a;
                    data >> a;
                    absorb += a;
                }

            if (hitInfo & (HITINFO_FULL_RESIST | HITINFO_PARTIAL_RESIST))
                for (uint8 i = 0; i < count; ++i)
                {
                    uint32 r;
                    data >> r;
                    resist += r;
                }

            // The tail of the packet, which this used to stop short of. It is
            // where the whole of incoming avoidance lives: TargetState carries
            // dodge, parry, immune and deflect, and blocked_amount is the only
            // report of how much a block actually removed.
            //
            // Worth reading even though the damage hooks already give net
            // damage, because an avoided swing deals nothing and therefore
            // fires no damage hook at all -- it is invisible to every other
            // path in this file. "Took 0 damage" and "was never attacked" are
            // different facts and used to look identical.
            uint8  targetState = 0;
            uint32 blocked     = 0;

            data >> targetState;
            data.read_skip<uint32>();   // unknown attackerstate
            data.read_skip<uint32>();   // melee spellid

            if (hitInfo & HITINFO_BLOCK)
                data >> blocked;

            if (victim == s_actorGuid)
                RecordIncomingSwing(hitInfo, targetState, fullDamage, absorb,
                                    resist, blocked);

            LogRecord rec;
            rec.attacker = attacker;
            rec.victim   = victim;
            rec.spellId  = 0;
            rec.amount   = fullDamage;
            rec.absorb   = absorb;
            rec.resist   = resist;
            rec.ms       = s_elapsedMs;
            rec.kind     = DK_MELEE;
            rec.evKind   = EK_DAMAGE;
            rec.crit     = (hitInfo & HITINFO_CRITICALHIT) != 0;
            s_log.Push(rec);
        }

        // Unit::SendSpellNonMeleeDamageLog, Unit.cpp:6737-6749.
        static void ReadSpell(WorldPacket& data)
        {
            ObjectGuid victim, attacker;
            uint32 spellId, damage, overkill, absorb, resist, blocked, hitInfo;
            uint8  school, physicalLog, unused;

            data >> victim.ReadAsPacked();
            data >> attacker.ReadAsPacked();
            data >> spellId >> damage >> overkill >> school >> absorb >> resist
                 >> physicalLog >> unused >> blocked >> hitInfo;

            LogRecord rec;
            rec.attacker = attacker;
            rec.victim   = victim;
            rec.spellId  = spellId;
            rec.amount   = damage;
            rec.absorb   = absorb;
            rec.resist   = resist;
            rec.ms       = s_elapsedMs;
            rec.kind     = DK_SPELL;
            rec.evKind   = EK_DAMAGE;
            rec.crit     = (hitInfo & SPELL_HIT_TYPE_CRIT) != 0;
            s_log.Push(rec);
        }

        // Unit::SendPeriodicAuraLog, Unit.cpp:6830-6866. Note the first GUID is
        // the unit the aura sits on and the second is its caster, which is the
        // opposite order to the spell log.
        static void ReadPeriodic(WorldPacket& data)
        {
            ObjectGuid victim, caster;
            uint32 spellId, count, auraType;

            data >> victim.ReadAsPacked();
            data >> caster.ReadAsPacked();
            data >> spellId >> count >> auraType;

            LogRecord rec;
            rec.attacker = caster;
            rec.victim   = victim;
            rec.spellId  = spellId;
            rec.ms       = s_elapsedMs;
            rec.kind     = DK_PERIODIC;

            if (auraType == SPELL_AURA_PERIODIC_DAMAGE ||
                auraType == SPELL_AURA_PERIODIC_DAMAGE_PERCENT)
            {
                uint32 damage, overDamage, school, absorb, resist;
                uint8  critical;
                data >> damage >> overDamage >> school >> absorb >> resist >> critical;

                rec.amount = damage;
                rec.absorb = absorb;
                rec.resist = resist;
                rec.evKind = EK_DAMAGE;
                rec.crit   = critical != 0;
            }
            else if (auraType == SPELL_AURA_PERIODIC_HEAL ||
                     auraType == SPELL_AURA_OBS_MOD_HEALTH)
            {
                uint32 heal, overheal, absorb;
                uint8  critical;
                data >> heal >> overheal >> absorb >> critical;

                // The effective heal, to match what OnHeal reports.
                rec.amount = heal > overheal ? heal - overheal : 0;
                rec.absorb = absorb;
                rec.evKind = EK_HEAL;
                rec.crit   = critical != 0;
            }
            else
                return;   // energize and mana leech: no damage event follows

            s_log.Push(rec);
        }

        // Damage shields -- Thorns, Retribution Aura and kin -- built inline in
        // Unit::DealDamageShieldDamage at Unit.cpp:2207-2216.
        //
        // This is the one damage source no Modify* hook can name: the core deals
        // it at Unit.cpp:2218 without ever calling CalculateSpellDamageTaken. It
        // was the whole of the resto druid's unattributed damage, and it needs
        // its own case because it is the only combat-log packet using *full*
        // GUIDs rather than packed ones, and the only one with no crit field --
        // a damage shield cannot crit.
        static void ReadShield(WorldPacket& data)
        {
            ObjectGuid attacker, victim;
            uint32 spellId, damage, overkill, school;

            data >> attacker >> victim >> spellId >> damage >> overkill >> school;

            LogRecord rec;
            rec.attacker = attacker;
            rec.victim   = victim;
            rec.spellId  = spellId;
            rec.amount   = damage;
            rec.ms       = s_elapsedMs;
            rec.kind     = DK_SPELL;
            rec.evKind   = EK_DAMAGE;
            rec.crit     = false;
            s_log.Push(rec);
        }

        // Unit::SendHealSpellLog, Unit.cpp:8377-8384.
        static void ReadHeal(WorldPacket& data)
        {
            ObjectGuid victim, healer;
            uint32 spellId, heal, overheal, absorb;
            uint8  critical, unused;

            data >> victim.ReadAsPacked();
            data >> healer.ReadAsPacked();
            data >> spellId >> heal >> overheal >> absorb >> critical >> unused;

            LogRecord rec;
            rec.attacker = healer;
            rec.victim   = victim;
            rec.spellId  = spellId;
            rec.amount   = heal > overheal ? heal - overheal : 0;
            rec.absorb   = absorb;
            rec.ms       = s_elapsedMs;
            rec.kind     = DK_SPELL;
            rec.evKind   = EK_HEAL;
            rec.crit     = critical != 0;
            s_log.Push(rec);
        }
    };

    class SimController : public WorldScript
    {
    public:
        SimController() : WorldScript("SimController") { }

        void OnStartup() override
        {
            ParseArgs();

            if (!s_cfg.active)
                return;

            // Read once and cached in statics, so no hook on a per-event path
            // ever touches sConfigMgr. Both are kill switches rather than tuning
            // dials -- 0 disables the sampler entirely.
            s_auraSampleMs = uint32(std::max(0,
                sConfigMgr->GetOption<int32>("Alonecraft.Sim.AuraSampleMs", 100)));
            s_resSampleMs  = uint32(std::max(0,
                sConfigMgr->GetOption<int32>("Alonecraft.Sim.ResourceSampleMs", 1000)));

            LOG_INFO("server.worldserver",
                "Simulator: character='{}' spec='{}' level={} target={} seconds={} seed={}",
                s_cfg.character, s_cfg.spec, s_cfg.level, s_cfg.targetEntry,
                s_cfg.seconds, s_cfg.seed);

            s_wallStart = std::chrono::steady_clock::now();
            s_state     = SIM_LOGIN_REQUESTED;
        }

        void OnUpdate(uint32 diff) override
        {
            if (!s_cfg.active || s_state == SIM_DONE)
                return;

            switch (s_state)
            {
                case SIM_LOGIN_REQUESTED:
                    if (RequestLogin())
                        s_state = SIM_AWAIT_LOGIN;
                    break;

                case SIM_AWAIT_LOGIN:
                {
                    s_waitedMs += diff;

                    Player* actor = ObjectAccessor::FindConnectedPlayer(s_actorGuid);
                    if (actor && actor->IsInWorld())
                    {
                        // Before the teleport, so the heavy DB work happens
                        // while the actor is still on its login map and cannot
                        // be caught mid-transfer.
                        if (!ConfigureActor(actor))
                            return;

                        if (!SendToArena(actor))
                        {
                            Fail("could not teleport actor to the arena");
                            return;
                        }

                        s_waitedMs = 0;
                        s_state    = SIM_AWAIT_ARENA;
                        LOG_INFO("server.worldserver",
                            "Simulator: '{}' logged in, moving to arena (map {}).",
                            s_cfg.character, s_cfg.mapId);
                        break;
                    }

                    // Virtual milliseconds, so this is a bounded number of ticks
                    // rather than a wall-clock timeout.
                    if (s_waitedMs > 120 * IN_MILLISECONDS)
                        Fail("bot did not log in");
                    break;
                }

                case SIM_AWAIT_ARENA:
                {
                    s_waitedMs += diff;

                    Player* actor = ObjectAccessor::FindConnectedPlayer(s_actorGuid);
                    if (!actor)
                    {
                        Fail("actor vanished during teleport");
                        return;
                    }

                    if (InArena(actor))
                    {
                        // Same path as between iterations, so iteration 0 is
                        // prepared and buffed exactly like every other one.
                        PrepareIteration(actor);
                        s_waitedMs = 0;
                        s_state    = SIM_BETWEEN;
                        break;
                    }

                    if (s_waitedMs > 120 * IN_MILLISECONDS)
                        Fail("actor never arrived at the arena");
                    break;
                }

                case SIM_RUNNING:
                {
                    ++s_ticks;
                    s_elapsedMs += diff;

                    // Combat lapses after a few idle seconds and takes the bot's
                    // combat engine with it, so re-assert it once a virtual second.
                    if ((s_ticks % 40) == 0)
                        MaintainEngagement();

                    // A fight ends when someone dies, not only when the clock
                    // runs out. Against the inert dummy neither happens and this
                    // is just the timeout; against a real boss it is the
                    // measurement.
                    uint8 outcome = OUTCOME_TIMEOUT;
                    {
                        Player* a = ObjectAccessor::FindConnectedPlayer(s_actorGuid);
                        Unit*   t = a ? ObjectAccessor::GetUnit(*a, s_targetGuid) : nullptr;

                        if (!a || !a->IsAlive())
                            outcome = OUTCOME_ACTOR_DIED;
                        else if (!t || !t->IsAlive())
                            outcome = OUTCOME_TARGET_DIED;

                        // Sampled here to reuse the two ObjectAccessor lookups
                        // this block already paid for.
                        //
                        // Driven by an accumulator against diff, not by
                        // (s_ticks % N): Alonecraft.Sim.TickMs is configurable,
                        // so a tick count is not a time, and a modulo would
                        // silently change the sample rate when someone changes
                        // the tick.
                        if (a && a->IsAlive())
                        {
                            s_auraSampleDue += diff;
                            if (s_auraSampleMs && s_auraSampleDue >= s_auraSampleMs)
                            {
                                s_auraSampleDue = 0;
                                SampleAuras(a, t);
                            }

                            s_resSampleDue += diff;
                            if (s_resSampleMs && s_resSampleDue >= s_resSampleMs)
                            {
                                s_resSampleDue = 0;
                                SampleResources(a, t);
                            }
                        }
                    }

                    if (outcome != OUTCOME_TIMEOUT ||
                        s_elapsedMs >= s_cfg.seconds * IN_MILLISECONDS)
                    {
                        s_outcome = outcome;
                        bool const finished = EndIteration();

                        Player* actor = ObjectAccessor::FindConnectedPlayer(s_actorGuid);
                        if (actor && !finished)
                            PrepareIteration(actor);

                        if (finished || !actor)
                        {
                            sMapMgr->SetSimArenaMapId(0);
                            WriteResult(true, actor ? "ok" : "actor vanished mid-run");
                            s_state = SIM_DONE;
                            World::StopNow(SHUTDOWN_EXIT_CODE);
                            break;
                        }

                        s_waitedMs = 0;
                        s_state    = SIM_BETWEEN;
                    }
                    break;
                }

                case SIM_BETWEEN:
                {
                    // The buff window. The actor has just been stripped to a
                    // canonical state, so this is the time its non-combat engine
                    // gets to put Shadowform, Inner Fire, aspects, stances,
                    // armours and so on back on. Generous by design: at ~80x
                    // realtime, 30 virtual seconds costs under half a second of
                    // wall clock, and buying identical iterations for that is
                    // the cheapest trade in the simulator.
                    s_waitedMs += diff;
                    if (s_waitedMs < s_cfg.buffSeconds * IN_MILLISECONDS)
                        break;

                    Player* actor = ObjectAccessor::FindConnectedPlayer(s_actorGuid);
                    if (!actor)
                    {
                        Fail("actor vanished between iterations");
                        return;
                    }

                    if (!StartIteration(actor))
                        return;
                    break;
                }

                default:
                    break;
            }
        }
    };
}

namespace Alonecraft::Sim
{
    bool Active()
    {
        return s_cfg.active && s_state == SIM_RUNNING;
    }

    void RecordDamage(Unit* attacker, Unit* victim, uint32 amount)
    {
        // Only damage the actor (or anything it owns) puts into the target.
        // Pet damage counts -- dropping it would be catastrophically wrong for
        // BM hunters, demonology and unholy DKs, where the pet is a first-class
        // Alonecraft tuning surface.
        ++s_anyDamageEvents;

        bool const fromActor = attacker->GetGUID() == s_actorGuid ||
                               attacker->GetCharmerOrOwnerGUID() == s_actorGuid;

        if (fromActor)
            ++s_actorDamageEvents;
        else if (attacker->GetGUID() != s_targetGuid)
        {
            // Neither ours nor the dummy hitting us: this is the contamination.
            ++s_foreignDamageByEntry[attacker->GetEntry()];
            ++s_foreignDamageByMap[attacker->GetMapId()];
        }

        // What the core said this event was. Same call stack as this hook, so it
        // is about this event and nothing else; stale otherwise.
        uint8 const dealType =
            (s_dealType.valid && s_dealType.ms == s_elapsedMs &&
             s_dealType.attacker == attacker->GetGUID() &&
             s_dealType.victim   == victim->GetGUID())
            ? s_dealType.type : uint8(NODAMAGE);

        // Damage *taken* by the actor, timestamped. TTD and TMI are built from
        // this; TMI in particular is a soft maximum over trailing 6-second
        // windows, so the series is required and a total will not do.
        if (victim->GetGUID() == s_actorGuid)
        {
            s_damageTaken += amount;
            s_incoming.emplace_back(s_elapsedMs, amount);

            // Incoming absorb, exactly, from the log. This is what makes a
            // shielded spec legible: OnDamage fires after CalcAbsorbResist, so
            // without it a mage behind Ice Barrier looks unattacked.
            uint16 flags = EF_INCOMING;
            uint32 spellId = 0, absorb = 0, resist = 0;

            if (LogRecord const* rec = s_log.Take(attacker->GetGUID(),
                    victim->GetGUID(), EK_DAMAGE, amount))
            {
                spellId = rec->spellId;
                absorb  = rec->absorb;
                resist  = rec->resist;
                flags  |= EF_LOGGED;

                if (rec->crit)
                    flags |= EF_CRIT;

                s_absorbedOnActor += absorb;
                s_resistedOnActor += resist;
            }

            PushEvent({ s_elapsedMs, spellId, amount, absorb, resist, 0,
                        flags, EK_DAMAGE, dealType });
        }

        // Deliberately does NOT clear the latch. This event is not ours, but the
        // one that pushed the latch may well be: split damage recurses into
        // DealDamage, so a nested event for a third unit arrives *first*, and
        // dropping the latch here misfiled the parent as unattributed.
        if (victim->GetGUID() != s_targetGuid || !fromActor)
            return;

        s_damage += amount;

        // Consume the latch only if it describes *this* event. A mismatch means
        // something interleaved, and guessing would misfile the damage.
        uint32 spellId = 0;
        uint8  kind    = DK_UNKNOWN;
        uint32 raw     = 0;
        uint32 absorb  = 0;
        uint32 resist  = 0;
        uint16 flags   = 0;
        bool   crit    = false;

        // The combat log first: it carries the spell id, the crit and the exact
        // absorb, for every category including the damage-shield path that no
        // Modify* hook ever sees. The latch is the fallback, and the two
        // disagreeing is itself worth knowing.
        LogRecord const* rec = s_log.Take(attacker->GetGUID(), victim->GetGUID(),
                                          EK_DAMAGE, amount);
        if (rec)
        {
            ++s_logMatched;
            spellId = rec->spellId;
            kind    = rec->kind;
            absorb  = rec->absorb;
            resist  = rec->resist;
            crit    = rec->crit;
            flags  |= EF_LOGGED;

            if (crit)
                flags |= EF_CRIT;

            // Consume any latch for the same event so it cannot be claimed by a
            // later one; its value is not needed now.
            uint32 lspell = 0, lraw = 0;
            uint8  lkind  = DK_UNKNOWN;
            s_latch.Take(attacker->GetGUID(), victim->GetGUID(), EK_DAMAGE,
                         lspell, lkind, lraw);
            raw = lraw;
        }
        else
        {
            ++s_logUnmatched;

            if (!s_latch.Take(attacker->GetGUID(), victim->GetGUID(), EK_DAMAGE,
                              spellId, kind, raw))
            {
                s_unattributedDamage += amount;
                s_unattributedByType[dealType < 6 ? dealType : NODAMAGE] += amount;
                flags |= EF_UNATTRIBUTED;

                // Say *why* it missed. "No hook fired for this event" and "the
                // hook fired naming a different attacker" are different bugs
                // with the same symptom.
                ++(s_latch.SawThisTick(victim->GetGUID(), EK_DAMAGE)
                       ? s_latchMissAttacker
                       : s_latchMissNoLatch);
            }
        }

        bool const fromPet = attacker->GetGUID() != s_actorGuid;
        if (fromPet)
            flags |= EF_FROM_PET;

        PushEvent({ s_elapsedMs, spellId, amount, absorb, resist, raw,
                    flags, EK_DAMAGE, dealType });

        for (AbilityStat* stat : { &s_abilities[{ spellId, fromPet }],
                                   &s_iterAbilities[{ spellId, fromPet }] })
        {
            stat->AddHit(amount, crit);
            stat->kind    = kind;
            stat->fromPet = fromPet;

            if (rec)
                ++stat->logged;
        }
    }

    void RecordHeal(Unit* healer, Unit* receiver, uint32 gain)
    {
        bool const fromActor = healer->GetGUID() == s_actorGuid ||
                               healer->GetCharmerOrOwnerGUID() == s_actorGuid;

        if (!fromActor)
            return;

        uint32 spellId = 0;
        uint8  kind    = DK_UNKNOWN;
        uint32 raw     = 0;
        uint16 flags   = 0;
        bool   crit    = false;

        // The heal log carries the crit byte and the spell id; the latch carries
        // the pre-overheal amount, which the log gives as `overheal` but which
        // the latch already has. Take both.
        if (LogRecord const* rec = s_log.Take(healer->GetGUID(),
                receiver->GetGUID(), EK_HEAL, gain))
        {
            spellId = rec->spellId;
            kind    = rec->kind;
            crit    = rec->crit;
            flags  |= EF_LOGGED;

            if (crit)
                flags |= EF_CRIT;
        }

        uint32 lspell = 0, lraw = 0;
        uint8  lkind  = DK_UNKNOWN;

        if (s_latch.Take(healer->GetGUID(), receiver->GetGUID(), EK_HEAL,
                         lspell, lkind, lraw))
        {
            raw = lraw;

            if (!spellId)
            {
                spellId = lspell;
                kind    = lkind;
            }
        }
        else if (!(flags & EF_LOGGED))
            ++s_unattributedHeals;

        if (receiver->GetGUID() == healer->GetGUID())
            flags |= EF_SELF;

        if (healer->GetGUID() != s_actorGuid)
            flags |= EF_FROM_PET;

        // raw is the amount before overheal was clipped, so the difference is
        // the overheal. It is only meaningful when the latch actually matched;
        // an unmatched heal reports zero overheal rather than a fabricated one.
        uint32 const overheal = raw > gain ? raw - gain : 0;

        s_healing  += gain;
        s_overheal += overheal;

        PushEvent({ s_elapsedMs, spellId, gain, 0, 0, raw, flags, EK_HEAL,
                    uint8(HEAL) });

        bool const fromPet = healer->GetGUID() != s_actorGuid;

        for (AbilityStat* stat : { &s_abilities[{ spellId, fromPet }],
                                   &s_iterAbilities[{ spellId, fromPet }] })
        {
            stat->healing   += gain;
            stat->healCount += 1;
            stat->overheal  += overheal;
            stat->fromPet    = fromPet;

            if (crit)
                stat->healCrits += 1;
        }
    }

    void RecordDeath(Unit* unit)
    {
        ObjectGuid const guid = unit->GetGUID();

        if (guid == s_actorGuid)
            s_deaths.emplace_back(s_elapsedMs, DR_ACTOR);
        else if (guid == s_targetGuid)
            s_deaths.emplace_back(s_elapsedMs, DR_TARGET);
        else if (unit->GetCharmerOrOwnerGUID() == s_actorGuid)
            s_deaths.emplace_back(s_elapsedMs, DR_PET);
    }
}

void AddSC_alonecraft_sim()
{
    new sim_target_dummy();
    new sim_sparring_dummy();
    new SimDamageCollector();
    new SimCombatLog();
    new SimController();
}
