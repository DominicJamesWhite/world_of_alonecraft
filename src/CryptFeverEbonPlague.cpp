#include "ScriptMgr.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "SpellScriptLoader.h"
#include "Unit.h"

// Crypt Fever & Ebon Plaguebringer rework (Alonecraft):
//
// Crypt Fever: Your diseases cause Crypt Fever, increasing disease damage
//   taken by X% and dealing additional Shadow damage every 3 sec.
//   Periodic damage is 50% of Blood Plague's base, defined in the DBC.
//
// Ebon Plaguebringer: Your diseases ALSO cause Ebon Plague (no longer
//   replaces Crypt Fever), increasing magic damage taken by X% and dealing
//   additional Shadow damage every 3 sec.
//   Periodic damage is 50% of Frost Fever's base, defined in the DBC.
//
// The only C++ needed: when Ebon Plague is applied, also apply Crypt Fever.

enum CryptFeverEbonPlagueSpells
{
    // Crypt Fever talent passives (the auras the DK has)
    CF_TALENT_R1                = 49032,
    CF_TALENT_R2                = 49631,
    CF_TALENT_R3                = 49632,

    // Crypt Fever disease debuffs (applied to the target)
    CF_DISEASE_R1               = 50508,
    CF_DISEASE_R2               = 50509,
    CF_DISEASE_R3               = 50510,
};

// ---------------------------------------------------------------------------
// Ebon Plague disease: when applied, also apply Crypt Fever
// (core only applies one or the other; we make them coexist)
// ---------------------------------------------------------------------------
class spell_dk_ebon_plague_apply_cf_AuraScript : public AuraScript
{
    PrepareAuraScript(spell_dk_ebon_plague_apply_cf_AuraScript);

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ CF_DISEASE_R1, CF_DISEASE_R2, CF_DISEASE_R3 });
    }

    void AfterApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        Unit* caster = GetCaster();
        Unit* target = GetTarget();
        if (!caster || !target)
            return;

        // Determine which Crypt Fever rank to apply based on the caster's talent
        uint32 cfSpellId = 0;
        if (caster->HasAura(CF_TALENT_R3))
            cfSpellId = CF_DISEASE_R3;
        else if (caster->HasAura(CF_TALENT_R2))
            cfSpellId = CF_DISEASE_R2;
        else if (caster->HasAura(CF_TALENT_R1))
            cfSpellId = CF_DISEASE_R1;

        if (!cfSpellId)
            return;

        // If Crypt Fever is already active, refresh its duration;
        // otherwise apply it fresh.
        if (Aura* existing = target->GetAura(cfSpellId, caster->GetGUID()))
            existing->RefreshDuration();
        else
            caster->CastSpell(target, cfSpellId, true);
    }

    void Register() override
    {
        AfterEffectApply += AuraEffectApplyFn(
            spell_dk_ebon_plague_apply_cf_AuraScript::AfterApply,
            EFFECT_0, SPELL_AURA_LINKED, AURA_EFFECT_HANDLE_REAL_OR_REAPPLY_MASK);
    }
};

// ---------------------------------------------------------------------------
// SpellScriptLoader
// ---------------------------------------------------------------------------
class ebon_plague_apply_cf_loader : public SpellScriptLoader
{
public:
    ebon_plague_apply_cf_loader() : SpellScriptLoader("spell_dk_ebon_plague_apply_cf") { }
    AuraScript* GetAuraScript() const override
    {
        return new spell_dk_ebon_plague_apply_cf_AuraScript();
    }
};

void AddSC_dk_crypt_fever_ebon_plague()
{
    new ebon_plague_apply_cf_loader();
}
