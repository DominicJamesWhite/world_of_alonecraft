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
 *   - Thorim             (solo encounter redesign: arena/corridor choice)
 */

#include "InstanceScript.h"
#include "ScaledEncounters.h"
#include "ScriptMgr.h"
#include "SpellAuraEffects.h"
#include "SpellAuras.h"
#include "SpellScript.h"

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
    SPELL_CRUNCH_ARMOR_10       = 63355, // 10-man: -20% armor per stack
    SPELL_CRUNCH_ARMOR_25       = 64002, // 25-man: -25% armor per stack
    SPELL_STONE_NOVA            = 63978, // Rubble: +5% damage taken per stack
};

/// Cap Kologarn stacking debuffs at low player counts.
/// Crunch Armor strips armor indefinitely without a tank swap; Stone Nova
/// stacks damage taken from rubble spawns with no way to avoid them solo.
///
///   N=1-2 → both debuffs removed at 4 stacks
///   N=3+  → normal behaviour
class ScaledEncounters_KologarnStacks : public UnitScript
{
public:
    ScaledEncounters_KologarnStacks()
        : UnitScript("ScaledEncounters_KologarnStacks",
                      /*addToScripts=*/true,
                      { UNITHOOK_ON_AURA_APPLY })
    { }

    void OnAuraApply(Unit* target, Aura* aura) override
    {
        if (!target || !aura || !target->IsPlayer())
            return;

        uint32 spellId = aura->GetId();
        if (spellId != SPELL_CRUNCH_ARMOR_10 && spellId != SPELL_CRUNCH_ARMOR_25 &&
            spellId != SPELL_STONE_NOVA)
            return;

        if (GetGroupSize(target) <= 2 && aura->GetStackAmount() >= 4)
            target->RemoveAura(spellId);
    }
};

/// Suppress Stone Grip at low player counts.  The grip stuns the player and
/// puts them in a vehicle seat on the right arm — with nobody to DPS the arm
/// free, this is an encounter-breaking mechanic for solo/duo.
///
/// Registered on 62166 (10-man) and 63981 (25-man) — the FORCE_CAST triggers
/// that select targets.  We empty the target list when N<=2, preventing the
/// grip chain (ride + stun + dot) from ever being applied.
class spell_scaled_kologarn_stone_grip : public SpellScript
{
    PrepareSpellScript(spell_scaled_kologarn_stone_grip);

    void FilterTargets(std::list<WorldObject*>& targets)
    {
        if (GetGroupSize(GetCaster()) <= 2)
            targets.clear();
    }

    void Register() override
    {
        OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(
            spell_scaled_kologarn_stone_grip::FilterTargets,
            EFFECT_ALL, TARGET_UNIT_SRC_AREA_ENEMY);
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
    SPELL_IRON_ARROGANCE        = 200308, // 66% melee damage reduction (3 bosses alive)
    SPELL_IRON_CONFIDENCE       = 200309, // 33% melee damage reduction (2 bosses alive)
};

enum AssemblyNpcs
{
    NPC_MOLGEIM                 = 32927,
    NPC_STEELBREAKER            = 32867,
    NPC_BRUNDIR                 = 32857,
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

/// Slow Brundir during Lightning Tendrils at low player counts.  Normally he
/// flies and chases a random player — solo there's no one else to kite to,
/// so the mechanic becomes unavoidable damage.  Reducing his flight speed
/// lets the solo player outrun him.
///
///   N=1-2 → Brundir slowed to 40% flight speed during Tendrils
///   N=3+  → normal behaviour
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
            target->SetSpeedRate(MOVE_FLIGHT, 0.4f);
    }

    void OnAuraRemove(Unit* target, AuraApplication* aurApp, AuraRemoveMode /*mode*/) override
    {
        if (!target || !aurApp || !target->IsCreature())
            return;

        if (!IsTendrilSpell(aurApp->GetBase()->GetId()))
            return;

        uint32 N = GetGroupSize(target);
        if (N <= 2)
            target->SetSpeedRate(MOVE_FLIGHT, 1.0f);
    }
};

/// Reduce Assembly of Iron melee damage at solo player counts based on how
/// many bosses are alive.  Three raid bosses meleeing simultaneously is the
/// primary source of lethal damage, not their special abilities.
///
///   3 alive → Iron Arrogance (66% melee reduction) on all three
///   2 alive → Iron Confidence (33% melee reduction) on the survivors
///   1 alive → no debuff (full melee damage)
///   N=2+    → normal behaviour (no debuffs applied)
class ScaledEncounters_AssemblyMelee : public AllCreatureScript
{
public:
    ScaledEncounters_AssemblyMelee()
        : AllCreatureScript("ScaledEncounters_AssemblyMelee")
    { }

    static bool IsAssemblyBoss(uint32 entry)
    {
        return entry == NPC_STEELBREAKER ||
               entry == NPC_MOLGEIM ||
               entry == NPC_BRUNDIR;
    }

    void OnAllCreatureUpdate(Creature* creature, uint32 /*diff*/) override
    {
        if (!creature || !IsAssemblyBoss(creature->GetEntry()))
            return;

        if (!creature->IsInCombat() || !creature->IsAlive())
        {
            // Clean up debuffs when out of combat or dead
            creature->RemoveAura(SPELL_IRON_ARROGANCE);
            creature->RemoveAura(SPELL_IRON_CONFIDENCE);
            return;
        }

        uint32 N = GetGroupSize(creature);
        if (N > 1)
        {
            // Not solo — remove any debuffs
            creature->RemoveAura(SPELL_IRON_ARROGANCE);
            creature->RemoveAura(SPELL_IRON_CONFIDENCE);
            return;
        }

        InstanceScript* instance = creature->GetInstanceScript();
        if (!instance)
            return;

        uint32 alive = 0;
        for (uint8 i = 0; i < 3; ++i)
            if (Creature* boss = instance->GetCreature(20 /*DATA_STEELBREAKER*/ + i))
                if (boss->IsAlive())
                    ++alive;

        // Determine the correct debuff for this alive-count
        uint32 wanted = 0;
        if (alive >= 3)
            wanted = SPELL_IRON_ARROGANCE;
        else if (alive == 2)
            wanted = SPELL_IRON_CONFIDENCE;
        // alive <= 1: no debuff

        uint32 unwanted = 0;
        if (wanted == SPELL_IRON_ARROGANCE)
            unwanted = SPELL_IRON_CONFIDENCE;
        else if (wanted == SPELL_IRON_CONFIDENCE)
            unwanted = SPELL_IRON_ARROGANCE;
        else
            unwanted = 0; // remove both below

        // Remove the wrong debuff
        if (unwanted)
            creature->RemoveAura(unwanted);
        else
        {
            creature->RemoveAura(SPELL_IRON_ARROGANCE);
            creature->RemoveAura(SPELL_IRON_CONFIDENCE);
        }

        // Apply the correct debuff if not already present
        if (wanted && !creature->HasAura(wanted))
            creature->CastSpell(creature, wanted, true);
    }
};

// ===========================  Auriaya  =====================================

enum AuriayaSpells
{
    SPELL_SAVAGE_POUNCE             = 64666, // Sanctum Sentry charge + stun + bleed
    SPELL_STRENGTH_OF_THE_PACK      = 64369, // damage buff when sentries near each other
    SPELL_SONIC_SCREECH_10          = 64422, // 10-man: ~60k split among all targets
    SPELL_SONIC_SCREECH_25          = 64688, // 25-man: ~190k split among all targets
    SPELL_FERAL_ESSENCE_BUFF        = 64455, // Feral Defender: +50% dmg per stack (8 max)
};

enum AuriayaNpcs
{
    NPC_SANCTUM_SENTRY              = 34014,
    NPC_FERAL_DEFENDER              = 34035,
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

/// Reduce Feral Essence damage bonus per stack at low player counts.
/// Normally each stack gives +50% damage done — at 8 stacks that's +400%,
/// which is tuned for an offtank to absorb.  Solo there's no offtank, so the
/// Defender's melee + pounce become lethal.
///
/// Uses OnAllCreatureUpdate because the boss script applies the aura via
/// AddAura + SetStackAmount, which recalculates effect amounts and overwrites
/// any OnAuraApply modification.  The update hook re-enforces the reduced
/// amount each tick (only runs for NPC 34035).
///
///   N=1   → +10% per stack (down from +50%)
///   N=2   → +20% per stack
///   N=3   → +30% per stack
///   N=4+  → normal (+50%)
class ScaledEncounters_FeralEssence : public AllCreatureScript
{
public:
    ScaledEncounters_FeralEssence()
        : AllCreatureScript("ScaledEncounters_FeralEssence")
    { }

    void OnAllCreatureUpdate(Creature* creature, uint32 /*diff*/) override
    {
        if (!creature || creature->GetEntry() != NPC_FERAL_DEFENDER)
            return;

        if (!creature->IsAlive() || !creature->IsInCombat())
            return;

        Aura* aura = creature->GetAura(SPELL_FERAL_ESSENCE_BUFF);
        if (!aura)
            return;

        uint32 N = GetGroupSize(creature);
        int32 pctPerStack = 0;
        if (N <= 1)
            pctPerStack = 10;
        else if (N == 2)
            pctPerStack = 20;
        else if (N == 3)
            pctPerStack = 30;

        if (!pctPerStack)
            return;

        // Effect 0: MOD_DAMAGE_PERCENT_DONE, default +50 per stack
        AuraEffect* dmgEffect = aura->GetEffect(EFFECT_0);
        if (!dmgEffect)
            return;

        int32 wanted = pctPerStack * aura->GetStackAmount();
        if (dmgEffect->GetAmount() != wanted)
            dmgEffect->ChangeAmount(wanted);
    }
};

/// Scale Sonic Screech damage at low player counts.  The spell's total damage
/// is split among all targets hit — in a normal raid 5-10 players share it,
/// but solo the full 60k (10-man) or 190k (25-man) hits one person.
///
/// Simulate the split by dividing damage as if more players were present:
///   N=1   → damage / 5  (as if 5 targets shared)
///   N=2   → damage / 3  (as if 6 targets shared)
///   N=3   → damage / 2  (as if 6 targets shared)
///   N=4+  → normal
class ScaledEncounters_AuriayaSonicScreech : public UnitScript
{
public:
    ScaledEncounters_AuriayaSonicScreech()
        : UnitScript("ScaledEncounters_AuriayaSonicScreech",
                      /*addToScripts=*/true,
                      { UNITHOOK_MODIFY_SPELL_DAMAGE_TAKEN })
    { }

    void ModifySpellDamageTaken(Unit* target, Unit* /*attacker*/, int32& damage, SpellInfo const* spellInfo) override
    {
        if (!spellInfo || !target || !target->IsPlayer())
            return;

        if (spellInfo->Id != SPELL_SONIC_SCREECH_10 && spellInfo->Id != SPELL_SONIC_SCREECH_25)
            return;

        uint32 N = GetGroupSize(target);
        if (N <= 1)
            damage /= 5;
        else if (N == 2)
            damage /= 3;
        else if (N == 3)
            damage /= 2;
    }
};

// ===========================  Hodir  =======================================

enum HodirSpells
{
    SPELL_ICICLE_BOSS_AURA          = 62227,  // periodic trigger every 2s
};

enum HodirNpcs
{
    NPC_HODIR                       = 32845,
};

/// Slow down the Icicle boss aura tick rate at low player counts.
/// The aura fires every 2 seconds and each tick targets a random player —
/// in a full raid the icicles spread across 10 people, but solo every tick
/// lands on the same player, creating non-stop movement with no time to DPS.
///
///   N=1   → 6s between icicles (3× slower)
///   N=2   → 4s between icicles (2× slower)
///   N=3-4 → 3s between icicles (1.5× slower)
///   N=5+  → normal (2s)
class ScaledEncounters_HodirIcicles : public UnitScript
{
public:
    ScaledEncounters_HodirIcicles()
        : UnitScript("ScaledEncounters_HodirIcicles",
                      /*addToScripts=*/true,
                      { UNITHOOK_ON_AURA_APPLY })
    { }

    void OnAuraApply(Unit* target, Aura* aura) override
    {
        if (!target || !aura || !target->IsCreature())
            return;

        if (aura->GetId() != SPELL_ICICLE_BOSS_AURA)
            return;

        if (target->ToCreature()->GetEntry() != NPC_HODIR)
            return;

        uint32 N = GetGroupSize(target);

        uint32 newAmplitude = 2000; // default
        if (N <= 1)
            newAmplitude = 6000;
        else if (N <= 2)
            newAmplitude = 4000;
        else if (N <= 4)
            newAmplitude = 3000;

        if (newAmplitude != 2000)
            if (AuraEffect* eff = aura->GetEffect(EFFECT_0))
                eff->SetAmplitude(newAmplitude);
    }
};

// ===========================  Freya  =======================================

enum FreyaSpells
{
    SPELL_UNSTABLE_GROWTH           = 200307, // custom: root + 1% HP/s regen
    SPELL_OVERWHELMED_GROWTH        = 200310, // custom: -75% damage done (lasher melee nerf)
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
        {
            creature->RemoveAura(SPELL_OVERWHELMED_GROWTH);
            return;
        }

        uint32 N = GetGroupSize(creature);
        if (N > 2)
        {
            creature->RemoveAura(SPELL_OVERWHELMED_GROWTH);
            return;
        }

        // Reduce melee damage — 10 lashers at full damage is overwhelming solo
        if (!creature->HasAura(SPELL_OVERWHELMED_GROWTH))
            creature->CastSpell(creature, SPELL_OVERWHELMED_GROWTH, true);

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
            if (Creature* creature = target->ToCreature())
                creature->SetSpeed(MOVE_RUN, creature->GetCreatureTemplate()->speed_run, true);
            return;
        }

        // 10% slower per stack: 1 stack = 90%, 5 stacks = 50%, 10 = 0% (rooted)
        float rate = std::max(0.0f, 1.0f - (stacks * 0.1f));
        if (Creature* creature = target->ToCreature())
            creature->SetSpeed(MOVE_RUN, creature->GetCreatureTemplate()->speed_run * rate, true);

        // Halve the damage increase effect (Effect 0: MOD_DAMAGE_PERCENT_DONE)
        if (AuraEffect* dmgEffect = aura->GetEffect(EFFECT_0))
        {
            // Default is +2% per stack; reduce to +1% per stack
            int32 reducedAmount = stacks; // 1% per stack instead of 2%
            dmgEffect->ChangeAmount(reducedAmount);
        }
    }
};

// ===========================  Thorim  =======================================

enum ThorimSoloConsts
{
    NPC_THORIM_BOSS                 = 32865,
    NPC_THORIM_LIGHTNING_ORB_NPC    = 33138,
    NPC_THORIM_WARBRINGER           = 32877,
    NPC_THORIM_EVOKER               = 32878,
    NPC_THORIM_CHAMPION             = 32876,
    NPC_THORIM_COMMONER             = 32904,

    SPELL_THORIM_BERSERK_FRIENDS    = 62560,
    SPELL_THORIM_SHEATH_LIGHTNING   = 62276,

    // Must match ACTION_FORCE_ARENA_PHASE2 in boss_thorim.cpp
    ACTION_THORIM_FORCE_P2          = 8,

    THORIM_SOLO_TIMER_MS            = 60000,  // 60s before easy-mode auto-P2
};

static bool IsArenaWaveNpc(uint32 entry)
{
    return entry == NPC_THORIM_WARBRINGER ||
           entry == NPC_THORIM_EVOKER     ||
           entry == NPC_THORIM_CHAMPION   ||
           entry == NPC_THORIM_COMMONER;
}

/// Solo Thorim controller.  At low player counts the encounter is redesigned:
///
///  **Easy mode (arena):**  If the player doesn't pull the corridor lever
///  within 60 seconds of Thorim's P1 starting, Thorim jumps down into the
///  arena for P2 without requiring the corridor gauntlet.  No Sif, no hard
///  mode.  Arena adds stay alive as part of the chaos.
///
///  **Hard mode (corridor):**  If the player pulls the lever, all arena wave
///  adds are despawned and no new ones spawn.  The player clears the corridor
///  gauntlet and triggers P2 normally from the ring.  Hard mode (Sif) is
///  determined by the original timer mechanic.
///
///  **Both modes:**  The arena-empty Lightning Orb wipe and the 5-minute
///  corridor-timeout berserk are disabled — a solo player can only be in one
///  place at a time.
class ScaledEncounters_ThorimSolo : public AllCreatureScript
{
public:
    ScaledEncounters_ThorimSolo()
        : AllCreatureScript("ScaledEncounters_ThorimSolo")
    { }

    void OnCreatureAddWorld(Creature* creature) override
    {
        if (!creature)
            return;

        Map* map = creature->GetMap();
        if (!map || !map->IsDungeon())
            return;

        uint32 entry = creature->GetEntry();

        // Despawn the Lightning Orb wipe mechanic for solo
        if (entry == NPC_THORIM_LIGHTNING_ORB_NPC && GetGroupSize(map) <= 1)
        {
            creature->DespawnOrUnsummon(Milliseconds(1));
            return;
        }

        // Despawn arena wave NPCs when the corridor path was chosen
        if (IsArenaWaveNpc(entry) && GetGroupSize(map) <= 1)
        {
            auto it = _state.find(creature->GetInstanceId());
            if (it != _state.end() && it->second.leverPulled)
                creature->DespawnOrUnsummon(Milliseconds(1));
        }
    }

    void OnAllCreatureUpdate(Creature* creature, uint32 diff) override
    {
        if (!creature || creature->GetEntry() != NPC_THORIM_BOSS)
            return;

        uint32 N = GetGroupSize(creature);
        if (N > 1)
        {
            _state.erase(creature->GetInstanceId());
            return;
        }

        // Out of combat → clean slate
        if (!creature->IsInCombat())
        {
            _state.erase(creature->GetInstanceId());
            return;
        }

        uint32 instId = creature->GetInstanceId();
        SoloState& state = _state[instId];

        // Detect lever pull (corridor path chosen)
        if (!state.leverPulled && !state.p2Forced)
        {
            if (InstanceScript* instance = creature->GetInstanceScript())
                if (GameObject* lever = instance->GetGameObject(501 /*DATA_THORIM_LEVER*/))
                    if (lever->GetGoState() == GO_STATE_ACTIVE)
                    {
                        state.leverPulled = true;
                        DespawnArenaWaveNpcs(creature);
                    }
        }

        // Corridor chosen or P2 already forced — nothing more to do
        if (state.leverPulled || state.p2Forced)
            return;

        // Only tick the auto-P2 timer while Thorim is in P1 (Sheath active)
        if (!creature->HasAura(SPELL_THORIM_SHEATH_LIGHTNING))
            return;

        state.p1Timer += diff;

        if (state.p1Timer >= THORIM_SOLO_TIMER_MS)
        {
            state.p2Forced = true;
            creature->AI()->DoAction(ACTION_THORIM_FORCE_P2);
        }
    }

private:
    struct SoloState
    {
        uint32 p1Timer      = 0;
        bool   leverPulled  = false;
        bool   p2Forced     = false;
    };

    std::unordered_map<uint32, SoloState> _state;

    void DespawnArenaWaveNpcs(Creature* thorim)
    {
        std::list<Creature*> found;
        thorim->GetCreatureListWithEntryInGrid(found, NPC_THORIM_WARBRINGER, 200.0f);
        thorim->GetCreatureListWithEntryInGrid(found, NPC_THORIM_EVOKER, 200.0f);
        thorim->GetCreatureListWithEntryInGrid(found, NPC_THORIM_CHAMPION, 200.0f);
        thorim->GetCreatureListWithEntryInGrid(found, NPC_THORIM_COMMONER, 200.0f);
        for (Creature* npc : found)
            npc->DespawnOrUnsummon(Milliseconds(1));
    }
};

/// Prevent the 5-minute corridor-timeout berserk for solo players.
/// The berserk fires when the corridor isn't cleared in time — impossible
/// to avoid when one player must simultaneously survive the arena.
class ScaledEncounters_ThorimBerserk : public UnitScript
{
public:
    ScaledEncounters_ThorimBerserk()
        : UnitScript("ScaledEncounters_ThorimBerserk",
                      /*addToScripts=*/true,
                      { UNITHOOK_ON_AURA_APPLY })
    { }

    void OnAuraApply(Unit* target, Aura* aura) override
    {
        if (!target || !aura)
            return;

        if (aura->GetId() != SPELL_THORIM_BERSERK_FRIENDS)
            return;

        if (GetGroupSize(target) <= 1)
            target->RemoveAura(SPELL_THORIM_BERSERK_FRIENDS);
    }
};

void AddSC_scaled_encounters_ulduar()
{
    new ScaledEncounters_RazorscaleFuseArmor();
    new ScaledEncounters_KologarnStacks();
    RegisterSpellScript(spell_scaled_kologarn_stone_grip);
    new ScaledEncounters_XT002Scrapbots();
    new ScaledEncounters_MolgeimRunes();
    new ScaledEncounters_OverwhelmingPower();
    new ScaledEncounters_StaticDisruption();
    new ScaledEncounters_BrundirTendrils();
    new ScaledEncounters_AssemblyMelee();
    new ScaledEncounters_AuriayaPull();
    new ScaledEncounters_FeralEssence();
    new ScaledEncounters_AuriayaSonicScreech();
    new ScaledEncounters_HodirIcicles();
    new ScaledEncounters_DetonatingLashers();
    new ScaledEncounters_SnaplasherBark();
    new ScaledEncounters_ThorimSolo();
    new ScaledEncounters_ThorimBerserk();
}
