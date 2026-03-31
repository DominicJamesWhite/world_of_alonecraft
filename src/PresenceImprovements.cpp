#include "ScriptMgr.h"
#include "Player.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "SpellScriptLoader.h"
#include "Unit.h"

// Improved Blood Presence (50365 / 50371)
// - Swapping presences no longer costs runes (handled via DBC SPELLMOD_COST modifier targeting all 3 presences)
// - In Blood Presence: +5% damage (200111)
// - In Frost/Unholy: retain healing bonus from Blood Presence (200110)
//
// This file will be extended in Phases 3/5 for Frost and Unholy retained benefits.

enum PresenceSpells
{
    BLOOD_PRESENCE       = 48266,
    FROST_PRESENCE       = 48263,
    UNHOLY_PRESENCE      = 48265,

    IMP_BLOOD_PRES_R1    = 50365,

    RETAINED_HEALING     = 200110,
    BONUS_DAMAGE         = 200111,
};

class spell_dk_presence_blood_retained_AuraScript : public AuraScript
{
    PrepareAuraScript(spell_dk_presence_blood_retained_AuraScript);

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ RETAINED_HEALING, BONUS_DAMAGE });
    }

    void HandleEffectApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        Unit* target = GetTarget();
        if (!target)
            return;

        Player* player = target->ToPlayer();
        if (!player || player->getClass() != CLASS_DEATH_KNIGHT)
            return;

        bool hasImpBloodPres = player->GetAuraEffectOfRankedSpell(IMP_BLOOD_PRES_R1, EFFECT_0) != nullptr;

        if (GetId() == BLOOD_PRESENCE)
        {
            // Entering Blood Presence: remove retained healing (no longer needed),
            // apply bonus damage if talent learned
            player->RemoveAura(RETAINED_HEALING);

            if (hasImpBloodPres && !player->HasAura(BONUS_DAMAGE))
                player->CastSpell(player, BONUS_DAMAGE, true);
        }
        else
        {
            // Entering Frost or Unholy: remove bonus damage,
            // apply retained healing if talent learned
            player->RemoveAura(BONUS_DAMAGE);

            if (hasImpBloodPres && !player->HasAura(RETAINED_HEALING))
                player->CastSpell(player, RETAINED_HEALING, true);
        }
    }

    void HandleEffectRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        Unit* target = GetTarget();
        if (!target)
            return;

        // Clean up both custom auras on presence removal
        target->RemoveAura(RETAINED_HEALING);
        target->RemoveAura(BONUS_DAMAGE);
    }

    void Register() override
    {
        AfterEffectApply += AuraEffectApplyFn(spell_dk_presence_blood_retained_AuraScript::HandleEffectApply, EFFECT_0, SPELL_AURA_ANY, AURA_EFFECT_HANDLE_REAL);
        AfterEffectRemove += AuraEffectRemoveFn(spell_dk_presence_blood_retained_AuraScript::HandleEffectRemove, EFFECT_0, SPELL_AURA_ANY, AURA_EFFECT_HANDLE_REAL);
    }
};

class spell_dk_presence_blood_retained_loader : public SpellScriptLoader
{
public:
    spell_dk_presence_blood_retained_loader() : SpellScriptLoader("spell_dk_presence_blood_retained") { }

    AuraScript* GetAuraScript() const override
    {
        return new spell_dk_presence_blood_retained_AuraScript();
    }
};

void AddSC_dk_presence_improvements()
{
    new spell_dk_presence_blood_retained_loader();
}
