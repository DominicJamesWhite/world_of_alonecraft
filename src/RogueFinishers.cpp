#include "AlonecraftTestLog.h"
#include "CellImpl.h"
#include "GridNotifiers.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SpellAuraEffects.h"
#include "SpellAuras.h"
#include "SpellMgr.h"
#include "SpellScript.h"
#include "SpellScriptLoader.h"
#include "Unit.h"

#include <algorithm>
#include <iterator>

// Enveloping Shadows (31211 / 31212 / 31213) -- Alonecraft
//
// TODO.md:
//   "Enveloping Shadows: Your finishing moves have a 6/12/20% chance per
//    combo point spent to also cast Slice and Dice on yourself and Expose
//    Armor on nearby enemies."
//
// Heightened Senses used to share this script, back when it also triggered off
// finishers. It is now a dodge proc and lives in HeightenedSenses.cpp.
//
// THE SELF-BUFF TARGET TRAP: Slice and Dice (5171 / 6774) ships as an
// ENEMY-TARGETED spell even though the buff lands on the caster -- Effect1 is
// an inert DUMMY on TARGET_UNIT_TARGET_ENEMY and only Effect2 (the melee
// haste aura) uses TARGET_UNIT_CASTER.  That puts TARGET_FLAG_UNIT_ENEMY in
// ExplicitTargetMask, so CastSpell(player, snd, true) is rejected with
// SPELL_FAILED_BAD_TARGETS -- CheckCast runs even for triggered casts.
//
// AddAura builds the aura directly and skips the Spell object, so the target
// check never runs.  Do NOT "fix" this by casting at the finisher's victim
// instead: hit triggers fire after the damage lands (Spell.cpp:3244 vs
// :2866), so a killing blow leaves a dead target and _IsValidAttackTarget
// rejects the cast all over again.
//
// FULL STRENGTH IS THE WHOLE POINT: spending one combo point on a proc should
// hand back the combo points and energy that a manual Slice and Dice plus
// Expose Armor would have cost, so both are granted as though five points had
// been spent.  Neither happens by default -- the engine scales both off the
// points actually spent -- so both are corrected explicitly:
//
//   Slice and Dice  duration, via Aura::CalcMaxDuration -> CalcSpellDuration
//   Expose Armor    armour reduction, via EffectPointsPerComboPoint
//
// SPELLMOD_DURATION is still applied to the former, so Glyph of Slice and Dice
// is honoured on top.  Core's Assassination equivalent, Cut to the Chase
// (spell_rogue.cpp:858), overwrites the duration the same way, though it can
// only refresh an existing Slice and Dice, never apply a fresh one.
//
// THE COMBO POINT TRAP: Spell::_handle_finish_phase clears combo points
// (Spell.cpp:4243) before proc handling runs, so any AuraScript proc that
// calls GetComboPoints() reads 0.  They are captured in BeforeCast instead,
// which is also where the engine itself reads them for
// SPELL_AURA_ADD_TARGET_TRIGGER (SpellInfo.cpp:454).
//
// The talent's per-rank chance lives in its dummy aura's base points, read
// back with GetAuraEffect(...)->GetAmount(), so there is no spell-ID switch
// here and retuning is a DBC edit.

enum RogueFinisherSpells
{
    SPELL_ROGUE_ENVELOPING_SHADOWS_R1 = 31211,
    SPELL_ROGUE_ENVELOPING_SHADOWS_R2 = 31212,
    SPELL_ROGUE_ENVELOPING_SHADOWS_R3 = 31213,

    ENVELOPING_SHADOWS_RADIUS         = 10,

    // The point of the talent is to hand back a *full strength* Slice and
    // Dice and Expose Armor, so both are granted as though five combo points
    // had been spent however many actually were.  Core has no constant for
    // the cap; it is hardcoded in Unit::AddComboPoints (Unit.cpp:13295).
    FULL_COMBO_POINTS                 = 5,
};

// Highest rank first -- the first one the rogue knows is the one to cast.
static uint32 const SliceAndDiceRanks[] = { 6774, 5171 };
static uint32 const ExposeArmorRanks[]  = { 48669, 26866, 11198, 11197, 8650, 8649, 8647 };

static uint32 GetBestKnownRank(Player const* player, uint32 const* ranks, size_t count)
{
    for (size_t i = 0; i < count; ++i)
        if (player->HasSpell(ranks[i]))
            return ranks[i];

    return 0;
}

// Returns the known rank's chance-per-combo-point dummy amount, or 0 if
// untalented.
static int32 GetDummyAmount(Unit const* caster, uint32 const* talents, size_t count)
{
    for (size_t i = 0; i < count; ++i)
        if (AuraEffect const* eff = caster->GetAuraEffect(talents[i], EFFECT_0))
            return eff->GetAmount();

    return 0;
}

static uint32 const EnvelopingTalents[] =
{
    SPELL_ROGUE_ENVELOPING_SHADOWS_R3,
    SPELL_ROGUE_ENVELOPING_SHADOWS_R2,
    SPELL_ROGUE_ENVELOPING_SHADOWS_R1
};

class spell_rog_finishers : public SpellScript
{
    PrepareSpellScript(spell_rog_finishers);

    bool Load() override
    {
        _comboPoints = 0;
        return GetCaster()->IsPlayer();
    }

    void HandleBeforeCast()
    {
        // Must be read here: by the time the spell finishes they are gone.
        if (Player* player = GetCaster()->ToPlayer())
            _comboPoints = player->GetComboPoints();
    }

    // chance = per-combo-point rate * combo points spent, capped at 100.
    bool Roll(int32 ratePerPoint) const
    {
        if (ratePerPoint <= 0 || !_comboPoints)
            return false;

        int32 chance = ratePerPoint * int32(_comboPoints);
        return roll_chance_i(std::min<int32>(chance, 100));
    }

    // AfterHit rather than AfterCast: for a delayed finisher (Deadly Throw)
    // AfterCast fires while the missile is still in the air, so the talent
    // would not land with the strike that earned it.  Finishers are
    // single-target, so this still runs exactly once.
    void HandleAfterHit()
    {
        Player* player = GetCaster() ? GetCaster()->ToPlayer() : nullptr;
        if (!player || !_comboPoints)
            return;

        // Enveloping Shadows casts Slice and Dice and Expose Armor, both of
        // which this script is also registered on.  Those triggered casts
        // arrive with no combo points and stop at the check above, but
        // refusing triggered casts outright makes that independent of the
        // combo point bookkeeping.
        if (GetSpell()->IsTriggered())
            return;

        ACTEST("ROG.FINISH", "finisher={} comboPoints={}",
            GetSpellInfo()->Id, uint32(_comboPoints));

        HandleEnvelopingShadows(player);
    }

    void HandleEnvelopingShadows(Player* player)
    {
        int32 const rate = GetDummyAmount(player, EnvelopingTalents, std::size(EnvelopingTalents));
        if (!rate)
            return;

        if (!Roll(rate))
        {
            ACTEST("ROG.FINISH", "enveloping ratePerCp={} cp={} chance={} roll=fail",
                rate, uint32(_comboPoints), std::min<int32>(rate * _comboPoints, 100));
            return;
        }

        ACTEST("ROG.FINISH", "enveloping ratePerCp={} cp={} chance={} roll=PASS",
            rate, uint32(_comboPoints), std::min<int32>(rate * _comboPoints, 100));

        // AddAura, not CastSpell -- see THE SELF-BUFF TARGET TRAP above.
        if (uint32 snd = GetBestKnownRank(player, SliceAndDiceRanks, std::size(SliceAndDiceRanks)))
        {
            if (Aura* aura = player->AddAura(snd, player))
            {
                // Always the FULL_COMBO_POINTS duration -- GetMaxDuration()
                // reads DurationEntry->Duration[2], the 5 point ceiling.
                // Same two-step as Cut to the Chase (spell_rogue.cpp:865):
                // SetDuration first so withMods applies SPELLMOD_DURATION,
                // then read the modded value back into the max so the client
                // timer matches.
                aura->SetDuration(aura->GetSpellInfo()->GetMaxDuration(), true);
                aura->SetMaxDuration(aura->GetDuration());

                ACTEST("ROG.FINISH", "enveloping Slice and Dice rank={} present=yes duration={}",
                    snd, aura->GetDuration());
            }
            else
                ACTEST("ROG.FINISH", "enveloping Slice and Dice rank={} present=NO", snd);
        }

        uint32 const expose = GetBestKnownRank(player, ExposeArmorRanks, std::size(ExposeArmorRanks));
        if (!expose)
        {
            ACTEST("ROG.FINISH", "enveloping: rogue knows no Expose Armor rank");
            return;
        }

        Unit* target = GetExplTargetUnit();
        if (!target)
            return;

        // Expose Armor's armour reduction scales off combo points too --
        // 48669 has EffectPointsPerComboPoint1 = -785, so a 1 point finisher
        // would otherwise grant -785 armour instead of -3925.
        //
        // SpellEffectInfo::CalcValue (SpellInfo.cpp:428) substitutes a custom
        // base point value for BasePoints and only *then* adds
        // PointsPerComboPoint * comboPoints, so paying forward the points that
        // were NOT spent lands the engine on the 5 point value.  Feeding its
        // own two fields back to it keeps this correct for every rank: the old
        // percentage ranks have PointsPerComboPoint = 0, which makes the
        // adjustment a no-op.
        SpellInfo const* exposeInfo = sSpellMgr->GetSpellInfo(expose);
        if (!exposeInfo)
            return;

        SpellEffectInfo const& exposeEffect = exposeInfo->Effects[EFFECT_0];
        int32 const unspent = int32(FULL_COMBO_POINTS) - int32(_comboPoints);
        int32 exposeBp = exposeEffect.BasePoints
            + int32(exposeEffect.PointsPerComboPoint * float(unspent));

        // Same searcher idiom as FocusedPower.cpp:53.
        std::list<Unit*> enemies;
        Acore::AnyUnfriendlyUnitInObjectRangeCheck check(target, player, float(ENVELOPING_SHADOWS_RADIUS));
        Acore::UnitListSearcher<Acore::AnyUnfriendlyUnitInObjectRangeCheck> searcher(target, enemies, check);
        Cell::VisitObjects(target, searcher, float(ENVELOPING_SHADOWS_RADIUS));

        uint32 exposed = 0;
        for (Unit* enemy : enemies)
        {
            if (!player->IsValidAttackTarget(enemy))
                continue;

            player->CastCustomSpell(enemy, expose, &exposeBp, nullptr, nullptr, true);
            ++exposed;
        }

        ACTEST("ROG.FINISH",
            "enveloping Expose Armor rank={} cp={} bp={} radius={} candidates={} applied={}",
            expose, uint32(_comboPoints), exposeBp, int32(ENVELOPING_SHADOWS_RADIUS),
            uint32(enemies.size()), exposed);
    }

    void Register() override
    {
        BeforeCast += SpellCastFn(spell_rog_finishers::HandleBeforeCast);
        AfterHit   += SpellHitFn(spell_rog_finishers::HandleAfterHit);
    }

    uint8 _comboPoints;
};

class spell_rog_finishers_loader : public SpellScriptLoader
{
public:
    spell_rog_finishers_loader() : SpellScriptLoader("spell_rog_finishers") { }

    SpellScript* GetSpellScript() const override
    {
        return new spell_rog_finishers();
    }
};

void AddSC_rog_finishers()
{
    new spell_rog_finishers_loader();
}
