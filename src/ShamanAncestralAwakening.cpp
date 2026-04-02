#include "ScriptMgr.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "SpellScriptLoader.h"
#include "Unit.h"

// Ancestral Awakening (extended) — Shaman Restoration talent
// Spell 52752 is the Ancestral Awakening proc heal (cast by the shaman on the
// lowest-health ally). After this heal lands, apply a guaranteed crit buff
// (200210) to the caster.
//
// 200210: SPELL_AURA_MOD_SPELL_CRIT_CHANCE, BP=99 (+100%), Duration 15s,
// ProcCharges=1 (consumed on next damage spell).

enum AncestralAwakeningSpells
{
    ANCESTRAL_AWAKENING_HEAL = 52752,
    ANCESTRAL_FURY_BUFF      = 200210,
};

class spell_sha_ancestral_fury_SpellScript : public SpellScript
{
    PrepareSpellScript(spell_sha_ancestral_fury_SpellScript);

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ ANCESTRAL_FURY_BUFF });
    }

    void HandleAfterHit()
    {
        Unit* caster = GetCaster();
        if (!caster)
            return;

        // Only apply if the heal actually landed
        if (GetHitHeal() <= 0)
            return;

        caster->CastSpell(caster, ANCESTRAL_FURY_BUFF, true);
    }

    void Register() override
    {
        AfterHit += SpellHitFn(spell_sha_ancestral_fury_SpellScript::HandleAfterHit);
    }
};

class spell_sha_ancestral_fury_loader : public SpellScriptLoader
{
public:
    spell_sha_ancestral_fury_loader() : SpellScriptLoader("spell_sha_ancestral_fury") { }

    SpellScript* GetSpellScript() const override
    {
        return new spell_sha_ancestral_fury_SpellScript();
    }
};

void AddSC_sha_ancestral_awakening_crit()
{
    new spell_sha_ancestral_fury_loader();
}
