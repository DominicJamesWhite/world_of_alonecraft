#include "CreatureScript.h"
#include "Player.h"
#include "ScriptedCreature.h"
#include "SpellAuraEffects.h"

// Nature's Guardian elemental pet AI scripts
// These "lesser" guardians are summoned by totems when the shaman has
// Nature's Guardian talent (30881/30883/30884/30885/30886).
//
// Because the guardian is owned by the totem (not the player), Pet.cpp's
// InitStatsForLevel() doesn't recognize our custom entry IDs and won't
// set weapon damage or apply scaling auras. We handle that here in
// InitializeAI() instead.
//
// Damage scales with player spellpower * talent rank percentage:
//   Rank 1 = 20%, Rank 2 = 40%, Rank 3 = 60%, Rank 4 = 80%, Rank 5 = 100%
// Formula: damage = petlevel + spellpower * pct/100 * COEFFICIENT
// COEFFICIENT is a tuning knob (0.15 = 15% of scaled spellpower).

static constexpr float SP_COEFFICIENT = 0.15f;

enum NaturesGuardianPetSpells
{
    // Talent ranks
    NATURES_GUARDIAN_R1             = 30881,
    NATURES_GUARDIAN_R2             = 30883,
    NATURES_GUARDIAN_R3             = 30884,
    NATURES_GUARDIAN_R4             = 30885,
    NATURES_GUARDIAN_R5             = 30886,

    // SPELL_PET_AVOIDANCE (32233) and SPELL_HUNTER_PET_SCALING_04 (61017)
    // are defined in PetDefines.h — use them directly.

    // Fire Guardian abilities (same as Greater Fire Elemental)
    SPELL_FIRE_NOVA                 = 12470,
    SPELL_FIRE_BLAST                = 57984,
    SPELL_FIRE_SHIELD               = 13377,

    // Earth Guardian abilities (same as Greater Earth Elemental)
    SPELL_ANGERED_EARTH             = 36213,

    // Water Guardian abilities
    SPELL_WATERBOLT                 = 31707,

    // Air Guardian abilities
    SPELL_LIGHTNING_BOLT            = 57780,  // NPC Lightning Bolt
};

enum GuardianEvents
{
    EVENT_FIRE_NOVA     = 1,
    EVENT_FIRE_BLAST    = 2,
    EVENT_ANGERED_EARTH = 1,
    EVENT_WATERBOLT     = 1,
    EVENT_LIGHTNING_BOLT = 1,
};

// Owner is the player (not the totem) because triggered spells resolve
// ownership up the chain. Use GetOwner()->ToPlayer() directly.
static Player* GetOwnerPlayer(Creature const* guardian)
{
    if (Unit* owner = guardian->GetOwner())
        return owner->ToPlayer();
    return nullptr;
}

// Maps guardian creature entry -> totem summon slot
static uint32 GetTotemSlotForEntry(uint32 entry)
{
    switch (entry)
    {
        case 200001: return SUMMON_SLOT_TOTEM_FIRE;
        case 200000: return SUMMON_SLOT_TOTEM_EARTH;
        case 200002: return SUMMON_SLOT_TOTEM_WATER;
        case 200003: return SUMMON_SLOT_TOTEM_AIR;
        default:     return 0;
    }
}

// Returns the spellpower scaling percentage (20-100) from the talent aura's
// Effect0 amount. BasePoints is N-1, GetAmount() returns N-1, so we add 1.
static int32 GetSpellpowerPct(Player const* player)
{
    static constexpr uint32 ranks[] = {
        NATURES_GUARDIAN_R5, NATURES_GUARDIAN_R4, NATURES_GUARDIAN_R3,
        NATURES_GUARDIAN_R2, NATURES_GUARDIAN_R1
    };
    for (uint32 rankSpell : ranks)
        if (AuraEffect const* eff = player->GetAuraEffect(rankSpell, EFFECT_0))
            return eff->GetAmount(); // DieSides=1, so Amount = BasePoints+1 = 20/40/60/80/100
    return 20; // fallback
}

// Looks up the totem GUID from the player's summon slots at spawn time,
// then checks each tick if that specific totem is still alive.
static ObjectGuid FindTotemGuid(Creature const* me)
{
    Player* player = GetOwnerPlayer(me);
    if (!player)
        return ObjectGuid::Empty;

    uint32 slot = GetTotemSlotForEntry(me->GetEntry());
    if (!slot)
        return ObjectGuid::Empty;

    return player->m_SummonSlot[slot];
}

static bool ShouldDespawn(Creature* me, ObjectGuid totemGuid)
{
    if (totemGuid.IsEmpty())
    {
        me->DespawnOrUnsummon();
        return true;
    }

    Creature* totem = ObjectAccessor::GetCreature(*me, totemGuid);
    if (!totem || !totem->IsAlive() || !totem->IsInWorld())
    {
        me->DespawnOrUnsummon();
        return true;
    }
    return false;
}

static void ApplyGuardianScaling(Creature* me)
{
    Player* player = GetOwnerPlayer(me);
    if (!player)
        return;

    int32 pct = GetSpellpowerPct(player);
    int32 spellpower = player->SpellBaseDamageBonusDone(SPELL_SCHOOL_MASK_ALL);
    uint32 petlevel = me->GetLevel();

    float baseDmg = float(petlevel) + spellpower * (pct / 100.0f) * SP_COEFFICIENT;
    me->SetBaseWeaponDamage(BASE_ATTACK, MINDAMAGE, baseDmg * 0.85f);
    me->SetBaseWeaponDamage(BASE_ATTACK, MAXDAMAGE, baseDmg * 1.15f);

    me->AddAura(SPELL_PET_AVOIDANCE, me);
    me->AddAura(SPELL_HUNTER_PET_SCALING_04, me);

    me->UpdateAllStats();
}

// ============================================================
// Fire Guardian (entry 200001)
// Abilities: Fire Nova, Fire Blast, Fire Shield
// ============================================================
struct npc_natures_guardian_fire : public ScriptedAI
{
    npc_natures_guardian_fire(Creature* creature)
        : ScriptedAI(creature), _initAttack(true)
    {
        _totemGuid = FindTotemGuid(me);
        ApplyGuardianScaling(me);
    }

    void InitializeAI() override { }

    void JustEngagedWith(Unit*) override
    {
        _events.Reset();
        _events.ScheduleEvent(EVENT_FIRE_NOVA, 5s, 10s);
        _events.ScheduleEvent(EVENT_FIRE_BLAST, 3s, 6s);

        me->RemoveAurasDueToSpell(SPELL_FIRE_SHIELD);
        me->CastSpell(me, SPELL_FIRE_SHIELD, true);
    }

    void UpdateAI(uint32 diff) override
    {
        if (ShouldDespawn(me, _totemGuid))
            return;

        if (_initAttack)
        {
            if (!me->IsInCombat())
                if (Player* owner = GetOwnerPlayer(me))
                    if (Unit* target = owner->GetSelectedUnit())
                        if (me->CanCreatureAttack(target))
                            AttackStart(target);
            _initAttack = false;
        }

        if (!UpdateVictim())
            return;

        _events.Update(diff);
        while (uint32 eventId = _events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_FIRE_NOVA:
                    me->CastSpell(me, SPELL_FIRE_NOVA, false);
                    _events.ScheduleEvent(EVENT_FIRE_NOVA, 8s, 15s);
                    break;
                case EVENT_FIRE_BLAST:
                    me->CastSpell(me->GetVictim(), SPELL_FIRE_BLAST, false);
                    _events.ScheduleEvent(EVENT_FIRE_BLAST, 4s, 8s);
                    break;
            }
        }

        DoMeleeAttackIfReady();
    }

private:
    EventMap _events;
    ObjectGuid _totemGuid;
    bool _initAttack;
};

// ============================================================
// Earth Guardian (entry 200000)
// Abilities: Angered Earth
// ============================================================
struct npc_natures_guardian_earth : public ScriptedAI
{
    npc_natures_guardian_earth(Creature* creature)
        : ScriptedAI(creature), _initAttack(true)
    {
        _totemGuid = FindTotemGuid(me);
        ApplyGuardianScaling(me);
    }

    void InitializeAI() override { }

    void JustEngagedWith(Unit*) override
    {
        _events.Reset();
        _events.ScheduleEvent(EVENT_ANGERED_EARTH, 0ms);
    }

    void UpdateAI(uint32 diff) override
    {
        if (ShouldDespawn(me, _totemGuid))
            return;

        if (_initAttack)
        {
            if (!me->IsInCombat())
                if (Player* owner = GetOwnerPlayer(me))
                    if (Unit* target = owner->GetSelectedUnit())
                        if (me->CanCreatureAttack(target))
                            AttackStart(target);
            _initAttack = false;
        }

        if (!UpdateVictim())
            return;

        _events.Update(diff);
        if (_events.ExecuteEvent() == EVENT_ANGERED_EARTH)
        {
            DoCastVictim(SPELL_ANGERED_EARTH);
            _events.ScheduleEvent(EVENT_ANGERED_EARTH, 5s, 20s);
        }

        DoMeleeAttackIfReady();
    }

private:
    EventMap _events;
    ObjectGuid _totemGuid;
    bool _initAttack;
};

// ============================================================
// Water Guardian (entry 200002)
// Abilities: Waterbolt (ranged caster)
// ============================================================
struct npc_natures_guardian_water : public ScriptedAI
{
    npc_natures_guardian_water(Creature* creature)
        : ScriptedAI(creature), _initAttack(true)
    {
        _totemGuid = FindTotemGuid(me);
        ApplyGuardianScaling(me);
    }

    void InitializeAI() override { }

    void JustEngagedWith(Unit*) override
    {
        _events.Reset();
        _events.ScheduleEvent(EVENT_WATERBOLT, 0ms);
    }

    void UpdateAI(uint32 diff) override
    {
        if (ShouldDespawn(me, _totemGuid))
            return;

        if (_initAttack)
        {
            if (!me->IsInCombat())
                if (Player* owner = GetOwnerPlayer(me))
                    if (Unit* target = owner->GetSelectedUnit())
                        if (me->CanCreatureAttack(target))
                            AttackStart(target);
            _initAttack = false;
        }

        if (!UpdateVictim())
            return;

        _events.Update(diff);
        if (_events.ExecuteEvent() == EVENT_WATERBOLT)
        {
            me->CastSpell(me->GetVictim(), SPELL_WATERBOLT, false);
            _events.ScheduleEvent(EVENT_WATERBOLT, 4s, 6s);
        }

        DoMeleeAttackIfReady();
    }

private:
    EventMap _events;
    ObjectGuid _totemGuid;
    bool _initAttack;
};

// ============================================================
// Air Guardian (entry 200003)
// Abilities: Lightning Bolt (ranged caster)
// ============================================================
struct npc_natures_guardian_air : public ScriptedAI
{
    npc_natures_guardian_air(Creature* creature)
        : ScriptedAI(creature), _initAttack(true)
    {
        _totemGuid = FindTotemGuid(me);
        ApplyGuardianScaling(me);
    }

    void InitializeAI() override { }

    void JustEngagedWith(Unit*) override
    {
        _events.Reset();
        _events.ScheduleEvent(EVENT_LIGHTNING_BOLT, 0ms);
    }

    void UpdateAI(uint32 diff) override
    {
        if (ShouldDespawn(me, _totemGuid))
            return;

        if (_initAttack)
        {
            if (!me->IsInCombat())
                if (Player* owner = GetOwnerPlayer(me))
                    if (Unit* target = owner->GetSelectedUnit())
                        if (me->CanCreatureAttack(target))
                            AttackStart(target);
            _initAttack = false;
        }

        if (!UpdateVictim())
            return;

        _events.Update(diff);
        if (_events.ExecuteEvent() == EVENT_LIGHTNING_BOLT)
        {
            me->CastSpell(me->GetVictim(), SPELL_LIGHTNING_BOLT, false);
            _events.ScheduleEvent(EVENT_LIGHTNING_BOLT, 4s, 6s);
        }

        DoMeleeAttackIfReady();
    }

private:
    EventMap _events;
    ObjectGuid _totemGuid;
    bool _initAttack;
};

void AddSC_natures_guardian_pets()
{
    RegisterCreatureAI(npc_natures_guardian_fire);
    RegisterCreatureAI(npc_natures_guardian_earth);
    RegisterCreatureAI(npc_natures_guardian_water);
    RegisterCreatureAI(npc_natures_guardian_air);
}
