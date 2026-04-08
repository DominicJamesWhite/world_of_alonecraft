/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license:
 * https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE-AGPL3
 *
 * Scaled encounter overrides for Naxxramas (1–5 players):
 *
 * Arachnid Quarter:
 *   - Anub'Rekhan           (locust swarm kite-scaling + impale blocked during swarm)
 *   - Grand Widow Faerlina   (worshipper respawn at low N)
 *   - Maexxna               (necrotic poison scaling at low N)
 *
 * Plague Quarter:
 *   - Heigan the Unclean     (spell disruption scaling at low N)
 *   - Eye Stalk gauntlet     (thinning at low N)
 *
 * Construct Quarter:
 *   - Gluth                 (zombie chow spawn scaling, Decimate damage)
 *   - Thaddius              (polarity shift damage scaling, Magnetic Pull block)
 *
 * Military Quarter:
 *   - Death Knight Cavalier  (bone armor scaling at low N)
 *   - Instructor Razuvious  (crystal converts understudies to guardians at low N)
 *   - Gothik the Harvester  (gate open on dead add spawn at low N)
 *   - Four Horsemen         (mark stack cap at low N)
 */

#include "GameTime.h"
#include "ScaledEncounters.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "Spell.h"
#include "SpellAuraEffects.h"
#include "SpellAuras.h"
#include "SpellInfo.h"

// ===========================  Spell / NPC IDs  =============================

enum NaxxSpells
{
    // Anub'Rekhan
    SPELL_LOCUST_SWARM_BUFF     = 28785,  // self-buff: triggers tick + self-slow
    SPELL_LOCUST_SWARM_TICK     = 28786,  // triggered AoE: periodic dmg + silence + pacify
    SPELL_IMPALE                = 28783,  // random-target dmg + knockback
    // Maexxna
    SPELL_NECROTIC_POISON_MAEX  = 54121,  // -75% healing for 30s
    // Heigan
    SPELL_SPELL_DISRUPTION      = 29310,  // -300% cast speed, 5s
    // Military Quarter trash
    SPELL_BONE_ARMOR_10         = 55315,  // 50k absorb, 60s
    SPELL_BONE_ARMOR_25         = 55336,  // 1.2M absorb, 60s
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
    // Arachnid Quarter
    NPC_ANUBREKHAN              = 15956,
    NPC_FAERLINA                = 15953,
    NPC_NAXXRAMAS_WORSHIPPER    = 16506,
    NPC_DK_CAVALIER             = 16163,
    // Plague Quarter
    NPC_EYE_STALK               = 16236,
    // Construct Quarter
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

// ===========================  Anub'Rekhan  ================================

// ---------------------------------------------------------------------------
//  Anub'Rekhan: Locust Swarm kite-scaling
//
//  Locust Swarm (28785) is a self-buff on Anub'Rekhan that slows him by
//  40% and triggers 28786 every 2s on nearby players.  28786 applies
//  periodic damage + silence + pacify.  The intended counter is kiting:
//  the tank outruns the slowed boss while ranged DPS stays outside the
//  swarm radius.
//
//  Solo the player IS the tank and the DPS — kiting is the right answer
//  but the default 40% boss slow isn't enough gap in Anub's room.
//  Fix: drastically increase the boss self-slow so the player can
//  actually outrun him, and reduce tick damage so getting clipped
//  during a turn isn't lethal.  Silence + pacify stay — that's the
//  punishment for bad kiting.
//
//  Boss self-slow (28785 Effect2, default -40%):
//    N=1 → -80%  (boss at 20% speed — very kiteable)
//    N=2 → -70%  (boss at 30% speed)
//    N=3 → -60%  (boss at 40% speed)
//    N=4+ → normal (-40%, boss at 60% speed)
//
//  Tick damage (28786, ~1500/2s):
//    N=1 → 25%    N=2 → 50%    N=3 → 75%    N=4+ → normal
// ---------------------------------------------------------------------------

class ScaledEncounters_AnubLocustSwarm : public UnitScript
{
public:
    ScaledEncounters_AnubLocustSwarm()
        : UnitScript("ScaledEncounters_AnubLocustSwarm",
                      /*addToScripts=*/true,
                      { UNITHOOK_ON_AURA_APPLY, UNITHOOK_MODIFY_SPELL_DAMAGE_TAKEN })
    { }

    void OnAuraApply(Unit* target, Aura* aura) override
    {
        if (!target || !aura)
            return;

        // Increase boss self-slow when Locust Swarm (28785) is applied to Anub'Rekhan
        if (aura->GetId() == SPELL_LOCUST_SWARM_BUFF && target->IsCreature()
            && target->ToCreature()->GetEntry() == NPC_ANUBREKHAN)
        {
            uint32 N = GetGroupSize(target);
            if (N >= 4)
                return;

            // Effect index 1 is MOD_DECREASE_SPEED (default -40)
            if (AuraEffect* slowEffect = aura->GetEffect(EFFECT_1))
            {
                int32 newSlow = -40;
                if (N <= 1)
                    newSlow = -80;  // 20% speed
                else if (N == 2)
                    newSlow = -70;  // 30% speed
                else // N == 3
                    newSlow = -60;  // 40% speed

                slowEffect->ChangeAmount(newSlow);
            }
        }
    }

    void ModifySpellDamageTaken(Unit* target, Unit* /*attacker*/, int32& damage, SpellInfo const* spellInfo) override
    {
        if (!spellInfo || !target || !target->IsPlayer())
            return;

        if (spellInfo->Id != SPELL_LOCUST_SWARM_TICK)
            return;

        uint32 N = GetGroupSize(target);
        if (N >= 4)
            return;

        if (N <= 1)
            damage /= 4;         // 25% — survivable clip
        else if (N == 2)
            damage /= 2;         // 50%
        else // N == 3
            damage = damage * 3 / 4;  // 75%
    }
};

// ---------------------------------------------------------------------------
//  Anub'Rekhan: Impale / Locust Swarm mutual exclusion at low N
//
//  Impale (28783) fires every 20s — ~5k damage + knockback.  During
//  Locust Swarm the knockback disrupts kiting and is often lethal.
//  Additionally, getting Impale-knocked right before swarm starts
//  leaves no time to position for the kite.
//
//    N=1-2:
//      • Impale blocked while Locust Swarm buff is active
//      • Locust Swarm blocked if Impale landed within the last 6s
//    N=3+  → normal
// ---------------------------------------------------------------------------

class ScaledEncounters_AnubImpale : public AllSpellScript
{
public:
    ScaledEncounters_AnubImpale()
        : AllSpellScript("ScaledEncounters_AnubImpale",
                          { ALLSPELLHOOK_ON_SPELL_CHECK_CAST })
    { }

    void OnSpellCheckCast(Spell* spell, bool /*strict*/, SpellCastResult& res) override
    {
        if (!spell || !spell->GetSpellInfo())
            return;

        Unit* caster = spell->GetCaster();
        if (!caster || !caster->IsCreature())
            return;

        if (caster->ToCreature()->GetEntry() != NPC_ANUBREKHAN)
            return;

        uint32 N = GetGroupSize(caster);
        if (N >= 3)
            return;

        uint32 spellId = spell->GetSpellInfo()->Id;
        Milliseconds now = GameTime::GetGameTimeMS();

        if (spellId == SPELL_IMPALE)
        {
            // Block Impale while Locust Swarm is active
            if (caster->HasAura(SPELL_LOCUST_SWARM_BUFF))
            {
                res = SPELL_FAILED_DONT_REPORT;
                return;
            }
            // Record this Impale timestamp for the swarm guard
            _lastImpaleTime = now;
        }
        else if (spellId == SPELL_LOCUST_SWARM_BUFF)
        {
            // Block Locust Swarm if Impale landed within the last 6s
            if (_lastImpaleTime > Milliseconds::zero() && (now - _lastImpaleTime) < 6s)
                res = SPELL_FAILED_DONT_REPORT;
        }
    }

private:
    Milliseconds _lastImpaleTime = Milliseconds::zero();
};

// ===========================  Grand Widow Faerlina  ========================

// ---------------------------------------------------------------------------
//  Faerlina: Worshipper respawn at low N
//
//  Frenzy (28798) fires every 60-80s and can only be removed by
//  Widow's Embrace (28732), cast by a dying Worshipper (10-man) or
//  a Mind-Controlled Worshipper (25-man).  There are exactly 4
//  Worshippers — once they're all dead the Frenzy becomes permanent.
//
//  Solo this creates a hard enrage timer (~4-5 minutes).  Fix: when
//  ALL Worshippers are dead during combat, respawn one after 30s so
//  the player always has a way to handle Frenzy.
//
//    N=1-5 → one Worshipper respawns 30s after the last one dies
//    N=6+  → normal (4 Worshippers, no respawn)
// ---------------------------------------------------------------------------

class ScaledEncounters_FaerlinaWorshippers : public AllCreatureScript
{
public:
    ScaledEncounters_FaerlinaWorshippers()
        : AllCreatureScript("ScaledEncounters_FaerlinaWorshippers")
    { }

    void OnAllCreatureUpdate(Creature* creature, uint32 diff) override
    {
        if (!creature || creature->GetEntry() != NPC_FAERLINA)
            return;

        if (!creature->IsAlive() || !creature->IsInCombat())
        {
            _waiting = false;
            _respawnTimer = 0;
            _checkTimer = 0;
            return;
        }

        uint32 N = GetGroupSize(creature);
        if (N > 5)
            return;

        // If waiting for respawn, count down
        if (_waiting)
        {
            _respawnTimer += diff;
            if (_respawnTimer >= 30000)
            {
                // Respawn one Worshipper at the front of the room
                if (Creature* w = creature->SummonCreature(NPC_NAXXRAMAS_WORSHIPPER, 3353.25f, -3620.0f, 260.9967f, 4.57276f))
                    w->SetInCombatWithZone();

                _waiting = false;
                _respawnTimer = 0;
            }
            return;
        }

        // Periodic check (every 5s): are all Worshippers dead?
        _checkTimer += diff;
        if (_checkTimer < 5000)
            return;
        _checkTimer = 0;

        std::list<Creature*> worshippers;
        creature->GetCreatureListWithEntryInGrid(worshippers, NPC_NAXXRAMAS_WORSHIPPER, 200.0f);
        for (Creature* w : worshippers)
            if (w->IsAlive())
                return;

        // All dead — start 30s respawn timer
        _waiting = true;
        _respawnTimer = 0;
    }

private:
    uint32 _checkTimer = 0;
    uint32 _respawnTimer = 0;
    bool   _waiting = false;
};

// ===========================  Maexxna  =====================================

// ---------------------------------------------------------------------------
//  Maexxna: Necrotic Poison scaling at low N
//
//  Necrotic Poison (54121) reduces healing received by 75% for 30s.
//  With no poison cleanse this is brutal solo — a single application
//  makes self-healing nearly useless for the full duration.
//
//  Scale both the healing reduction and duration down at low N:
//    N=1-2 → -25% healing,  8s duration
//    N=3-4 → -50% healing, 15s duration
//    N=5+  → normal (-75%, 30s)
// ---------------------------------------------------------------------------

class ScaledEncounters_MaexxnaPoison : public UnitScript
{
public:
    ScaledEncounters_MaexxnaPoison()
        : UnitScript("ScaledEncounters_MaexxnaPoison",
                      /*addToScripts=*/true,
                      { UNITHOOK_ON_AURA_APPLY })
    { }

    void OnAuraApply(Unit* target, Aura* aura) override
    {
        if (!target || !aura || !target->IsPlayer())
            return;

        if (aura->GetId() != SPELL_NECROTIC_POISON_MAEX)
            return;

        uint32 N = GetGroupSize(target);
        if (N >= 5)
            return;

        // Scale the healing reduction (Effect 0, default -75)
        if (AuraEffect* healEffect = aura->GetEffect(EFFECT_0))
        {
            if (N <= 2)
                healEffect->ChangeAmount(-25);
            else // N == 3-4
                healEffect->ChangeAmount(-50);
        }

        // Scale the duration (default 30s)
        if (N <= 2)
            aura->SetDuration(8000);
        else // N == 3-4
            aura->SetDuration(15000);
    }
};

// ===========================  Heigan the Unclean  ==========================

// ---------------------------------------------------------------------------
//  Heigan: Spell Disruption cast-speed scaling at low N
//
//  Spell Disruption (29310) slows casting speed by 300% for 5s, cast
//  every 10-15s during the slow dance phase.  In a raid this is a
//  minor annoyance — solo it makes casters nearly non-functional
//  since casts take 4x as long with almost no gap between applications.
//
//  Scale the slow down so casters can still function:
//    N=1   → -20%  (minor nuisance)
//    N=2   → -50%  (noticeable but playable)
//    N=3-4 → -100% (casts take 2x as long)
//    N=5+  → normal (-300%)
// ---------------------------------------------------------------------------

class ScaledEncounters_HeiganDisruption : public UnitScript
{
public:
    ScaledEncounters_HeiganDisruption()
        : UnitScript("ScaledEncounters_HeiganDisruption",
                      /*addToScripts=*/true,
                      { UNITHOOK_ON_AURA_APPLY })
    { }

    void OnAuraApply(Unit* target, Aura* aura) override
    {
        if (!target || !aura || !target->IsPlayer())
            return;

        if (aura->GetId() != SPELL_SPELL_DISRUPTION)
            return;

        uint32 N = GetGroupSize(target);
        if (N >= 5)
            return;

        if (AuraEffect* hasteEffect = aura->GetEffect(EFFECT_0))
        {
            if (N <= 1)
                hasteEffect->ChangeAmount(-20);
            else if (N == 2)
                hasteEffect->ChangeAmount(-50);
            else // N == 3-4
                hasteEffect->ChangeAmount(-100);
        }
    }
};

// ---------------------------------------------------------------------------
//  Plague Quarter: Eye Stalk thinning at low N
//
//  The gauntlet between Heigan and Loatheb has 17 Eye Stalks that
//  continuously Mind Flay (2.5k-4k shadow/sec + 50% slow).  Running
//  through this solo is a death sentence from stacked Mind Flays.
//
//  Despawn a percentage on spawn:
//    N=1 → keep ~7 (despawn 60%)
//    N=2 → keep ~9 (despawn 50%)
//    N=3 → keep ~12 (despawn 30%)
//    N=4 → keep ~14 (despawn 15%)
//    N=5+ → normal (all 17)
// ---------------------------------------------------------------------------

class ScaledEncounters_EyeStalks : public AllCreatureScript
{
public:
    ScaledEncounters_EyeStalks()
        : AllCreatureScript("ScaledEncounters_EyeStalks")
    { }

    void OnCreatureAddWorld(Creature* creature) override
    {
        if (!creature || creature->GetEntry() != NPC_EYE_STALK)
            return;

        Map* map = creature->GetMap();
        if (!map || !map->IsDungeon())
            return;

        uint32 N = GetGroupSize(map);
        if (N >= 5)
            return;

        uint32 despawnPct = 0;
        if (N <= 1)
            despawnPct = 60;
        else if (N == 2)
            despawnPct = 50;
        else if (N == 3)
            despawnPct = 30;
        else // N == 4
            despawnPct = 15;

        if (urand(1, 100) <= despawnPct)
            creature->DespawnOrUnsummon(Milliseconds(1));
    }
};

// ===========================  Military Quarter Trash  ======================

// ---------------------------------------------------------------------------
//  Death Knight Cavalier: Bone Armor scaling at low N
//
//  Bone Armor (55315 / 55336) absorbs 50k–1.2M damage for 60s and
//  recasts every 15-20s via SmartAI, making the Cavalier effectively
//  invulnerable to a solo player.  Classes without magic dispel have
//  no way to remove it.
//
//  Fix: reduce the absorb to a breakable amount and shorten the
//  duration so there's a natural gap between shields (SmartAI recasts
//  every ~15-20s, so an 8s shield = 7-12s vulnerability window).
//
//    N=1-2 → 5,000 absorb,  8s duration
//    N=3-4 → 15,000 absorb, 12s duration
//    N=5+  → normal
// ---------------------------------------------------------------------------

class ScaledEncounters_CavalierBoneArmor : public UnitScript
{
public:
    ScaledEncounters_CavalierBoneArmor()
        : UnitScript("ScaledEncounters_CavalierBoneArmor",
                      /*addToScripts=*/true,
                      { UNITHOOK_ON_AURA_APPLY })
    { }

    void OnAuraApply(Unit* target, Aura* aura) override
    {
        if (!target || !aura || !target->IsCreature())
            return;

        if (target->ToCreature()->GetEntry() != NPC_DK_CAVALIER)
            return;

        uint32 spellId = aura->GetId();
        if (spellId != SPELL_BONE_ARMOR_10 && spellId != SPELL_BONE_ARMOR_25)
            return;

        uint32 N = GetGroupSize(target);
        if (N >= 5)
            return;

        // Scale absorb amount (Effect 0)
        if (AuraEffect* absorbEffect = aura->GetEffect(EFFECT_0))
        {
            if (N <= 2)
                absorbEffect->SetAmount(5000);
            else // N == 3-4
                absorbEffect->SetAmount(15000);
        }

        // Shorten duration — SmartAI recasts every 15-20s,
        // so a short duration creates a natural vulnerability window
        if (N <= 2)
            aura->SetDuration(8000);
        else // N == 3-4
            aura->SetDuration(12000);
    }
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
//  #2 — Razuvious: Obedience Crystal converts understudies at low N
//
//  Normally players MC an understudy via the Obedience Crystal (NPC
//  29912, spellclick → Force Obedience 55479) to tank Razuvious.
//  At N=1-2 this is impossible (can't MC and DPS simultaneously).
//
//  Fix: at low N, clicking the crystal despawns all hostile understudies
//  and respawns them as player-faction guardians that auto-tank
//  Razuvious.  Using the player's faction (not FACTION_FRIENDLY) means
//  they're naturally hostile to Scourge — standard combat helpers work.
//
//    N=1-2 → crystal converts understudies into guardian tanks
//    N=3+  → normal (crystal MCs as usual)
// ---------------------------------------------------------------------------

enum UnderstudySpells
{
    SPELL_FORCE_OBEDIENCE         = 55479,
    SPELL_UNDERSTUDY_BONE_BARRIER = 29061,
};

/// Guardian AI for converted understudies — auto-tanks Razuvious.
struct GuardianUnderstudyAI : public ScriptedAI
{
    explicit GuardianUnderstudyAI(Creature* creature) : ScriptedAI(creature)
    {
        _barrierTimer = 0;
        _threatTimer = 0;
    }

    uint32 _barrierTimer;
    uint32 _threatTimer;

    void Activate(Player* player)
    {
        me->SetFaction(player->GetFaction());
        me->SetReactState(REACT_AGGRESSIVE);
        me->CombatStop(true);

        if (Creature* razuvious = me->FindNearestCreature(NPC_RAZUVIOUS, 100.0f))
        {
            AttackStart(razuvious);
            razuvious->AddThreat(me, 1000000.0f);
        }

        me->CastSpell(me, SPELL_UNDERSTUDY_BONE_BARRIER, false);
        _barrierTimer = 30000;
        _threatTimer = 10000;
    }

    void UpdateAI(uint32 diff) override
    {
        // Razuvious died or reset — revert to original faction.
        Creature* razuvious = me->FindNearestCreature(NPC_RAZUVIOUS, 100.0f);
        if (!razuvious || !razuvious->IsAlive() || !razuvious->IsInCombat())
        {
            EnterEvadeMode();
            return;
        }

        if (!UpdateVictim())
        {
            AttackStart(razuvious);
            return;
        }

        // Keep threat topped up so Razuvious stays on us
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

/// At low N, clicking the Obedience Crystal converts understudies
/// into friendly guardians instead of mind-controlling them.
class ScaledEncounters_RazuviousCrystal : public AllSpellScript
{
public:
    ScaledEncounters_RazuviousCrystal()
        : AllSpellScript("ScaledEncounters_RazuviousCrystal",
                          { ALLSPELLHOOK_ON_SPELL_CHECK_CAST })
    { }

    void OnSpellCheckCast(Spell* spell, bool /*strict*/, SpellCastResult& res) override
    {
        if (!spell || !spell->GetSpellInfo())
            return;

        if (spell->GetSpellInfo()->Id != SPELL_FORCE_OBEDIENCE)
            return;

        Unit* caster = spell->GetCaster();
        if (!caster || !caster->IsPlayer())
            return;

        uint32 N = GetGroupSize(caster);
        if (N >= 3)
            return;

        // Block the MC spell
        res = SPELL_FAILED_DONT_REPORT;

        Player* player = caster->ToPlayer();

        // Convert all living understudies in place
        std::list<Creature*> understudies;
        caster->GetCreatureListWithEntryInGrid(understudies, NPC_DEATH_KNIGHT_UNDERSTUDY, 200.0f);
        for (Creature* u : understudies)
        {
            if (!u->IsAlive())
                continue;

            // Swap to guardian AI and activate
            auto* ai = new GuardianUnderstudyAI(u);
            u->SetAI(ai);
            ai->Activate(player);
        }
    }
};

// ===========================  Gothik the Harvester  ========================

// ---------------------------------------------------------------------------
//  Gothik: Open the inner gate when the first dead-side add spawns
//
//  The living/dead side split is structurally impossible solo.  The gate
//  closes on engage and living adds spawn on the player's side normally.
//  When the first dead-side add spawns, it would appear on the empty
//  side with no target and pile up.  Fix: open the gate AND force each
//  dead add into combat so it paths through to the player.  (Just
//  opening the gate isn't enough — the boss AI's gateOpened flag stays
//  false, so JustSummoned never assigns a target.)
//    N=1-2 → gate opens + dead adds aggro on spawn
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

    void OnAllCreatureUpdate(Creature* creature, uint32 /*diff*/) override
    {
        if (!creature || !IsGothikDeadAdd(creature->GetEntry()))
            return;

        if (!creature->IsAlive() || creature->GetVictim())
            return;

        Map* map = creature->GetMap();
        if (!map || !map->IsDungeon())
            return;

        uint32 N = GetGroupSize(map);
        if (N >= 3)
            return;

        // The minion AI never calls UpdateVictim() — it relies on
        // JustSummoned to assign a target, which only picks same-side
        // players.  Force idle dead adds to attack the nearest player.
        if (Player* player = creature->SelectNearestPlayer(200.0f))
        {
            creature->AI()->AttackStart(player);
            creature->SetReactState(REACT_AGGRESSIVE);
            creature->SetInCombatWithZone();
        }
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
    // Arachnid Quarter
    new ScaledEncounters_AnubLocustSwarm();
    new ScaledEncounters_AnubImpale();
    new ScaledEncounters_FaerlinaWorshippers();
    new ScaledEncounters_MaexxnaPoison();
    // Plague Quarter
    new ScaledEncounters_HeiganDisruption();
    new ScaledEncounters_EyeStalks();
    // Construct Quarter
    new ScaledEncounters_ThaddiusPolarity();
    new ScaledEncounters_ThaddMagneticPull();
    new ScaledEncounters_GluthZombies();
    new ScaledEncounters_GluthDecimate();
    // Military Quarter
    new ScaledEncounters_CavalierBoneArmor();
    new ScaledEncounters_RazuviousCrystal();
    new ScaledEncounters_GothikGate();
    new ScaledEncounters_FourHorsemenMarks();
}
