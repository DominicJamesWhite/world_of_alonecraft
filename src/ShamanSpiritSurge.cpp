#include "CellImpl.h"
#include "GridNotifiers.h"
#include "ScriptMgr.h"
#include "Player.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "SpellScriptLoader.h"
#include "Unit.h"
#include "GameTime.h"

// Spiritsurge (redesigned) — Shaman Enhancement talent
// Ranks: 30872 / 30873 (SPELL_AURA_DUMMY passives)
// Mechanic:
//   When Earthliving Weapon heals (proc spells 51945/51990/51997/51998/51999/52000),
//   if player has Spiritsurge talent, apply buff 200217 (2 charges, 15s). 8s ICD.
//
//   When shocks are cast with Spiritsurge buff 200217 active:
//     - Earth Shock: summon earth elemental for 6s (200218)
//     - Flame Shock: spread Flame Shock to enemies within 15yd of target
//     - Frost Shock: apply root (200219) on target for 3s
//   Each shock consumes a charge of 200217.

enum SpiritSurgeSpells
{
    SPIRITSURGE_R1         = 30872,
    SPIRITSURGE_R2         = 30873,
    SPIRITSURGE_BUFF       = 200217,
    EARTH_ELEMENTAL_SUMMON = 200218,
    FROST_SHOCK_ROOT       = 200219,
};

static constexpr Milliseconds SPIRITSURGE_ICD_MS = 8000ms;
static std::unordered_map<ObjectGuid, Milliseconds> s_lastProcTime;

static bool HasSpiritSurge(Unit const* caster)
{
    return caster->HasAura(SPIRITSURGE_R1) ||
           caster->HasAura(SPIRITSURGE_R2);
}

static void ConsumeCharge(Unit* caster)
{
    if (Aura* buff = caster->GetAura(SPIRITSURGE_BUFF))
    {
        if (buff->GetCharges() <= 1)
            buff->Remove();
        else
            buff->DropCharge();
    }
}

// Part 1: Earthliving heal proc — apply Spiritsurge buff
// Registered to: 51945, 51990, 51997, 51998, 51999, 52000
class spell_sha_spiritsurge_proc : public SpellScript
{
    PrepareSpellScript(spell_sha_spiritsurge_proc);

    void HandleAfterCast()
    {
        Unit* caster = GetCaster();
        if (!caster || !HasSpiritSurge(caster))
            return;

        Milliseconds now = GameTime::GetGameTimeMS();
        ObjectGuid guid = caster->GetGUID();
        auto it = s_lastProcTime.find(guid);
        if (it != s_lastProcTime.end() && (now - it->second) < SPIRITSURGE_ICD_MS)
            return;

        s_lastProcTime[guid] = now;
        caster->CastSpell(caster, SPIRITSURGE_BUFF, true);
    }

    void Register() override
    {
        AfterCast += SpellCastFn(spell_sha_spiritsurge_proc::HandleAfterCast);
    }
};

// Part 2a: Earth Shock — summon earth elemental
// Registered to: -8042 (all ranks)
class spell_sha_spiritsurge_earth : public SpellScript
{
    PrepareSpellScript(spell_sha_spiritsurge_earth);

    void HandleAfterCast()
    {
        Unit* caster = GetCaster();
        Unit* target = GetExplTargetUnit();
        if (!caster || !target || !caster->HasAura(SPIRITSURGE_BUFF))
            return;

        caster->CastSpell(target, EARTH_ELEMENTAL_SUMMON, true);
        ConsumeCharge(caster);
    }

    void Register() override
    {
        AfterCast += SpellCastFn(spell_sha_spiritsurge_earth::HandleAfterCast);
    }
};

// Part 2b: Flame Shock — spread to enemies within 15yd
// Registered to: -8050 (all ranks)
class spell_sha_spiritsurge_flame : public SpellScript
{
    PrepareSpellScript(spell_sha_spiritsurge_flame);

    void HandleAfterCast()
    {
        Unit* caster = GetCaster();
        Unit* target = GetExplTargetUnit();
        if (!caster || !target || !caster->HasAura(SPIRITSURGE_BUFF))
            return;

        uint32 flameShockId = GetSpellInfo()->Id;

        std::list<Unit*> enemies;
        Acore::AnyUnfriendlyUnitInObjectRangeCheck check(target, caster, 15.0f);
        Acore::UnitListSearcher<Acore::AnyUnfriendlyUnitInObjectRangeCheck> searcher(target, enemies, check);
        Cell::VisitObjects(target, searcher, 15.0f);

        for (Unit* enemy : enemies)
        {
            if (enemy == target)
                continue;

            if (!caster->IsValidAttackTarget(enemy))
                continue;

            // Skip if enemy already has Flame Shock from this caster
            if (enemy->GetAuraEffect(SPELL_AURA_PERIODIC_DAMAGE, SPELLFAMILY_SHAMAN,
                0x10000000, 0, 0, caster->GetGUID()))
                continue;

            caster->CastSpell(enemy, flameShockId, true);
        }

        ConsumeCharge(caster);
    }

    void Register() override
    {
        AfterCast += SpellCastFn(spell_sha_spiritsurge_flame::HandleAfterCast);
    }
};

// Part 2c: Frost Shock — apply root
// Registered to: -8056 (all ranks)
class spell_sha_spiritsurge_frost : public SpellScript
{
    PrepareSpellScript(spell_sha_spiritsurge_frost);

    void HandleAfterCast()
    {
        Unit* caster = GetCaster();
        Unit* target = GetExplTargetUnit();
        if (!caster || !target || !caster->HasAura(SPIRITSURGE_BUFF))
            return;

        caster->CastSpell(target, FROST_SHOCK_ROOT, true);
        ConsumeCharge(caster);
    }

    void Register() override
    {
        AfterCast += SpellCastFn(spell_sha_spiritsurge_frost::HandleAfterCast);
    }
};

class spell_sha_spiritsurge_proc_loader : public SpellScriptLoader
{
public:
    spell_sha_spiritsurge_proc_loader() : SpellScriptLoader("spell_sha_spiritsurge_proc") { }
    SpellScript* GetSpellScript() const override { return new spell_sha_spiritsurge_proc(); }
};

class spell_sha_spiritsurge_earth_loader : public SpellScriptLoader
{
public:
    spell_sha_spiritsurge_earth_loader() : SpellScriptLoader("spell_sha_spiritsurge_earth") { }
    SpellScript* GetSpellScript() const override { return new spell_sha_spiritsurge_earth(); }
};

class spell_sha_spiritsurge_flame_loader : public SpellScriptLoader
{
public:
    spell_sha_spiritsurge_flame_loader() : SpellScriptLoader("spell_sha_spiritsurge_flame") { }
    SpellScript* GetSpellScript() const override { return new spell_sha_spiritsurge_flame(); }
};

class spell_sha_spiritsurge_frost_loader : public SpellScriptLoader
{
public:
    spell_sha_spiritsurge_frost_loader() : SpellScriptLoader("spell_sha_spiritsurge_frost") { }
    SpellScript* GetSpellScript() const override { return new spell_sha_spiritsurge_frost(); }
};

void AddSC_sha_spiritsurge()
{
    new spell_sha_spiritsurge_proc_loader();
    new spell_sha_spiritsurge_earth_loader();
    new spell_sha_spiritsurge_flame_loader();
    new spell_sha_spiritsurge_frost_loader();
}
