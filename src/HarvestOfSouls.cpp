#include "ScriptMgr.h"
#include "Player.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "SpellScriptLoader.h"
#include "Unit.h"
#include "CellImpl.h"
#include "GridNotifiers.h"
#include "Log.h"

// Harvest of Souls (renamed On a Pale Horse, talent 2039)
// Death Strike applies a life-draining magic debuff to nearby diseased enemies.
// The drain ticks every 5s for PERIODIC_LEECH damage (~Blood Plague per tick),
// multiplied by the number of diseases on the target, and heals the caster.
// Rank 1: 50% chance, Rank 2: 100% chance.

enum HarvestOfSoulsSpells
{
    HARVEST_PASSIVE_R1 = 200129,
    HARVEST_PASSIVE_R2 = 200130,
    HARVEST_DISEASE    = 200131,
};

static constexpr float HARVEST_RANGE = 15.0f;

// Part A: SpellScript on Death Strike (registered via spell_script_names on -49998)
// After Death Strike hits, apply Harvest of Souls to nearby diseased enemies.
class spell_dk_harvest_of_souls_SpellScript : public SpellScript
{
    PrepareSpellScript(spell_dk_harvest_of_souls_SpellScript);

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ HARVEST_DISEASE, HARVEST_PASSIVE_R1, HARVEST_PASSIVE_R2 });
    }

    bool Load() override
    {
        _executed = false;
        return GetCaster()->IsPlayer();
    }

    void HandleAfterHit()
    {
        LOG_ERROR("spell.scripts", "HarvestOfSouls: HandleAfterHit called, _executed={}", _executed);

        if (_executed)
            return;

        Unit* caster = GetCaster();
        Unit* hitUnit = GetHitUnit();
        LOG_ERROR("spell.scripts", "HarvestOfSouls: caster={}, hitUnit={}", caster ? "yes" : "null", hitUnit ? "yes" : "null");

        if (!caster || !hitUnit)
            return;

        // Determine talent rank and chance from passive aura base points
        int32 chance = 0;
        bool hasR2 = caster->HasAura(HARVEST_PASSIVE_R2);
        bool hasR1 = caster->HasAura(HARVEST_PASSIVE_R1);
        LOG_ERROR("spell.scripts", "HarvestOfSouls: hasR1={}, hasR2={}", hasR1, hasR2);

        if (AuraEffect const* eff = caster->GetAuraEffect(HARVEST_PASSIVE_R2, EFFECT_0))
            chance = eff->GetAmount() + 1;  // basepoints=99 -> 100%
        else if (AuraEffect const* eff = caster->GetAuraEffect(HARVEST_PASSIVE_R1, EFFECT_0))
            chance = eff->GetAmount() + 1;  // basepoints=49 -> 50%

        LOG_ERROR("spell.scripts", "HarvestOfSouls: chance={}", chance);

        if (chance <= 0)
            return;

        _executed = true;

        if (!roll_chance_i(chance))
        {
            LOG_ERROR("spell.scripts", "HarvestOfSouls: chance roll failed");
            return;
        }

        // Find enemies within 15yd of caster that have diseases from this caster
        std::list<Unit*> enemies;
        Acore::AnyUnfriendlyUnitInObjectRangeCheck check(caster, caster, HARVEST_RANGE);
        Acore::UnitListSearcher<Acore::AnyUnfriendlyUnitInObjectRangeCheck> searcher(caster, enemies, check);
        Cell::VisitObjects(caster, searcher, HARVEST_RANGE);

        LOG_ERROR("spell.scripts", "HarvestOfSouls: found {} enemies in {}yd range", enemies.size(), HARVEST_RANGE);

        for (Unit* enemy : enemies)
        {
            if (!enemy->IsAlive())
                continue;

            if (!caster->IsValidAttackTarget(enemy))
                continue;

            uint32 diseases = enemy->GetDiseasesByCaster(caster->GetGUID());
            LOG_ERROR("spell.scripts", "HarvestOfSouls: enemy {} has {} diseases",
                enemy->GetName(), diseases);

            // Only apply to enemies that already have diseases from this caster
            if (diseases == 0)
                continue;

            LOG_ERROR("spell.scripts", "HarvestOfSouls: casting {} on {}", HARVEST_DISEASE, enemy->GetName());
            caster->CastSpell(enemy, HARVEST_DISEASE, true);
        }
    }

    void Register() override
    {
        AfterHit += SpellHitFn(spell_dk_harvest_of_souls_SpellScript::HandleAfterHit);
    }

    bool _executed;
};

class spell_dk_harvest_of_souls_loader : public SpellScriptLoader
{
public:
    spell_dk_harvest_of_souls_loader() : SpellScriptLoader("spell_dk_harvest_of_souls") { }

    SpellScript* GetSpellScript() const override
    {
        return new spell_dk_harvest_of_souls_SpellScript();
    }
};

// Part B: UnitScript to multiply Harvest of Souls tick damage by disease count.
// GetDiseasesByCaster only counts PERIODIC_DAMAGE and LINKED auras (not
// PERIODIC_LEECH), so Harvest of Souls does not count itself.
class spell_dk_harvest_of_souls_tick : public UnitScript
{
public:
    spell_dk_harvest_of_souls_tick()
        : UnitScript("spell_dk_harvest_of_souls_tick", true,
            { UNITHOOK_MODIFY_PERIODIC_DAMAGE_AURAS_TICK }) { }

    void ModifyPeriodicDamageAurasTick(Unit* target, Unit* attacker,
        uint32& damage, SpellInfo const* spellInfo) override
    {
        if (!attacker || !target || !spellInfo)
            return;

        if (spellInfo->Id != HARVEST_DISEASE)
            return;

        uint32 diseaseCount = target->GetDiseasesByCaster(attacker->GetGUID());
        if (diseaseCount > 1)
            damage *= diseaseCount;
    }
};

void AddSC_dk_harvest_of_souls()
{
    new spell_dk_harvest_of_souls_loader();
    new spell_dk_harvest_of_souls_tick();
}
