#include "AlonecraftTestLog.h"
#include "Item.h"
#include "ItemTemplate.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SpellAuraEffects.h"
#include "SpellAuras.h"
#include "SpellScript.h"
#include "SpellScriptLoader.h"
#include "Unit.h"

// Bladework (14185, was Preparation) -- Alonecraft
//
// TODO.md: "Redesigned. AOE combo point spender, 30 energy. Nearby enemies
// are marked for rapid counterattack, causing their next three attacks on
// you to be immediately countered for 125% weapon damage (180% if a dagger
// is equipped). 30s debuff duration."
//
// Three pieces:
//   14185   the cast -- a dummy aimed at everything within 10 yards, which
//           applies the mark to each unit it hits
//   200504  Marked for Counterattack -- sits on the ENEMY, one charge per
//           combo point spent
//   200505  Counterattack -- the weapon damage, cast by the rogue
//
// The mark being on the enemy is what makes this work: the enemy attacking
// is the proc event, and inside the proc eventInfo.GetActor() is the enemy
// while GetActionTarget() is the rogue.  Same arrangement as Mark of Blood
// (MarkOfBloodRework.cpp:24).
//
// Avoided attacks count too: the spell_proc row spells out HitMask 1075
// (NORMAL | CRITICAL | DODGE | PARRY | ABSORB), so a dodge or a parry
// counters just like a landed blow.  Leaving HitMask at 0 would fall back to
// the DONE-proc default of NORMAL | CRITICAL | ABSORB (SpellMgr.cpp:931) and
// silently drop both.  Misses are excluded on purpose -- that is the
// attacker whiffing, not the rogue defending.
//
// Charges scale with combo points: the spell_proc row's Charges is the cap
// (5), and HandleDummy overwrites the applied aura's charges with the
// points actually spent.  Combo points survive until _handle_finish_phase
// (Spell.cpp:4247), which runs after effect handling, so GetComboPoints()
// still reads the spent total inside the effect handler.

enum BladeworkSpells
{
    SPELL_ROGUE_BLADEWORK_MARK    = 200504,
    SPELL_ROGUE_COUNTERATTACK     = 200505,

    // 125% base, 180% with a dagger -- the DBC carries the 125%, so this is
    // the multiplier applied on top.
    COUNTERATTACK_DAGGER_PCT      = 180,
    COUNTERATTACK_BASE_PCT        = 125,
};

// The cast: mark everything it lands on.
class spell_rog_bladework : public SpellScript
{
    PrepareSpellScript(spell_rog_bladework);

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_ROGUE_BLADEWORK_MARK });
    }

    bool Load() override
    {
        return GetCaster()->IsPlayer();
    }

    void HandleDummy(SpellEffIndex /*effIndex*/)
    {
        Unit* caster = GetCaster();
        Unit* target = GetHitUnit();
        if (!caster || !target)
            return;

        // Read the points before the mark goes up.  The no-argument overload
        // ignores the combo target (Unit.h:1014), which matters here -- this
        // is an AoE and the points may be parked on a different mob.  A
        // finishing move cannot be cast at 0, but clamp anyway so a mark is
        // never charge-less.
        uint8 points = caster->GetComboPoints();
        if (!points)
            points = 1;

        caster->CastSpell(target, SPELL_ROGUE_BLADEWORK_MARK, true);

        // Set after the cast: re-marking a target that already carries the
        // mark refreshes the aura, and the refresh resets charges to the
        // spell_proc cap (SpellAuras.cpp:992).
        Aura* mark = target->GetAura(SPELL_ROGUE_BLADEWORK_MARK, caster->GetGUID());
        if (mark)
            mark->SetCharges(points);

        ACTEST("ROG.BLADEWORK", "marked {} comboPoints={} charges={} dur={}",
            Alonecraft::TestLog::N(target), uint32(points),
            mark ? uint32(mark->GetCharges()) : 0u,
            mark ? mark->GetDuration() : 0);
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_rog_bladework::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
    }
};

// The mark: the enemy attacked, so counter it.
class spell_rog_bladework_mark : public AuraScript
{
    PrepareAuraScript(spell_rog_bladework_mark);

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_ROGUE_COUNTERATTACK });
    }

    bool CheckProc(ProcEventInfo& eventInfo)
    {
        Unit* enemy = eventInfo.GetActor();
        Unit* rogue = eventInfo.GetActionTarget();
        if (!enemy || !rogue || enemy == rogue)
            return false;

        // Only the rogue who placed the mark counters, and only attacks
        // aimed at that rogue count.
        if (rogue->GetGUID() != GetCasterGUID())
            return false;

        // Same guard Retaliation uses (spell_warrior.cpp:874) -- no
        // countering while stunned or while the attacker is behind you.
        bool const inFront = rogue->isInFront(enemy, float(M_PI));
        bool const stunned = rogue->HasUnitState(UNIT_STATE_STUNNED);

        if (!inFront || stunned)
        {
            ACTEST("ROG.BLADEWORK", "attack from {} NOT countered inFront={} stunned={}",
                Alonecraft::TestLog::N(enemy), inFront, stunned);
            return false;
        }

        return true;
    }

    void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();

        Unit* enemy = eventInfo.GetActor();
        Unit* rogue = eventInfo.GetActionTarget();
        if (!enemy || !rogue)
            return;

        Aura const* mark = enemy->GetAura(SPELL_ROGUE_BLADEWORK_MARK, rogue->GetGUID());

        // hitMask distinguishes a landed blow from a dodge (0x10) or a parry
        // (0x20) -- all three counter, and the log is the only way to tell
        // which path fired.
        ACTEST("ROG.BLADEWORK", "countering {} chargesBefore={} hitMask={:#x}",
            Alonecraft::TestLog::N(enemy), mark ? uint32(mark->GetCharges()) : 0u,
            eventInfo.GetHitMask());

        rogue->CastSpell(enemy, SPELL_ROGUE_COUNTERATTACK, true, nullptr, aurEff);
    }

    void Register() override
    {
        DoCheckProc += AuraCheckProcFn(spell_rog_bladework_mark::CheckProc);
        OnEffectProc += AuraEffectProcFn(spell_rog_bladework_mark::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
    }
};

// The counter itself: 125% weapon damage, 180% with a dagger.
//
// 200505 carries SpellFamilyName 0 on purpose.  SpellEffects.cpp:3422 gives
// rogue-family spells matching 0x6000000 a flat +50% with a dagger, which
// would land on 187.5% instead of the 180% asked for, so the dagger branch
// is owned here instead.
class spell_rog_counterattack : public SpellScript
{
    PrepareSpellScript(spell_rog_counterattack);

    void HandleDamage(SpellEffIndex /*effIndex*/)
    {
        Player* player = GetCaster() ? GetCaster()->ToPlayer() : nullptr;
        if (!player)
            return;

        Item* mainHand = player->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_MAINHAND);
        if (!mainHand)
            return;

        ItemTemplate const* proto = mainHand->GetTemplate();
        if (!proto || proto->SubClass != ITEM_SUBCLASS_WEAPON_DAGGER)
        {
            ACTEST("ROG.BLADEWORK", "counterattack damage={} weapon=non-dagger pct={}",
                GetHitDamage(), int32(COUNTERATTACK_BASE_PCT));
            return;
        }

        int32 const before = GetHitDamage();
        SetHitDamage(before * COUNTERATTACK_DAGGER_PCT / COUNTERATTACK_BASE_PCT);

        ACTEST("ROG.BLADEWORK", "counterattack damage {} -> {} weapon=dagger pct={}",
            before, GetHitDamage(), int32(COUNTERATTACK_DAGGER_PCT));
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_rog_counterattack::HandleDamage, EFFECT_0, SPELL_EFFECT_WEAPON_PERCENT_DAMAGE);
    }
};

class spell_rog_bladework_loader : public SpellScriptLoader
{
public:
    spell_rog_bladework_loader() : SpellScriptLoader("spell_rog_bladework") { }

    SpellScript* GetSpellScript() const override
    {
        return new spell_rog_bladework();
    }
};

class spell_rog_bladework_mark_loader : public SpellScriptLoader
{
public:
    spell_rog_bladework_mark_loader() : SpellScriptLoader("spell_rog_bladework_mark") { }

    AuraScript* GetAuraScript() const override
    {
        return new spell_rog_bladework_mark();
    }
};

class spell_rog_counterattack_loader : public SpellScriptLoader
{
public:
    spell_rog_counterattack_loader() : SpellScriptLoader("spell_rog_counterattack") { }

    SpellScript* GetSpellScript() const override
    {
        return new spell_rog_counterattack();
    }
};

void AddSC_rog_bladework()
{
    new spell_rog_bladework_loader();
    new spell_rog_bladework_mark_loader();
    new spell_rog_counterattack_loader();
}
