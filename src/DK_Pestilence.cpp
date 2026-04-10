#include "ScriptMgr.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "SpellScriptLoader.h"
#include "Unit.h"

// Alonecraft Pestilence override:
// Replaces core spell_dk_pestilence to fix Glyph of Disease refresh.
// Core uses else-if for Ebon Plague / Crypt Fever, so only one gets
// refreshed. In Alonecraft both coexist, so both must be refreshed.

enum PestilenceSpells
{
    SPELL_DK_GLYPH_OF_DISEASE       = 63334,
    SPELL_DK_BLOOD_PLAGUE            = 55078,
    SPELL_DK_FROST_FEVER             = 55095,
    SPELL_DK_ICY_TALONS_TALENT_R1   = 50880,
    SPELL_DK_CRYPT_FEVER_R1         = 50508,
    SPELL_DK_EBON_PLAGUE_R1         = 51726,
};

// 50842 - Pestilence
class spell_dk_pestilence_alonecraft : public SpellScript
{
    PrepareSpellScript(spell_dk_pestilence_alonecraft);

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo(
        {
            SPELL_DK_GLYPH_OF_DISEASE,
            SPELL_DK_BLOOD_PLAGUE,
            SPELL_DK_FROST_FEVER,
            SPELL_DK_ICY_TALONS_TALENT_R1
        });
    }

    void HandleScriptEffect(SpellEffIndex /*effIndex*/)
    {
        Unit* caster = GetCaster();
        Unit* hitUnit = GetHitUnit();
        Unit* target = GetExplTargetUnit();
        if (!target)
            return;

        // Spread on others
        if (target != hitUnit)
        {
            // Blood Plague
            if (target->GetAura(SPELL_DK_BLOOD_PLAGUE, caster->GetGUID()))
                caster->CastSpell(hitUnit, SPELL_DK_BLOOD_PLAGUE, true);

            // Frost Fever
            if (target->GetAura(SPELL_DK_FROST_FEVER, caster->GetGUID()))
                caster->CastSpell(hitUnit, SPELL_DK_FROST_FEVER, true);
        }
        // Refresh on target (Glyph of Disease)
        else if (caster->GetAura(SPELL_DK_GLYPH_OF_DISEASE))
        {
            // Blood Plague
            if (Aura* disease = target->GetAura(SPELL_DK_BLOOD_PLAGUE, caster->GetGUID()))
                disease->RefreshDuration();

            // Frost Fever
            if (Aura* disease = target->GetAura(SPELL_DK_FROST_FEVER, caster->GetGUID()))
            {
                disease->RefreshDuration();
                if (Aura const* talons = caster->GetAuraOfRankedSpell(SPELL_DK_ICY_TALONS_TALENT_R1))
                    caster->CastSpell(caster, talons->GetSpellInfo()->Effects[EFFECT_0].TriggerSpell, true);
            }

            // Ebon Plague — refresh independently
            if (Aura* disease = target->GetAuraOfRankedSpell(SPELL_DK_EBON_PLAGUE_R1, caster->GetGUID()))
                disease->RefreshDuration();

            // Crypt Fever — refresh independently (core uses else-if, we need both)
            if (Aura* disease = target->GetAuraOfRankedSpell(SPELL_DK_CRYPT_FEVER_R1, caster->GetGUID()))
                disease->RefreshDuration();
        }
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_dk_pestilence_alonecraft::HandleScriptEffect, EFFECT_2, SPELL_EFFECT_SCRIPT_EFFECT);
    }
};

class pestilence_alonecraft_loader : public SpellScriptLoader
{
public:
    pestilence_alonecraft_loader() : SpellScriptLoader("spell_dk_pestilence") { }
    SpellScript* GetSpellScript() const override
    {
        return new spell_dk_pestilence_alonecraft();
    }
};

void AddSC_dk_pestilence_alonecraft()
{
    new pestilence_alonecraft_loader();
}
