#include "ScriptMgr.h"
#include "SpellScript.h"
#include "SpellScriptLoader.h"
#include "Unit.h"

// Master of Ghouls Rework (Alonecraft)
// Talent 1984 now grants custom spell 200132 instead of vanilla 52143.
// - Keeps the 60s Raise Dead cooldown reduction (ADD_FLAT_MODIFIER in DBC Effect1).
// - Army of the Dead tick rate halved via ADD_PCT_MODIFIER (DBC Effect2).
// - Ghoul is a temporary guardian (core takes guardian path since HasAura(52143) is false).
// - Raise Dead (46584) summons 3 additional guardian ghouls via custom spell 200133.

enum MasterOfGhoulsSpells
{
    SPELL_MASTER_OF_GHOULS      = 200132,  // custom replacement for 52143
    SPELL_SUMMON_EXTRA_GHOULS   = 200133,  // custom: 3 guardians, SummonProps 713
};

// ---------------------------------------------------------------
// Script 1: Raise Dead — summon 3 extra guardian ghouls
// ---------------------------------------------------------------
class spell_alonecraft_raise_dead_extra_SpellScript : public SpellScript
{
    PrepareSpellScript(spell_alonecraft_raise_dead_extra_SpellScript);

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_MASTER_OF_GHOULS, SPELL_SUMMON_EXTRA_GHOULS });
    }

    void HandleAfterCast()
    {
        Unit* caster = GetCaster();
        if (!caster || !caster->HasAura(SPELL_MASTER_OF_GHOULS))
            return;

        caster->CastSpell(caster, SPELL_SUMMON_EXTRA_GHOULS, true);
    }

    void Register() override
    {
        AfterCast += SpellCastFn(spell_alonecraft_raise_dead_extra_SpellScript::HandleAfterCast);
    }
};

class spell_alonecraft_raise_dead_extra_loader : public SpellScriptLoader
{
public:
    spell_alonecraft_raise_dead_extra_loader() : SpellScriptLoader("spell_alonecraft_raise_dead_extra") { }

    SpellScript* GetSpellScript() const override
    {
        return new spell_alonecraft_raise_dead_extra_SpellScript();
    }
};

// ---------------------------------------------------------------
// Registration
// ---------------------------------------------------------------
void AddSC_dk_master_of_ghouls_rework()
{
    new spell_alonecraft_raise_dead_extra_loader();
}
