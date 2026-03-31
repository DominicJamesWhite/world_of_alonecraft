#include "ScriptMgr.h"
#include "Player.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "SpellScriptLoader.h"
#include "Unit.h"

// Runic Power Mastery (49455 / 50147)
// Increases max RP by 15/30. Spending Runic Power has a 15/30% chance
// to restore 2% of maximum health.

enum RunicPowerMasterySpells
{
    RPM_HEAL = 200105,
};

class spell_dk_runic_power_mastery_AuraScript : public AuraScript
{
    PrepareAuraScript(spell_dk_runic_power_mastery_AuraScript);

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ RPM_HEAL });
    }

    bool CheckProc(ProcEventInfo& eventInfo)
    {
        SpellInfo const* spellInfo = eventInfo.GetSpellInfo();
        if (!spellInfo)
            return false;

        // Only proc on spells that cost Runic Power
        if (spellInfo->PowerType != POWER_RUNIC_POWER || spellInfo->ManaCost == 0)
            return false;

        return true;
    }

    void HandleProc(AuraEffect const* aurEff, ProcEventInfo& /*eventInfo*/)
    {
        PreventDefaultAction();

        Unit* caster = GetTarget();
        if (!caster)
            return;

        // aurEff->GetAmount() = proc chance (15 or 30)
        if (!roll_chance_i(aurEff->GetAmount()))
            return;

        // Heal for 2% of max health
        int32 healAmount = caster->CountPctFromMaxHealth(2);
        if (healAmount <= 0)
            return;

        caster->CastCustomSpell(RPM_HEAL, SPELLVALUE_BASE_POINT0, healAmount, caster, true, nullptr, aurEff);
    }

    void Register() override
    {
        DoCheckProc += AuraCheckProcFn(spell_dk_runic_power_mastery_AuraScript::CheckProc);
        OnEffectProc += AuraEffectProcFn(spell_dk_runic_power_mastery_AuraScript::HandleProc, EFFECT_1, SPELL_AURA_DUMMY);
    }
};

class spell_dk_runic_power_mastery_loader : public SpellScriptLoader
{
public:
    spell_dk_runic_power_mastery_loader() : SpellScriptLoader("spell_dk_runic_power_mastery") { }

    AuraScript* GetAuraScript() const override
    {
        return new spell_dk_runic_power_mastery_AuraScript();
    }
};

void AddSC_dk_runic_power_mastery()
{
    new spell_dk_runic_power_mastery_loader();
}
