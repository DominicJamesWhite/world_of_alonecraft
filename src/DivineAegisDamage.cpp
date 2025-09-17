#include "ScriptMgr.h"
#include "Player.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "SpellScriptLoader.h"
#include "Unit.h"
#include "Log.h"

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

    void OnApply(AuraEffect const* aurEff, AuraEffectHandleModes /*mode*/)
    {
        Unit* target = GetTarget();
        if (!target)
            return;

        Player* player = target->ToPlayer();
        if (!player)
            return;

        uint32 spellId = GetSpellInfo()->Id;
        int32 percentage = aurEff->GetAmount();

        LOG_ERROR("scripts", "Divine Aegis crit tracker aura ({}) applied to player {} ({}) with {}% value", 
                 spellId, player->GetName(), player->GetGUID().ToString(), percentage);
    }

    void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        Unit* target = GetTarget();
        if (!target)
            return;

        Player* player = target->ToPlayer();
        if (!player)
            return;

        uint32 spellId = GetSpellInfo()->Id;
        LOG_ERROR("scripts", "Divine Aegis crit tracker aura ({}) removed from player {} ({})", 
                 spellId, player->GetName(), player->GetGUID().ToString());
    }

    void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();

        Unit* caster = GetTarget();
        if (!caster)
            return;

        Player* player = caster->ToPlayer();
        if (!player)
            return;

        uint32 spellId = GetSpellInfo()->Id;
        
        // Log proc detection
        LOG_ERROR("scripts", "Divine Aegis damage proc detected for aura {} on player {} ({})", 
                 spellId, player->GetName(), player->GetGUID().ToString());

        // Get the percentage directly from the aura effect (like original Divine Aegis)
        int32 aegisPercentage = aurEff->GetAmount();
        if (aegisPercentage <= 0)
        {
            LOG_ERROR("scripts", "Divine Aegis proc cancelled - invalid percentage {} for player {}", 
                     aegisPercentage, player->GetName());
            return;
        }

        // Get the damage dealt from the event
        uint32 damage = eventInfo.GetDamageInfo() ? eventInfo.GetDamageInfo()->GetDamage() : 0;
        if (damage == 0)
        {
            LOG_ERROR("scripts", "Divine Aegis proc cancelled - no damage info for player {}", 
                     player->GetName());
            return;
        }

        // Log damage detected
        LOG_ERROR("scripts", "Divine Aegis damage detected: {} damage, {}% from aura {} for player {}", 
                 damage, aegisPercentage, spellId, player->GetName());

        // Calculate absorb amount based on aura percentage (like original Divine Aegis)
        int32 absorbAmount = CalculatePct(static_cast<int32>(damage), aegisPercentage);

        // Log absorb calculation
        LOG_ERROR("scripts", "Divine Aegis calculated absorb: {} absorb from {} damage ({}%) for player {}", 
                 absorbAmount, damage, aegisPercentage, player->GetName());

        // Apply the Divine Aegis absorb shield to the player (self-cast)
        ApplyDivineAegisAbsorb(player, player, absorbAmount);

        // Log successful application
        LOG_ERROR("scripts", "Divine Aegis absorb shield applied: {} absorb for player {}", 
                 absorbAmount, player->GetName());
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
    LOG_ERROR("scripts", "Loading Divine Aegis damage mechanic script...");
    new spell_divine_aegis_crit_tracker_loader();
    LOG_ERROR("scripts", "Divine Aegis damage mechanic script loaded successfully!");
}
