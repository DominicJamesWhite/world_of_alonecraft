#include "AlonecraftTestLog.h"
#include "CellImpl.h"
#include "GridNotifiers.h"
#include "ScriptMgr.h"
#include "Player.h"
#include "SpellAuraEffects.h"
#include "SpellScript.h"
#include "SpellScriptLoader.h"
#include "Unit.h"

// Ruin (redesigned) -- Warlock Destruction talent
// Ranks: 17959 / 59738 / 59739 / 59740 / 59741
//
// "No longer affects Imp. Instead critical hits on targets with Immolate
//  active spread Immolate to nearby targets (so long as those targets aren't
//  CCed with something that breaks on damage)."
//
// Handled by DBC:
//   Effect1: ADD_PCT_MODIFIER crit damage bonus, with EffectSpellClassMaskA1
//            narrowed from 5093 to 997 -- that drops Firebolt's 0x1000 bit,
//            which is the "no longer affects Imp" half.
//   Effect2: SPELL_AURA_DUMMY carrying the spread radius in BasePoints.
//
// Handled by spell_proc:
//   HitMask = PROC_HIT_CRITICAL, Cooldown = 1000.  The ICD matters: without
//   it a Rain of Fire tick storm would re-run the spread on every tick.
//
// Handled by this script:
//   The spread itself -- find the victim's Immolate (preserving its rank),
//   then re-apply it to nearby valid enemies that are not crowd-controlled
//   by something damage would break.
//
// Modelled on FocusedPower.cpp (Mind Sear -> SW:P spread).

enum RuinSpells
{
    RUIN_R1 = 17959,
};

// Immolate's Warlock (family 5) SpellFamilyFlags word 0.
static constexpr uint32 IMMOLATE_FAMILY_MASK0 = 0x4;

// Bound on how many extra targets one crit can seed.  Keeps AoE packs from
// turning a single crit into a dozen extra casts.
static constexpr uint32 RUIN_MAX_SPREAD_TARGETS = 4;

class spell_warl_ruin_immolate_spread : public AuraScript
{
    PrepareAuraScript(spell_warl_ruin_immolate_spread);

    bool CheckProc(ProcEventInfo& eventInfo)
    {
        return eventInfo.GetActionTarget() != nullptr;
    }

    void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();

        Unit* caster = GetTarget();
        Unit* victim = eventInfo.GetActionTarget();
        if (!caster || !victim || caster == victim)
            return;

        // The crit target must be carrying OUR Immolate; the rank is
        // preserved so the spread copies keep the caster's snapshot.
        AuraEffect* immolate = victim->GetAuraEffect(
            SPELL_AURA_PERIODIC_DAMAGE, SPELLFAMILY_WARLOCK,
            IMMOLATE_FAMILY_MASK0, 0, 0, caster->GetGUID());

        if (!immolate)
        {
            ACTEST("WARL.RUIN", "crit on {} but NO immolate of ours -- no spread",
                Alonecraft::TestLog::N(victim));
            return;
        }

        uint32 const immolateSpellId = immolate->GetBase()->GetId();

        float radius = static_cast<float>(aurEff->GetAmount());
        if (radius <= 0.0f)
        {
            ACTEST("WARL.RUIN", "radius={} is not positive -- Effect1 basepoints wrong?", radius);
            return;
        }

        std::list<Unit*> enemies;
        Acore::AnyUnfriendlyUnitInObjectRangeCheck check(victim, caster, radius);
        Acore::UnitListSearcher<Acore::AnyUnfriendlyUnitInObjectRangeCheck> searcher(victim, enemies, check);
        Cell::VisitObjects(victim, searcher, radius);

        uint32 spread = 0;
        uint32 skippedCc = 0;
        uint32 skippedHasImmolate = 0;
        uint32 skippedInvalid = 0;

        for (Unit* enemy : enemies)
        {
            if (spread >= RUIN_MAX_SPREAD_TARGETS)
                break;

            if (enemy == victim)
                continue;

            if (!caster->IsValidAttackTarget(enemy))
            {
                ++skippedInvalid;
                continue;
            }

            // Never break someone else's crowd control.
            if (enemy->HasBreakableByDamageCrowdControlAura())
            {
                ++skippedCc;
                ACTEST("WARL.RUIN", "skip {} reason=breakable-cc", Alonecraft::TestLog::N(enemy));
                continue;
            }

            // Don't clip an existing Immolate of ours.
            if (enemy->GetAuraEffect(SPELL_AURA_PERIODIC_DAMAGE, SPELLFAMILY_WARLOCK,
                IMMOLATE_FAMILY_MASK0, 0, 0, caster->GetGUID()))
            {
                ++skippedHasImmolate;
                continue;
            }

            caster->CastSpell(enemy, immolateSpellId, true);
            ++spread;

            ACTEST("WARL.RUIN", "spread immolate={} to={}", immolateSpellId,
                Alonecraft::TestLog::N(enemy));
        }

        ACTEST("WARL.RUIN",
            "crit source={} victim={} immolate={} radius={} candidates={} spread={} "
            "cap={} skippedCc={} skippedHasImmolate={} skippedInvalid={}",
            Alonecraft::TestLog::N(caster), Alonecraft::TestLog::N(victim), immolateSpellId,
            radius, uint32(enemies.size()), spread, RUIN_MAX_SPREAD_TARGETS,
            skippedCc, skippedHasImmolate, skippedInvalid);
    }

    void Register() override
    {
        DoCheckProc += AuraCheckProcFn(spell_warl_ruin_immolate_spread::CheckProc);
        OnEffectProc += AuraEffectProcFn(spell_warl_ruin_immolate_spread::HandleProc, EFFECT_1, SPELL_AURA_DUMMY);
    }
};

class spell_warl_ruin_immolate_spread_loader : public SpellScriptLoader
{
public:
    spell_warl_ruin_immolate_spread_loader() : SpellScriptLoader("spell_warl_ruin_immolate_spread") { }

    AuraScript* GetAuraScript() const override
    {
        return new spell_warl_ruin_immolate_spread();
    }
};

void AddSC_warl_ruin()
{
    new spell_warl_ruin_immolate_spread_loader();
}
