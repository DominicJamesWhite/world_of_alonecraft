#include "AlonecraftTestLog.h"
#include "ScriptMgr.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "SpellScript.h"
#include "SpellScriptLoader.h"
#include "Unit.h"

// Dirty Tricks (14076 / 14094) -- Alonecraft
//
// TODO.md: "Redesigned. Ambush and Backstab no longer require you to be
// behind the target."
//
// The behind-target rule is SPELL_ATTR0_CU_REQ_CASTER_BEHIND_TARGET, which
// is not a DBC field: SpellMgr.cpp:3531-3591 assigns it from a hard-coded
// switch and Spell.cpp:5870 enforces it.  It also lives on the SpellInfo,
// so it is global -- there is no per-player version of it to toggle.
//
// So the attribute is cleared for everyone at spell-load time and the
// restriction is re-imposed per cast for rogues who did not take the
// talent.  Both halves live in this module; no core file is modified.
//
// The talent is a single point: the redesign is binary, so a second rank
// would buy nothing.  woa_2026_08_03_07.sql shortens talent_dbc 262 to one
// rank and reverts 14094 to vanilla.

enum DirtyTricksSpells
{
    SPELL_ROGUE_DIRTY_TRICKS = 14076,
};

// Every Ambush and Backstab rank listed in SpellMgr.cpp:3531/3568.  Kept as
// an explicit list rather than a rank-chain walk because the attribute is
// applied per SpellInfo during load, before rank chains are usable.
static uint32 const BackstabRanks[] =
{
    53, 2589, 2590, 2591, 8721, 11279, 11280, 11281, 25300, 26863, 48656, 48657
};

static uint32 const AmbushRanks[] =
{
    8676, 8724, 8725, 11267, 11268, 11269, 27441, 48689, 48690, 48691
};

// Clears the behind-target attribute as each SpellInfo finishes loading.
// GLOBALHOOK_ON_LOAD_SPELL_CUSTOM_ATTR fires at SpellMgr.cpp:3845, i.e.
// after the switch that sets it, so this reliably wins.
class RogueDirtyTricks_GlobalScript : public GlobalScript
{
public:
    RogueDirtyTricks_GlobalScript() : GlobalScript("RogueDirtyTricks_GlobalScript", {
        GLOBALHOOK_ON_LOAD_SPELL_CUSTOM_ATTR
    }) { }

    void OnLoadSpellCustomAttr(SpellInfo* spell) override
    {
        if (!spell)
            return;

        for (uint32 id : BackstabRanks)
            if (spell->Id == id)
            {
                spell->AttributesCu &= ~SPELL_ATTR0_CU_REQ_CASTER_BEHIND_TARGET;
                return;
            }

        for (uint32 id : AmbushRanks)
            if (spell->Id == id)
            {
                spell->AttributesCu &= ~SPELL_ATTR0_CU_REQ_CASTER_BEHIND_TARGET;
                return;
            }
    }
};

class spell_rog_dirty_tricks : public SpellScript
{
    PrepareSpellScript(spell_rog_dirty_tricks);

    bool Load() override
    {
        return GetCaster()->IsPlayer();
    }

    SpellCastResult CheckCast()
    {
        Unit* caster = GetCaster();
        Unit* target = GetExplTargetUnit();
        if (!caster || !target)
            return SPELL_CAST_OK;

        bool const talented = caster->HasAura(SPELL_ROGUE_DIRTY_TRICKS);
        bool const inFront  = target->HasInArc(static_cast<float>(M_PI), caster);

        ACTEST("ROG.DIRTY", "spell={} target={} talented={} casterInFrontOfTarget={} result={}",
            GetSpellInfo()->Id, Alonecraft::TestLog::N(target), talented, inFront,
            (talented || !inFront) ? "OK" : "FAILED_NOT_BEHIND");

        if (talented)
            return SPELL_CAST_OK;

        // Untalented -- restore the core rule from Spell.cpp:5870.
        if (inFront)
            return SPELL_FAILED_NOT_BEHIND;

        return SPELL_CAST_OK;
    }

    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_rog_dirty_tricks::CheckCast);
    }
};

class spell_rog_dirty_tricks_loader : public SpellScriptLoader
{
public:
    spell_rog_dirty_tricks_loader() : SpellScriptLoader("spell_rog_dirty_tricks") { }

    SpellScript* GetSpellScript() const override
    {
        return new spell_rog_dirty_tricks();
    }
};

void AddSC_rog_dirty_tricks()
{
    new RogueDirtyTricks_GlobalScript();
    new spell_rog_dirty_tricks_loader();
}
