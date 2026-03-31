#include "ScriptMgr.h"
#include "SpellAuraEffects.h"
#include "SpellScript.h"
#include "Unit.h"

// Hungering Cold Rework (Alonecraft)
// Aura 51209 (the freeze applied to enemies):
//   - OnApply: infect the target with Frost Fever
//   - OnRemove: deal Frost damage (200113) and heal the DK for that amount
// Also preserves the vanilla CheckProc that blocks disease-dispel procs
// from breaking the freeze.
//
// 200113 is a SCHOOL_DAMAGE spell with AP scaling via spell_bonus_data
// (ap_bonus = 0.3203, ~2/3 of full DnD). A companion SpellScript on
// 200113 heals the caster for the actual damage dealt.

enum HungeringColdSpells
{
    SPELL_DK_FROST_FEVER         = 55095,
    SPELL_DK_HUNGERING_COLD_THAW = 200113,
    SPELL_DK_CHILLBLAINS_HEAL    = 200112,
};

// SpellScript on 200113 — after dealing damage, heal the caster
class spell_dk_hungering_cold_thaw : public SpellScript
{
    PrepareSpellScript(spell_dk_hungering_cold_thaw);

    void HealCaster()
    {
        Unit* caster = GetCaster();
        if (!caster)
            return;

        int32 damage = GetHitDamage();
        if (damage <= 0)
            return;

        caster->CastCustomSpell(SPELL_DK_CHILLBLAINS_HEAL,
            SPELLVALUE_BASE_POINT0, damage, caster, true);
    }

    void Register() override
    {
        AfterHit += SpellHitFn(spell_dk_hungering_cold_thaw::HealCaster);
    }
};

// AuraScript on 51209 — the freeze aura applied to enemies
class spell_dk_hungering_cold_rework : public AuraScript
{
    PrepareAuraScript(spell_dk_hungering_cold_rework);

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo(
        {
            SPELL_DK_FROST_FEVER,
            SPELL_DK_HUNGERING_COLD_THAW
        });
    }

    // Vanilla behavior: don't let disease dispels break the freeze
    bool CheckProc(ProcEventInfo& eventInfo)
    {
        SpellInfo const* spellInfo = eventInfo.GetSpellInfo();
        if (!spellInfo)
            return true;

        return spellInfo->Dispel != DISPEL_DISEASE;
    }

    // When the freeze is applied, infect the target with Frost Fever
    void OnApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        Unit* caster = GetCaster();
        Unit* target = GetTarget();
        if (!caster || !target)
            return;

        caster->CastSpell(target, SPELL_DK_FROST_FEVER, true);
    }

    // When the freeze ends (expires or broken), drain health
    void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        Unit* caster = GetCaster();
        Unit* target = GetTarget();
        if (!caster || !target)
            return;

        // Damage + heal handled by 200113 spell + its SpellScript
        caster->CastSpell(target, SPELL_DK_HUNGERING_COLD_THAW, true);
    }

    void Register() override
    {
        DoCheckProc += AuraCheckProcFn(spell_dk_hungering_cold_rework::CheckProc);
        AfterEffectApply += AuraEffectApplyFn(spell_dk_hungering_cold_rework::OnApply, EFFECT_0, SPELL_AURA_MOD_STUN, AURA_EFFECT_HANDLE_REAL);
        AfterEffectRemove += AuraEffectRemoveFn(spell_dk_hungering_cold_rework::OnRemove, EFFECT_0, SPELL_AURA_MOD_STUN, AURA_EFFECT_HANDLE_REAL);
    }
};

void AddSC_dk_hungering_cold_rework()
{
    RegisterSpellScript(spell_dk_hungering_cold_thaw);
    RegisterSpellScript(spell_dk_hungering_cold_rework);
}
