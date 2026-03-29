#include "ScriptMgr.h"
#include "Player.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "SpellScriptLoader.h"
#include "Unit.h"

// Spell IDs
#define DIVINE_AEGIS_CRIT_TRACKER_R1 200073  // Aura that tracks spell critical hits - Rank 1
#define DIVINE_AEGIS_CRIT_TRACKER_R2 200074  // Aura that tracks spell critical hits - Rank 2
#define DIVINE_AEGIS_CRIT_TRACKER_R3 200075  // Aura that tracks spell critical hits - Rank 3
#define DIVINE_AEGIS_ABSORB_ID 47753         // Divine Aegis absorb shield

/*
 * Helper function to apply Divine Aegis absorb shield
 */
static void ApplyDivineAegisAbsorb(Unit* caster, Unit* target, int32 absorbAmount)
{
    if (!caster || !target || absorbAmount <= 0)
        return;

    // Multiple effects stack, so let's try to find existing Divine Aegis aura
    if (AuraEffect const* existingAegis = target->GetAuraEffect(DIVINE_AEGIS_ABSORB_ID, EFFECT_0))
        absorbAmount += existingAegis->GetAmount();

    // Cap the absorb at target level * 125 (same as original Divine Aegis)
    absorbAmount = std::min(absorbAmount, static_cast<int32>(target->GetLevel() * 125));

    // Cast the Divine Aegis absorb shield
    caster->CastCustomSpell(DIVINE_AEGIS_ABSORB_ID, SPELLVALUE_BASE_POINT0, absorbAmount, target, true);
}

/*
 * AuraScript for the Divine Aegis critical hit tracker (200073)
 * This is a persistent aura that procs on spell critical hits
 * It triggers the Divine Aegis damage-to-absorb conversion
 */
class spell_divine_aegis_crit_tracker_AuraScript : public AuraScript
{
    PrepareAuraScript(spell_divine_aegis_crit_tracker_AuraScript);

    void OnApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/) { }

    void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/) { }

    void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();

        Unit* caster = GetTarget();
        if (!caster)
            return;

        Player* player = caster->ToPlayer();
        if (!player)
            return;

        int32 aegisPercentage = aurEff->GetAmount();
        if (aegisPercentage <= 0)
            return;

        uint32 damage = eventInfo.GetDamageInfo() ? eventInfo.GetDamageInfo()->GetDamage() : 0;
        if (damage == 0)
            return;

        int32 absorbAmount = CalculatePct(static_cast<int32>(damage), aegisPercentage);

        ApplyDivineAegisAbsorb(player, player, absorbAmount);
    }

    void Register() override
    {
        AfterEffectApply += AuraEffectApplyFn(spell_divine_aegis_crit_tracker_AuraScript::OnApply, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
        AfterEffectRemove += AuraEffectRemoveFn(spell_divine_aegis_crit_tracker_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
        OnEffectProc += AuraEffectProcFn(spell_divine_aegis_crit_tracker_AuraScript::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
    }
};

class spell_divine_aegis_crit_tracker_loader : public SpellScriptLoader
{
public:
    spell_divine_aegis_crit_tracker_loader() : SpellScriptLoader("spell_divine_aegis_crit_tracker") { }

    AuraScript* GetAuraScript() const override
    {
        return new spell_divine_aegis_crit_tracker_AuraScript();
    }
};

// Registration
void AddSC_divine_aegis_damage_mechanic()
{
    new spell_divine_aegis_crit_tracker_loader();
}
