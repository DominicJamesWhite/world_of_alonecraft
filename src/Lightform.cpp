#include "ScriptMgr.h"
#include "Player.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "SpellScriptLoader.h"
#include "Unit.h"
#include "Log.h"

#define LIGHTFORM 200060
#define LIGHTFORM_BONUS_1 200061
#define LIGHTFORM_BONUS_2 200062

/*
 * AuraScript for the main priest aura (200060).
 * When this aura is applied, it automatically applies the two linked auras (200061 and 200062).
 * When this aura is removed, it removes the linked auras as well.
 */
class spell_priest_aura_interaction_AuraScript : public AuraScript
{
    PrepareAuraScript(spell_priest_aura_interaction_AuraScript);

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({LIGHTFORM, LIGHTFORM_BONUS_1, LIGHTFORM_BONUS_2});
    }

    void OnApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        Unit* target = GetTarget();
        if (!target)
            return;

        Player* player = target->ToPlayer();
        if (!player)
            return;

        // Only apply to priests
        if (player->getClass() != CLASS_PRIEST)
            return;

        // Apply the linked auras if they don't already exist
        if (!player->HasAura(LIGHTFORM_BONUS_1))
            player->AddAura(LIGHTFORM_BONUS_1, player);

        if (!player->HasAura(LIGHTFORM_BONUS_2))
            player->AddAura(LIGHTFORM_BONUS_2, player);
    }

    void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        Unit* target = GetTarget();
        if (!target)
            return;

        Player* player = target->ToPlayer();
        if (!player)
            return;

        // Only handle priests
        if (player->getClass() != CLASS_PRIEST)
            return;

        // Remove the linked auras when the main aura is removed
        if (player->HasAura(LIGHTFORM_BONUS_1))
            player->RemoveAura(LIGHTFORM_BONUS_1);

        if (player->HasAura(LIGHTFORM_BONUS_2))
            player->RemoveAura(LIGHTFORM_BONUS_2);
    }

    void Register() override
    {
        AfterEffectApply += AuraEffectApplyFn(spell_priest_aura_interaction_AuraScript::OnApply, EFFECT_ALL, SPELL_AURA_ANY, AURA_EFFECT_HANDLE_REAL);
        AfterEffectRemove += AuraEffectRemoveFn(spell_priest_aura_interaction_AuraScript::OnRemove, EFFECT_ALL, SPELL_AURA_ANY, AURA_EFFECT_HANDLE_REAL);
    }
};

class spell_priest_aura_interaction_loader : public SpellScriptLoader
{
public:
    spell_priest_aura_interaction_loader() : SpellScriptLoader("spell_priest_aura_interaction") { }

    AuraScript* GetAuraScript() const override
    {
        return new spell_priest_aura_interaction_AuraScript();
    }
};

// Registration function
void AddSC_priest_aura_interaction()
{
    new spell_priest_aura_interaction_loader();
}
