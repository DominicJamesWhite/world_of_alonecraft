/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license:
 * https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE-AGPL3
 *
 * Scaled encounter overrides for Naxxramas (1–5 players):
 *
 * Construct Quarter:
 *   - Gluth                 (zombie chow spawn scaling, Decimate damage)
 *   - Thaddius              (polarity shift damage scaling, Magnetic Pull block)
 *
 * Military Quarter:
 *   - Instructor Razuvious  (understudy auto-tank at low N)
 *   - Gothik the Harvester  (gate open on dead add spawn at low N)
 *   - Four Horsemen         (mark stack cap at low N)
 */

#include "ScaledEncounters.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "Spell.h"
#include "SpellAuras.h"
#include "SpellInfo.h"

// ===========================  Spell / NPC IDs  =============================

enum NaxxSpells
{
    // Thaddius
    SPELL_POSITIVE_CHARGE       = 28062,
    SPELL_NEGATIVE_CHARGE       = 28085,
    SPELL_MAGNETIC_PULL         = 28337,
    // Gluth
    SPELL_DECIMATE_DAMAGE       = 28375,
    // Four Horsemen marks
    SPELL_MARK_OF_KORTHAZZ     = 28832,
    SPELL_MARK_OF_BLAUMEUX     = 28833,
    SPELL_MARK_OF_RIVENDARE    = 28834,
    SPELL_MARK_OF_ZELIEK       = 28835,
};

enum NaxxNPCs
{
    NPC_ZOMBIE_CHOW             = 16360,
    NPC_GLUTH                   = 15932,
    // Military Quarter
    NPC_RAZUVIOUS               = 16061,
    NPC_DEATH_KNIGHT_UNDERSTUDY = 16803,
    // Gothik dead-side adds
    NPC_DEAD_TRAINEE            = 16127,
    NPC_DEAD_KNIGHT             = 16148,
    NPC_DEAD_HORSE              = 16149,
    NPC_DEAD_RIDER              = 16150,
};

// ===========================  Thaddius  ====================================

// ---------------------------------------------------------------------------
//  #13 — Thaddius: Polarity Shift damage scaling
//
//  The polarity charge spells deal damage to nearby players with the
//  opposite polarity.  With fewer than 5 players the mechanic is
//  disproportionately punishing because there's almost no way to
//  spread out.  Scale damage: damage * (N-1) / 4.
//    N=1 → 0%   N=2 → 25%   N=3 → 50%   N=4 → 75%   N=5 → 100%
// ---------------------------------------------------------------------------

class ScaledEncounters_ThaddiusPolarity : public UnitScript
{
public:
    ScaledEncounters_ThaddiusPolarity()
        : UnitScript("ScaledEncounters_ThaddiusPolarity",
                      /*addToScripts=*/true,
                      { UNITHOOK_MODIFY_SPELL_DAMAGE_TAKEN })
    { }

    void ModifySpellDamageTaken(Unit* target, Unit* /*attacker*/, int32& damage, SpellInfo const* spellInfo) override
    {
        if (!spellInfo || !target)
            return;

        if (spellInfo->Id != SPELL_POSITIVE_CHARGE && spellInfo->Id != SPELL_NEGATIVE_CHARGE)
            return;

        uint32 N = GetGroupSize(target);
        if (N >= 5)
            return;

        // damage * (N-1) / 4  — at N=1 this zeroes out the damage entirely
        damage = damage * static_cast<int32>(N - 1) / 4;
    }
};

// ---------------------------------------------------------------------------
//  Stalagg & Feugen: Magnetic Pull blocker at low N
//
//  Magnetic Pull swaps tanks between the two platforms.  With fewer
//  than 3 players this causes minions to chase the solo player off
//  their platform, triggering the tesla overload (massive AoE shock).
//  Block the spell entirely at low N so the minions stay put.
// ---------------------------------------------------------------------------

class ScaledEncounters_ThaddMagneticPull : public AllSpellScript
{
public:
    ScaledEncounters_ThaddMagneticPull()
        : AllSpellScript("ScaledEncounters_ThaddMagneticPull",
                          { ALLSPELLHOOK_ON_SPELL_CHECK_CAST })
    { }

    void OnSpellCheckCast(Spell* spell, bool /*strict*/, SpellCastResult& res) override
    {
        if (!spell || !spell->GetSpellInfo())
            return;

        if (spell->GetSpellInfo()->Id != SPELL_MAGNETIC_PULL)
            return;

        Unit* caster = spell->GetCaster();
        if (!caster)
            return;

        uint32 N = GetGroupSize(caster);
        if (N < 3)
            res = SPELL_FAILED_DONT_REPORT;
    }
};

// ===========================  Gluth  =======================================

// ---------------------------------------------------------------------------
//  Gluth: Zombie Chow spawn scaling
//
//  Zombie chow attack Gluth, and when he kills them he heals 5% max HP.
//  A solo player can never out-DPS this healing.  Scale by thinning
//  the zombie population on spawn:
//    N=1 → no chow at all     N=2 → 50% despawned
//    N=3 → 30% despawned      N=4+ → normal
// ---------------------------------------------------------------------------

class ScaledEncounters_GluthZombies : public AllCreatureScript
{
public:
    ScaledEncounters_GluthZombies()
        : AllCreatureScript("ScaledEncounters_GluthZombies")
    { }

    void OnCreatureAddWorld(Creature* creature) override
    {
        if (!creature || creature->GetEntry() != NPC_ZOMBIE_CHOW)
            return;

        Map* map = creature->GetMap();
        if (!map || !map->IsDungeon())
            return;

        uint32 N = GetGroupSize(map);

        if (N <= 1)
        {
            // No zombie chow at all — despawn immediately.
            creature->DespawnOrUnsummon(Milliseconds(1));
            return;
        }

        // At N=2 despawn 50% of spawns, at N=3 despawn ~30%.
        uint32 despawnChancePct = 0;
        if (N == 2)
            despawnChancePct = 50;
        else if (N == 3)
            despawnChancePct = 30;

        if (despawnChancePct && urand(1, 100) <= despawnChancePct)
            creature->DespawnOrUnsummon(Milliseconds(1));
    }
};

// ---------------------------------------------------------------------------
//  Gluth: Decimate damage reduction
//
//  Decimate reduces the player to 5% HP.  With no raid healers a solo
//  player is left nearly dead every 90–110s.  Reduce the damage so the
//  player keeps more HP:
//    N=1 → 50% damage (player left at ~52% HP)
//    N=2 → 75% damage (player left at ~28% HP)
//    N=3+ → normal (5% HP)
// ---------------------------------------------------------------------------

class ScaledEncounters_GluthDecimate : public UnitScript
{
public:
    ScaledEncounters_GluthDecimate()
        : UnitScript("ScaledEncounters_GluthDecimate",
                      /*addToScripts=*/true,
                      { UNITHOOK_MODIFY_SPELL_DAMAGE_TAKEN })
    { }

    void ModifySpellDamageTaken(Unit* target, Unit* /*attacker*/, int32& damage, SpellInfo const* spellInfo) override
    {
        if (!spellInfo || !target || !target->IsPlayer())
            return;

        if (spellInfo->Id != SPELL_DECIMATE_DAMAGE)
            return;

        uint32 N = GetGroupSize(target);
        if (N >= 3)
            return;

        if (N <= 1)
            damage /= 2;   // 50% damage → player stays ~52% HP
        else
            damage = damage * 3 / 4;  // 75% damage → player stays ~28% HP
    }
};

// ===========================  Instructor Razuvious  ========================

// ---------------------------------------------------------------------------
//  #2 — Razuvious: Understudy auto-tank at low N
//
//  Normally players MC an understudy to tank Razuvious.  At N=1-2 this
//  is impossible (can't MC and DPS simultaneously).  Override the
//  understudy AI: set faction to friendly (35), auto-engage Razuvious,
//  cast Bone Barrier (29061) on self, and periodically Taunt (29060).
//    N=1-2 → understudies become friendly and auto-tank
//    N=3+  → normal (player MC required)
// ---------------------------------------------------------------------------

enum UnderstudySpells
{
    SPELL_UNDERSTUDY_BONE_BARRIER = 29061,
};

/// RP action IDs sent by Razuvious's AI to the understudy.
enum UnderstudyActions
{
    ACTION_FACE_ME          = 1,
    ACTION_TALK             = 2,
    ACTION_EMOTE            = 3,
    ACTION_SALUTE           = 4,
    ACTION_BACK_TO_TRAINING = 5,
};

enum UnderstudyMisc
{
    NPC_TARGET_DUMMY        = 16211,
    SAY_DEATH_KNIGHT        = 0,  // broadcast_text for understudy line
    GROUP_OOC_RP            = 0,
};

/// Custom AI that preserves the pre-fight training RP, then flips
/// to friendly and auto-tanks Razuvious when combat starts.
struct AutoTankUnderstudyAI : public ScriptedAI
{
    explicit AutoTankUnderstudyAI(Creature* creature) : ScriptedAI(creature)
    {
        _threatTimer = 0;
        _barrierTimer = 0;
        _activated = false;
    }

    uint32 _threatTimer;
    uint32 _barrierTimer;
    bool   _activated;

    // ---- Pre-fight RP (mirrors original boss_razuvious_minionAI) ----

    void Reset() override
    {
        scheduler.CancelAll();
        _activated = false;
        ScheduleAttackDummy();
    }

    void ScheduleAttackDummy()
    {
        me->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_STATE_READY1H);
        if (Creature* dummy = me->FindNearestCreature(NPC_TARGET_DUMMY, 10.0f))
            me->SetFacingToObject(dummy);

        scheduler.Schedule(6s, 9s, GROUP_OOC_RP, [this](TaskContext context)
        {
            me->HandleEmoteCommand(EMOTE_ONESHOT_ATTACK1H);
            context.Repeat(6s, 9s);
        });
    }

    void DoAction(int32 action) override
    {
        switch (action)
        {
            case ACTION_FACE_ME:
                scheduler.CancelGroup(GROUP_OOC_RP);
                me->SetSheath(SHEATH_STATE_UNARMED);
                me->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_STATE_NONE);
                if (Creature* raz = me->FindNearestCreature(NPC_RAZUVIOUS, 100.0f))
                    me->SetFacingToObject(raz);
                break;
            case ACTION_TALK:
                Talk(SAY_DEATH_KNIGHT);
                break;
            case ACTION_EMOTE:
                me->HandleEmoteCommand(EMOTE_ONESHOT_TALK);
                break;
            case ACTION_SALUTE:
                me->HandleEmoteCommand(EMOTE_ONESHOT_SALUTE);
                break;
            case ACTION_BACK_TO_TRAINING:
                me->SetSheath(SHEATH_STATE_MELEE);
                ScheduleAttackDummy();
                break;
        }
    }

    // ---- Combat: flip to friendly and auto-tank ----

    void Activate()
    {
        _activated = true;
        scheduler.CancelAll();

        me->SetFaction(FACTION_FRIENDLY);
        me->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_STATE_NONE);
        me->SetReactState(REACT_AGGRESSIVE);
        me->CombatStop(true);

        if (Creature* razuvious = me->FindNearestCreature(NPC_RAZUVIOUS, 100.0f))
        {
            AttackStart(razuvious);
            // Seed massive threat so Razuvious attacks the understudy.
            razuvious->AddThreat(me, 1000000.0f);
        }

        me->CastSpell(me, SPELL_UNDERSTUDY_BONE_BARRIER, false);
        _barrierTimer = 30000;
        _threatTimer = 10000;
    }

    void UpdateAI(uint32 diff) override
    {
        scheduler.Update(diff);

        // Check if Razuvious has entered combat — if so, activate.
        if (!_activated)
        {
            if (Creature* razuvious = me->FindNearestCreature(NPC_RAZUVIOUS, 100.0f))
                if (razuvious->IsInCombat())
                    Activate();
            return;
        }

        // Razuvious died — despawn.
        Creature* razuvious = me->FindNearestCreature(NPC_RAZUVIOUS, 100.0f);
        if (!razuvious || !razuvious->IsAlive())
        {
            me->DespawnOrUnsummon(Milliseconds(1));
            return;
        }

        if (!UpdateVictim())
        {
            AttackStart(razuvious);
            return;
        }

        // Periodically top up threat so Razuvious stays on us.
        if (_threatTimer <= diff)
        {
            razuvious->AddThreat(me, 500000.0f);
            _threatTimer = 10000;
        }
        else
            _threatTimer -= diff;

        if (_barrierTimer <= diff)
        {
            me->CastSpell(me, SPELL_UNDERSTUDY_BONE_BARRIER, false);
            _barrierTimer = 30000;
        }
        else
            _barrierTimer -= diff;

        DoMeleeAttackIfReady();
    }
};

class ScaledEncounters_RazuviousUnderstudy : public AllCreatureScript
{
public:
    ScaledEncounters_RazuviousUnderstudy()
        : AllCreatureScript("ScaledEncounters_RazuviousUnderstudy")
    { }

    CreatureAI* GetCreatureAI(Creature* creature) const override
    {
        if (!creature || creature->GetEntry() != NPC_DEATH_KNIGHT_UNDERSTUDY)
            return nullptr;

        Map* map = creature->GetMap();
        if (!map || !map->IsDungeon())
            return nullptr;

        uint32 N = GetGroupSize(map);
        if (N >= 3)
            return nullptr; // normal MC mechanic

        return new AutoTankUnderstudyAI(creature);
    }
};

// ===========================  Gothik the Harvester  ========================

// ---------------------------------------------------------------------------
//  Gothik: Open the inner gate when the first dead-side add spawns
//
//  The living/dead side split is structurally impossible solo.  The gate
//  closes on engage and living adds spawn on the player's side normally.
//  When the first dead-side add spawns, it would appear on the empty
//  side with no target and pile up.  Instead, open the gate at that
//  moment so dead adds path through to the player alongside living adds.
//    N=1-2 → gate opens on first dead add spawn
//    N=3+  → normal (split mechanic)
// ---------------------------------------------------------------------------

static constexpr uint32 GO_GOTHIK_INNER_GATE = 181170;

static bool IsGothikDeadAdd(uint32 entry)
{
    switch (entry)
    {
        case NPC_DEAD_TRAINEE:
        case NPC_DEAD_KNIGHT:
        case NPC_DEAD_HORSE:
        case NPC_DEAD_RIDER:
            return true;
        default:
            return false;
    }
}

class ScaledEncounters_GothikGate : public AllCreatureScript
{
public:
    ScaledEncounters_GothikGate()
        : AllCreatureScript("ScaledEncounters_GothikGate")
    { }

    void OnCreatureAddWorld(Creature* creature) override
    {
        if (!creature || !IsGothikDeadAdd(creature->GetEntry()))
            return;

        Map* map = creature->GetMap();
        if (!map || !map->IsDungeon())
            return;

        uint32 N = GetGroupSize(map);
        if (N >= 3)
            return;

        // Open the inner gate so dead adds can path to the player's side.
        if (GameObject* gate = creature->FindNearestGameObject(GO_GOTHIK_INNER_GATE, 200.0f))
            gate->SetGoState(GO_STATE_ACTIVE);
    }
};

// ===========================  Four Horsemen  ===============================

// ---------------------------------------------------------------------------
//  #8 — Four Horsemen: Mark stack cap at low N
//
//  Each horseman applies an independent mark every 12-15s.  Damage
//  escalates exponentially with stacks (stack 5 = 12,500 per mark).
//  A solo player accumulates marks from all 4 horsemen simultaneously.
//  Cap the maximum stack count based on N:
//    N=1 → cap at 3 stacks (max 1,500 dmg per mark)
//    N=2 → cap at 4 stacks (max 4,000 dmg per mark)
//    N=3 → cap at 5 stacks (max 12,500 dmg per mark)
//    N=4+ → no cap (normal)
// ---------------------------------------------------------------------------

static bool IsHorsemanMark(uint32 spellId)
{
    switch (spellId)
    {
        case SPELL_MARK_OF_KORTHAZZ:
        case SPELL_MARK_OF_BLAUMEUX:
        case SPELL_MARK_OF_RIVENDARE:
        case SPELL_MARK_OF_ZELIEK:
            return true;
        default:
            return false;
    }
}

class ScaledEncounters_FourHorsemenMarks : public UnitScript
{
public:
    ScaledEncounters_FourHorsemenMarks()
        : UnitScript("ScaledEncounters_FourHorsemenMarks",
                      /*addToScripts=*/true,
                      { UNITHOOK_ON_AURA_APPLY })
    { }

    void OnAuraApply(Unit* target, Aura* aura) override
    {
        if (!target || !aura || !target->IsPlayer())
            return;

        if (!IsHorsemanMark(aura->GetId()))
            return;

        uint32 N = GetGroupSize(target);
        if (N >= 4)
            return;

        uint8 maxStacks = 0;
        if (N <= 1)
            maxStacks = 3;
        else if (N == 2)
            maxStacks = 4;
        else // N == 3
            maxStacks = 5;

        if (aura->GetStackAmount() > maxStacks)
            aura->SetStackAmount(maxStacks);
    }
};

// ---------------------------------------------------------------------------
//  Registration
// ---------------------------------------------------------------------------

void AddSC_scaled_encounters_naxx()
{
    // Construct Quarter
    new ScaledEncounters_ThaddiusPolarity();
    new ScaledEncounters_ThaddMagneticPull();
    new ScaledEncounters_GluthZombies();
    new ScaledEncounters_GluthDecimate();
    // Military Quarter
    new ScaledEncounters_RazuviousUnderstudy();
    new ScaledEncounters_GothikGate();
    new ScaledEncounters_FourHorsemenMarks();
}
