#include "ScriptMgr.h"
#include "Player.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "SpellScriptLoader.h"
#include "Unit.h"

// Anti-Magic Shell (48707) — module override of core spell_dk_anti_magic_shell_self.
// Replicates base behavior (absorb, RP energize, Cyclone immunity) and adds
// Magic Siphon healing: X% of damage absorbed by AMS is returned as health
// when the DK has any rank of Magic Siphon (renamed Magic Suppression).

enum MagicSiphonSpells
{
    SPELL_DK_RUNIC_POWER_ENERGIZE = 49088,
    SPELL_DK_MAGIC_SIPHON_HEAL   = 200116,
    SPELL_DK_MAGIC_SIPHON_R1     = 49224,
    SPELL_DK_MAGIC_SIPHON_R2     = 49610,
    SPELL_DK_MAGIC_SIPHON_R3     = 49611,
    SPELL_DK_CYCLONE             = 33786,
};

class spell_dk_ams_magic_siphon_AuraScript : public AuraScript
{
    PrepareAuraScript(spell_dk_ams_magic_siphon_AuraScript);

    uint32 absorbPct, hpPct;

    bool Load() override
    {
        absorbPct = GetSpellInfo()->Effects[EFFECT_0].CalcValue(GetCaster());
        hpPct = GetSpellInfo()->Effects[EFFECT_1].CalcValue(GetCaster());
        return true;
    }

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_DK_RUNIC_POWER_ENERGIZE, SPELL_DK_MAGIC_SIPHON_HEAL });
    }

    void CalculateAmount(AuraEffect const* /*aurEff*/, int32& amount, bool& /*canBeRecalculated*/)
    {
        amount = GetCaster()->CountPctFromMaxHealth(hpPct);
    }

    void Absorb(AuraEffect* /*aurEff*/, DamageInfo& dmgInfo, uint32& absorbAmount)
    {
        absorbAmount = std::min(CalculatePct(dmgInfo.GetDamage(), absorbPct), GetTarget()->CountPctFromMaxHealth(hpPct));
    }

    void Trigger(AuraEffect* aurEff, DamageInfo& /*dmgInfo*/, uint32& absorbAmount)
    {
        Unit* target = GetTarget();

        // Runic Power energize (20% of absorbed damage) — vanilla behavior
        int32 bp = CalculatePct(absorbAmount, 20);
        target->CastCustomSpell(SPELL_DK_RUNIC_POWER_ENERGIZE, SPELLVALUE_BASE_POINT0, bp, target, true, nullptr, aurEff);

        // Magic Siphon: heal for X% of absorbed damage based on talent rank
        int32 healPct = 0;
        if (target->HasAura(SPELL_DK_MAGIC_SIPHON_R3))
            healPct = 30;
        else if (target->HasAura(SPELL_DK_MAGIC_SIPHON_R2))
            healPct = 20;
        else if (target->HasAura(SPELL_DK_MAGIC_SIPHON_R1))
            healPct = 10;

        if (healPct > 0)
        {
            int32 healAmount = CalculatePct(absorbAmount, healPct);
            if (healAmount > 0)
                target->CastCustomSpell(SPELL_DK_MAGIC_SIPHON_HEAL, SPELLVALUE_BASE_POINT0, healAmount, target, true, nullptr, aurEff);
        }
    }

    void HandleEffectApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        GetTarget()->ApplySpellImmune(GetId(), IMMUNITY_ID, SPELL_DK_CYCLONE, true);
    }

    void HandleEffectRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        GetTarget()->ApplySpellImmune(GetId(), IMMUNITY_ID, SPELL_DK_CYCLONE, false);
    }

    void Register() override
    {
        DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_dk_ams_magic_siphon_AuraScript::CalculateAmount, EFFECT_0, SPELL_AURA_SCHOOL_ABSORB);
        OnEffectAbsorb += AuraEffectAbsorbFn(spell_dk_ams_magic_siphon_AuraScript::Absorb, EFFECT_0);
        AfterEffectAbsorb += AuraEffectAbsorbFn(spell_dk_ams_magic_siphon_AuraScript::Trigger, EFFECT_0);

        OnEffectApply += AuraEffectApplyFn(spell_dk_ams_magic_siphon_AuraScript::HandleEffectApply, EFFECT_1, SPELL_AURA_MOD_IMMUNE_AURA_APPLY_SCHOOL, AURA_EFFECT_HANDLE_REAL);
        OnEffectRemove += AuraEffectRemoveFn(spell_dk_ams_magic_siphon_AuraScript::HandleEffectRemove, EFFECT_1, SPELL_AURA_MOD_IMMUNE_AURA_APPLY_SCHOOL, AURA_EFFECT_HANDLE_REAL);
    }
};

class spell_dk_ams_magic_siphon_loader : public SpellScriptLoader
{
public:
    spell_dk_ams_magic_siphon_loader() : SpellScriptLoader("spell_dk_ams_magic_siphon") { }

    AuraScript* GetAuraScript() const override
    {
        return new spell_dk_ams_magic_siphon_AuraScript();
    }
};

void AddSC_dk_magic_siphon()
{
    new spell_dk_ams_magic_siphon_loader();
}
