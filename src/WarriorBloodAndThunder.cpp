#include "AlonecraftTestLog.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SpellAuraEffects.h"
#include "SpellAuras.h"
#include "SpellScript.h"
#include "SpellScriptLoader.h"
#include "Unit.h"
#include "Random.h"
#include "Util.h"

#include <vector>

// Blood and Thunder (was Improved Disarm) -- Warrior Protection talent
// Talent ranks: 12313 / 12804 -- woa_2026_08_09_15.sql
// Registered on -6343, i.e. every rank of Thunder Clap.
//
// TODO.md: "Improved Disarm (Renamed to Blood and Thunder) (1, 3): Redesigned.
//  When you Thunder Clap a target affected by Rend, you have a 50/100% chance
//  to Rend nearby enemies. (2 ranks)"
//
// Handled by DBC:
//   The talent is a bare SPELL_AURA_DUMMY carrying the per-rank chance, and
//   Thunder Clap's own targeting is untouched.  Nothing else.
//
// Handled here:
//   The spread.  There is no generic "copy this DoT to the other targets"
//   helper anywhere in core -- every spell that does it hand-rolls it -- so
//   this follows DK_Pestilence.cpp, the module's existing example.
//
// Why the target list is captured rather than acted on per target.
// OnEffectHitTarget fires once per victim in an unspecified order, and this
// effect needs to know about ALL the victims before it can decide anything:
// the donor (whoever already has your Rend) might be hit last.  So the list is
// snapshotted during target selection and the work happens in AfterCast, which
// runs exactly once.
//
// GUIDs, not pointers, for the snapshot.  Target selection and AfterCast are
// the same tick so raw Unit* would very likely survive, but "very likely" is
// not a reason to hold a raw pointer across a phase boundary in a script that
// also casts spells.
//
// The donor's Rend is re-cast by ID rather than a fixed rank being applied.
// GetAuraOfRankedSpell(772, casterGUID) finds whichever rank the warrior
// actually applied, so the copies match the original and keep working when a
// higher rank is learned.  The amount is not copied: core's spell_warr_rend
// (src/server/scripts/Spells/spell_warrior.cpp:564) recalculates the periodic
// damage from the weapon on every application, which is what should happen --
// a snapshotted amount would be wrong the moment the warrior swapped weapons.
//
// One roll per cast, not one per target.  "You have a 50/100% chance to Rend
// nearby enemies" is a single outcome; rolling per victim would make rank 1
// spread to about half the pack every time instead of to the whole pack half
// the time.

enum BloodAndThunderSpells
{
    SPELL_WARRIOR_REND_RANK_1            = 772,
    SPELL_WARRIOR_BLOOD_AND_THUNDER_R1   = 12313,
    SPELL_WARRIOR_BLOOD_AND_THUNDER_R2   = 12804
};

class spell_warr_blood_and_thunder : public SpellScript
{
    PrepareSpellScript(spell_warr_blood_and_thunder);

    void CaptureTargets(std::list<WorldObject*>& targets)
    {
        _hit.clear();
        _hit.reserve(targets.size());

        for (WorldObject* obj : targets)
            if (Unit* unit = obj->ToUnit())
                _hit.push_back(unit->GetGUID());
    }

    void SpreadRend()
    {
        Unit* caster = GetCaster();
        if (!caster || _hit.size() < 2)
            return;

        // The chance is the talent's dummy amount.  Rank 2 is checked first so
        // that a warrior who somehow carries both takes the higher one.
        int32 chance = 0;
        if (AuraEffect const* r2 = caster->GetAuraEffect(SPELL_WARRIOR_BLOOD_AND_THUNDER_R2, EFFECT_0))
            chance = r2->GetAmount();
        else if (AuraEffect const* r1 = caster->GetAuraEffect(SPELL_WARRIOR_BLOOD_AND_THUNDER_R1, EFFECT_0))
            chance = r1->GetAmount();

        if (chance <= 0 || !roll_chance_i(chance))
            return;

        // Find whoever is already carrying our Rend -- that is the "target
        // affected by Rend" the talent asks about.
        Aura* donor = nullptr;
        ObjectGuid donorGuid;
        for (ObjectGuid const& guid : _hit)
        {
            Unit* unit = ObjectAccessor::GetUnit(*caster, guid);
            if (!unit)
                continue;

            if (Aura* rend = unit->GetAuraOfRankedSpell(SPELL_WARRIOR_REND_RANK_1, caster->GetGUID()))
            {
                donor = rend;
                donorGuid = guid;
                break;
            }
        }

        if (!donor)
            return;

        uint32 const rendId = donor->GetId();
        uint32 spread = 0;

        for (ObjectGuid const& guid : _hit)
        {
            if (guid == donorGuid)
                continue;

            Unit* unit = ObjectAccessor::GetUnit(*caster, guid);
            if (!unit || !unit->IsAlive())
                continue;

            // Never break someone else's crowd control -- Rend is a damage
            // over time effect, so it would.
            if (unit->HasBreakableByDamageCrowdControlAura())
                continue;

            caster->CastSpell(unit, rendId, true);
            ++spread;
        }

        ACTEST("WAR.BLOODTHUNDER", "chance={} rend={} donor={} hit={} spread={}",
            chance, rendId, donorGuid.ToString(), uint32(_hit.size()), spread);
    }

    void Register() override
    {
        OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(
            spell_warr_blood_and_thunder::CaptureTargets, EFFECT_0, TARGET_UNIT_DEST_AREA_ENEMY);
        AfterCast += SpellCastFn(spell_warr_blood_and_thunder::SpreadRend);
    }

    std::vector<ObjectGuid> _hit;
};

class spell_warr_blood_and_thunder_loader : public SpellScriptLoader
{
public:
    spell_warr_blood_and_thunder_loader() : SpellScriptLoader("spell_warr_blood_and_thunder") { }

    SpellScript* GetSpellScript() const override
    {
        return new spell_warr_blood_and_thunder();
    }
};

void AddSC_war_blood_and_thunder()
{
    new spell_warr_blood_and_thunder_loader();
}
