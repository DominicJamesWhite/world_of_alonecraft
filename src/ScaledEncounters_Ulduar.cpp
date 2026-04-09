/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license:
 * https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE-AGPL3
 *
 * Scaled encounter overrides for Ulduar (1–5 players):
 *   - Flame Leviathan   (vehicle damage/HP scaling, turret auto-fire)
 *   - Razorscale         (harpoon count scaling)
 *   - Kologarn           (Stone Grip duration scaling)
 *   - Yogg-Saron         (Brain Link, brain phase add spawns, sanity)
 *   - Assembly of Iron   (Supercharge buff scaling)
 */

#include "InstanceScript.h"
#include "ScaledEncounters.h"
#include "ScriptMgr.h"
#include "SpellAuras.h"

// ===========================  Razorscale  ==================================

enum RazorscaleSpells
{
    SPELL_FUSEARMOR         = 64771, // stacking debuff
    SPELL_FUSED_ARMOR       = 64774, // permanent debuff triggered at 5 stacks
};

/// Cap Fuse Armor stacks at low player counts so the permanent Fused Armor
/// debuff is never applied.  Solo: 2 stacks max, duo: 3, trio: 4, 4+: normal.
class ScaledEncounters_RazorscaleFuseArmor : public UnitScript
{
public:
    ScaledEncounters_RazorscaleFuseArmor()
        : UnitScript("ScaledEncounters_RazorscaleFuseArmor",
                      /*addToScripts=*/true,
                      { UNITHOOK_ON_AURA_APPLY })
    { }

    void OnAuraApply(Unit* target, Aura* aura) override
    {
        if (!target || !aura || !target->IsPlayer())
            return;

        if (aura->GetId() != SPELL_FUSEARMOR)
            return;

        uint32 N = GetGroupSize(target);
        uint8 maxStacks = 0;
        if (N <= 1)
            maxStacks = 2;
        else if (N == 2)
            maxStacks = 3;
        else if (N == 3)
            maxStacks = 4;

        if (maxStacks && aura->GetStackAmount() > maxStacks)
            aura->SetStackAmount(maxStacks);
    }
};

// ===========================  Kologarn  ====================================

enum KologarnSpells
{
    SPELL_STONE_GRIP            = 62166, // 10-man Stone Grip (stun + vehicle ride)
    SPELL_STONE_GRIP_25         = 63981, // 25-man variant
};

/// Suppress Stone Grip at low player counts.  The grip stuns the player and
/// puts them in a vehicle seat on the right arm — with nobody to DPS the arm
/// free, this is an encounter-breaking mechanic for solo/duo.
///
///   N=1-2 → grip is removed immediately on application
///   N=3+  → normal behaviour
class ScaledEncounters_KologarnGrip : public UnitScript
{
public:
    ScaledEncounters_KologarnGrip()
        : UnitScript("ScaledEncounters_KologarnGrip",
                      /*addToScripts=*/true,
                      { UNITHOOK_ON_AURA_APPLY })
    { }

    void OnAuraApply(Unit* target, Aura* aura) override
    {
        if (!target || !aura || !target->IsPlayer())
            return;

        uint32 spellId = aura->GetId();
        if (spellId != SPELL_STONE_GRIP && spellId != SPELL_STONE_GRIP_25)
            return;

        uint32 N = GetGroupSize(target);
        if (N <= 2)
            target->RemoveAura(spellId);
    }
};

// ===========================  XT-002 Deconstructor  ========================

enum XT002Npcs
{
    NPC_XS013_SCRAPBOT          = 33343,
};

/// Despawn scrapbots at low player counts.  Each scrapbot that reaches XT-002
/// heals him for 1% max HP — solo players in melee range can't intercept them
/// all, making the heart phase a net heal instead of a DPS window.
///
///   N=1   → all scrapbots despawned on spawn
///   N=2   → 50% despawned
///   N=3   → 30% despawned
///   N=4+  → normal
class ScaledEncounters_XT002Scrapbots : public AllCreatureScript
{
public:
    ScaledEncounters_XT002Scrapbots()
        : AllCreatureScript("ScaledEncounters_XT002Scrapbots")
    { }

    void OnCreatureAddWorld(Creature* creature) override
    {
        if (!creature || creature->GetEntry() != NPC_XS013_SCRAPBOT)
            return;

        Map* map = creature->GetMap();
        if (!map || !map->IsDungeon())
            return;

        uint32 N = GetGroupSize(map);

        if (N <= 1)
        {
            creature->DespawnOrUnsummon(Milliseconds(1));
            return;
        }

        uint32 despawnChancePct = 0;
        if (N == 2)
            despawnChancePct = 50;
        else if (N == 3)
            despawnChancePct = 30;

        if (despawnChancePct && urand(1, 100) <= despawnChancePct)
            creature->DespawnOrUnsummon(Milliseconds(1));
    }
};

// ===========================  Assembly of Iron  ============================

enum AssemblySpells
{
    SPELL_LIGHTNING_TENDRILS     = 61887, // 10-man self-buff (triggers flight + chase)
    SPELL_LIGHTNING_TENDRILS_25  = 63486, // 25-man variant
    SPELL_RUNE_OF_POWER         = 61973, // ground buff zone (cast on lowest-HP boss)
    SPELL_RUNE_OF_DEATH         = 62269, // ground damage zone (cast on random player)
    SPELL_STATIC_DISRUPTION     = 61911, // nature damage + stacking nature vuln debuff
    SPELL_OVERWHELMING_POWER    = 64637, // phase 3 buff → Meltdown on expiry
    SPELL_ELECTRICAL_CHARGE     = 61902, // Steelbreaker self-heal/buff on tank death
};

enum AssemblyNpcs
{
    NPC_MOLGEIM                 = 32927,
    NPC_STEELBREAKER            = 32867,
};

/// Prevent Rune of Power and Rune of Death from overlapping at low player
/// counts.  Solo, all bosses stack on the player so both ground effects land
/// in the same spot — the player can't benefit from Power without standing in
/// Death.  When both DynObjects are active simultaneously, remove Rune of Death.
///
///   N=1-2 → Rune of Death removed if Rune of Power is also active
///   N=3+  → normal behaviour
class ScaledEncounters_MolgeimRunes : public AllCreatureScript
{
public:
    ScaledEncounters_MolgeimRunes()
        : AllCreatureScript("ScaledEncounters_MolgeimRunes")
    { }

    void OnAllCreatureUpdate(Creature* creature, uint32 /*diff*/) override
    {
        if (!creature || creature->GetEntry() != NPC_MOLGEIM)
            return;

        if (!creature->IsInCombat())
            return;

        uint32 N = GetGroupSize(creature);
        if (N > 2)
            return;

        // If both runes are active on the ground at once, remove Death
        if (creature->GetDynObject(SPELL_RUNE_OF_POWER) &&
            creature->GetDynObject(SPELL_RUNE_OF_DEATH))
            creature->RemoveDynObject(SPELL_RUNE_OF_DEATH);
    }
};

/// Replace the Overwhelming Power → Meltdown instakill with a Steelbreaker
/// heal at low player counts.  Normally a tank swap absorbs the kill, but solo
/// there's nobody to swap with.
///
/// On apply:  extend duration from 30s to 60s (more time to DPS).
/// On expiry: remove the aura manually before Meltdown triggers, then heal
///            Steelbreaker via Electrical Charge instead of killing the player.
///
///   N=1-2 → modified behaviour (60s duration, no instakill, boss heals)
///   N=3+  → normal
class ScaledEncounters_OverwhelmingPower : public UnitScript
{
public:
    ScaledEncounters_OverwhelmingPower()
        : UnitScript("ScaledEncounters_OverwhelmingPower",
                      /*addToScripts=*/true,
                      { UNITHOOK_ON_AURA_APPLY, UNITHOOK_ON_UNIT_UPDATE })
    { }

    void OnAuraApply(Unit* target, Aura* aura) override
    {
        if (!target || !aura || !target->IsPlayer())
            return;

        if (aura->GetId() != SPELL_OVERWHELMING_POWER)
            return;

        uint32 N = GetGroupSize(target);
        if (N <= 2)
        {
            aura->SetMaxDuration(60000);
            aura->SetDuration(60000);
        }
    }

    void OnUnitUpdate(Unit* unit, uint32 /*diff*/) override
    {
        if (!unit || !unit->IsPlayer())
            return;

        Aura* aura = unit->GetAura(SPELL_OVERWHELMING_POWER);
        if (!aura)
            return;

        uint32 N = GetGroupSize(unit);
        if (N > 2)
            return;

        // When less than 500ms remains, intercept before Meltdown triggers
        if (aura->GetDuration() < 500)
        {
            // Remove the aura manually — AURA_REMOVE_BY_CANCEL does not
            // trigger the "on expire" Meltdown spell
            unit->RemoveAura(SPELL_OVERWHELMING_POWER, ObjectGuid::Empty,
                             0, AURA_REMOVE_BY_CANCEL);

            // Heal Steelbreaker via Electrical Charge (same as a tank death)
            if (InstanceScript* instance = unit->GetInstanceScript())
            {
                if (Creature* steelbreaker = instance->GetCreature(20 /*DATA_STEELBREAKER*/))
                    steelbreaker->CastSpell(steelbreaker, SPELL_ELECTRICAL_CHARGE, true);
            }
        }
    }
};

/// Cap Static Disruption stacks at low player counts.  Each stack increases
/// nature damage taken by 10% — solo the debuff stacks endlessly on the only
/// target, making Steelbreaker's nature damage lethal.
///
///   N=1   → max 2 stacks (+20%)
///   N=2   → max 3 stacks (+30%)
///   N=3   → max 4 stacks (+40%)
///   N=4+  → normal
class ScaledEncounters_StaticDisruption : public UnitScript
{
public:
    ScaledEncounters_StaticDisruption()
        : UnitScript("ScaledEncounters_StaticDisruption",
                      /*addToScripts=*/true,
                      { UNITHOOK_ON_AURA_APPLY })
    { }

    void OnAuraApply(Unit* target, Aura* aura) override
    {
        if (!target || !aura || !target->IsPlayer())
            return;

        if (aura->GetId() != SPELL_STATIC_DISRUPTION)
            return;

        uint32 N = GetGroupSize(target);
        uint8 maxStacks = 0;
        if (N <= 1)
            maxStacks = 2;
        else if (N == 2)
            maxStacks = 3;
        else if (N == 3)
            maxStacks = 4;

        if (maxStacks && aura->GetStackAmount() > maxStacks)
            aura->SetStackAmount(maxStacks);
    }
};

/// Root Brundir during Lightning Tendrils at low player counts.  Normally he
/// flies up and chases a random player — solo there's no one else to run to,
/// so the mechanic becomes unavoidable damage.  Rooting him lets the tendril
/// damage still tick but keeps him stationary so a solo player can move away.
///
///   N=1-2 → Brundir is rooted in place for the duration of Tendrils
///   N=3+  → normal behaviour (chases players)
class ScaledEncounters_BrundirTendrils : public UnitScript
{
public:
    ScaledEncounters_BrundirTendrils()
        : UnitScript("ScaledEncounters_BrundirTendrils",
                      /*addToScripts=*/true,
                      { UNITHOOK_ON_AURA_APPLY, UNITHOOK_ON_AURA_REMOVE })
    { }

    static bool IsTendrilSpell(uint32 id)
    {
        return id == SPELL_LIGHTNING_TENDRILS || id == SPELL_LIGHTNING_TENDRILS_25;
    }

    void OnAuraApply(Unit* target, Aura* aura) override
    {
        if (!target || !aura || !target->IsCreature())
            return;

        if (!IsTendrilSpell(aura->GetId()))
            return;

        uint32 N = GetGroupSize(target);
        if (N <= 2)
        {
            target->StopMoving();
            target->GetMotionMaster()->Clear();
            target->SetControlled(true, UNIT_STATE_ROOT);
        }
    }

    void OnAuraRemove(Unit* target, AuraApplication* aurApp, AuraRemoveMode /*mode*/) override
    {
        if (!target || !aurApp || !target->IsCreature())
            return;

        if (!IsTendrilSpell(aurApp->GetBase()->GetId()))
            return;

        uint32 N = GetGroupSize(target);
        if (N <= 2)
            target->SetControlled(false, UNIT_STATE_ROOT);
    }
};

// ===========================  Auriaya  =====================================

enum AuriayaSpells
{
    SPELL_SAVAGE_POUNCE             = 64666, // Sanctum Sentry charge + stun + bleed
    SPELL_STRENGTH_OF_THE_PACK      = 64369, // damage buff when sentries near each other
};

enum AuriayaNpcs
{
    NPC_SANCTUM_SENTRY              = 34014,
};

/// Nerf the Auriaya pull at low player counts.  The Sanctum Sentries' Savage
/// Pounce + Strength of the Pack buff combine to one-shot solo players on the
/// initial engage — there's no way to avoid the pounce since the sentries
/// immediately charge when pulled.
///
/// Two-part fix:
///   1. Reduce Savage Pounce damage via ModifySpellDamageTaken
///   2. Remove Strength of the Pack buff on application
///
///   N=1   → pounce damage at 25%, pack buff removed
///   N=2   → pounce damage at 50%, pack buff removed
///   N=3   → pounce damage at 75%
///   N=4+  → normal
class ScaledEncounters_AuriayaPull : public UnitScript
{
public:
    ScaledEncounters_AuriayaPull()
        : UnitScript("ScaledEncounters_AuriayaPull",
                      /*addToScripts=*/true,
                      { UNITHOOK_ON_AURA_APPLY, UNITHOOK_MODIFY_SPELL_DAMAGE_TAKEN })
    { }

    void OnAuraApply(Unit* target, Aura* aura) override
    {
        if (!target || !aura || !target->IsCreature())
            return;

        if (aura->GetId() != SPELL_STRENGTH_OF_THE_PACK)
            return;

        if (target->ToCreature()->GetEntry() != NPC_SANCTUM_SENTRY)
            return;

        uint32 N = GetGroupSize(target);
        if (N <= 2)
            target->RemoveAura(SPELL_STRENGTH_OF_THE_PACK);
    }

    void ModifySpellDamageTaken(Unit* target, Unit* /*attacker*/, int32& damage, SpellInfo const* spellInfo) override
    {
        if (!spellInfo || !target || !target->IsPlayer())
            return;

        if (spellInfo->Id != SPELL_SAVAGE_POUNCE)
            return;

        uint32 N = GetGroupSize(target);
        if (N <= 1)
            damage /= 4;         // 25%
        else if (N == 2)
            damage /= 2;         // 50%
        else if (N == 3)
            damage = damage * 3 / 4;  // 75%
    }
};

// ===========================  Freya  =======================================

enum FreyaSpells
{
    SPELL_UNSTABLE_GROWTH           = 200307, // custom: root + 1% HP/s regen
    SPELL_HARDENED_BARK             = 62664,  // Snaplasher stacking damage reduction
};

enum FreyaNpcs
{
    NPC_DETONATING_LASHER           = 32918,
    NPC_SNAPLASHER                  = 32916,
};

/// Make Detonating Lashers manageable at low player counts.  Ten lashers spawn
/// simultaneously and explode on death (SPELL_DETONATE 62598) — solo, they all
/// rush the player and chain-detonate for a guaranteed kill.
///
/// At low N, lashers that drop below 25% HP gain the "Unstable Growth" buff
/// (200307) — a visible root + 1% HP/s regen aura.  This gives the player
/// time to back away before finishing them off at range, while the regen keeps
/// things engaging.  The buff is removed when the lasher heals above 25%.
///
///   N=1-2 → Unstable Growth applied at 25% HP, removed above 25%
///   N=3+  → normal
class ScaledEncounters_DetonatingLashers : public AllCreatureScript
{
public:
    ScaledEncounters_DetonatingLashers()
        : AllCreatureScript("ScaledEncounters_DetonatingLashers")
    { }

    void OnAllCreatureUpdate(Creature* creature, uint32 /*diff*/) override
    {
        if (!creature || creature->GetEntry() != NPC_DETONATING_LASHER)
            return;

        if (!creature->IsAlive() || !creature->IsInCombat())
            return;

        uint32 N = GetGroupSize(creature);
        if (N > 2)
            return;

        bool isLowHp = creature->HealthBelowPct(25);
        bool hasBuff  = creature->HasAura(SPELL_UNSTABLE_GROWTH);

        if (isLowHp && !hasBuff)
        {
            creature->StopMoving();
            creature->GetMotionMaster()->Clear();
            creature->CastSpell(creature, SPELL_UNSTABLE_GROWTH, true);
        }
        else if (!isLowHp && hasBuff)
        {
            creature->RemoveAura(SPELL_UNSTABLE_GROWTH);
        }
    }
};

/// Slow Snaplashers and reduce their damage buff as Hardened Bark stacks
/// increase at low player counts.  Each stack normally gives +2% damage done
/// and -2% damage taken — solo the Snaplasher becomes unkillable AND hits
/// like a truck.
///
/// At low N:
///   - Speed reduced by 10% per stack (kiting counterplay)
///   - Damage increase effect (Effect 0) halved per stack
///   - At 20 stacks the aura resets to 0 (damage cycle restarts)
///
///   N=1-2 → slowed + damage buff halved + reset at 20 stacks
///   N=3+  → normal
class ScaledEncounters_SnaplasherBark : public UnitScript
{
public:
    ScaledEncounters_SnaplasherBark()
        : UnitScript("ScaledEncounters_SnaplasherBark",
                      /*addToScripts=*/true,
                      { UNITHOOK_ON_AURA_APPLY })
    { }

    void OnAuraApply(Unit* target, Aura* aura) override
    {
        if (!target || !aura || !target->IsCreature())
            return;

        if (aura->GetId() != SPELL_HARDENED_BARK)
            return;

        if (target->ToCreature()->GetEntry() != NPC_SNAPLASHER)
            return;

        uint32 N = GetGroupSize(target);
        if (N > 2)
            return;

        uint8 stacks = aura->GetStackAmount();

        // At 50 stacks, reset the whole aura — damage cycle restarts
        if (stacks >= 20)
        {
            target->RemoveAura(SPELL_HARDENED_BARK);
            target->SetSpeed(MOVE_RUN, target->GetCreatureTemplate()->speed_run, true);
            return;
        }

        // 10% slower per stack: 1 stack = 90%, 5 stacks = 50%, 10 = 0% (rooted)
        float rate = std::max(0.0f, 1.0f - (stacks * 0.1f));
        target->SetSpeed(MOVE_RUN, target->GetCreatureTemplate()->speed_run * rate, true);

        // Halve the damage increase effect (Effect 0: MOD_DAMAGE_PERCENT_DONE)
        if (AuraEffect* dmgEffect = aura->GetEffect(EFFECT_0))
        {
            // Default is +2% per stack; reduce to +1% per stack
            int32 reducedAmount = stacks; // 1% per stack instead of 2%
            dmgEffect->ChangeAmount(reducedAmount);
        }
    }
};

void AddSC_scaled_encounters_ulduar()
{
    new ScaledEncounters_RazorscaleFuseArmor();
    new ScaledEncounters_KologarnGrip();
    new ScaledEncounters_XT002Scrapbots();
    new ScaledEncounters_MolgeimRunes();
    new ScaledEncounters_OverwhelmingPower();
    new ScaledEncounters_StaticDisruption();
    new ScaledEncounters_BrundirTendrils();
    new ScaledEncounters_AuriayaPull();
    new ScaledEncounters_DetonatingLashers();
    new ScaledEncounters_SnaplasherBark();
}
