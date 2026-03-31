#include "ScriptMgr.h"
#include "Player.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "SpellScriptLoader.h"
#include "Unit.h"

// Improved Blood Presence (50365 / 50371)
// - Swapping presences no longer costs runes (handled via DBC SPELLMOD modifier targeting all 3 presences)
// - In Blood Presence: +5% damage (200111)
// - In Frost/Unholy: retain healing bonus from Blood Presence (200110)
//
// Improved Frost Presence (50384 / 50385)
// - Swapping presences no longer costs runes (same DBC mechanism)
// - In Frost Presence: -5% damage taken (200115)
// - In Blood/Unholy: retain armor bonus from Frost Presence (200114)

enum PresenceSpells
{
    BLOOD_PRESENCE       = 48266,
    FROST_PRESENCE       = 48263,
    UNHOLY_PRESENCE      = 48265,

    IMP_BLOOD_PRES_R1    = 50365,
    IMP_FROST_PRES_R1    = 50384,

    RETAINED_HEALING     = 200110,
    BONUS_DAMAGE         = 200111,
    RETAINED_ARMOR       = 200114,
    FROST_DR             = 200115,
};

class spell_dk_presence_blood_retained_AuraScript : public AuraScript
{
    PrepareAuraScript(spell_dk_presence_blood_retained_AuraScript);

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ RETAINED_HEALING, BONUS_DAMAGE, RETAINED_ARMOR, FROST_DR });
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
        bool hasImpFrostPres = player->GetAuraEffectOfRankedSpell(IMP_FROST_PRES_R1, EFFECT_0) != nullptr;

        if (GetId() == BLOOD_PRESENCE)
        {
            // Entering Blood Presence: remove Blood-specific retained auras,
            // apply bonus damage if talent learned
            player->RemoveAura(RETAINED_HEALING);
            player->RemoveAura(FROST_DR);

            if (hasImpBloodPres && !player->HasAura(BONUS_DAMAGE))
                player->CastSpell(player, BONUS_DAMAGE, true);

            // Retain Frost armor if Imp Frost Presence is learned
            if (hasImpFrostPres && !player->HasAura(RETAINED_ARMOR))
                player->CastSpell(player, RETAINED_ARMOR, true);
        }
        else if (GetId() == FROST_PRESENCE)
        {
            // Entering Frost Presence: remove retained auras (no longer needed),
            // apply DR if Imp Frost Presence learned
            player->RemoveAura(BONUS_DAMAGE);
            player->RemoveAura(RETAINED_ARMOR);

            if (hasImpFrostPres && !player->HasAura(FROST_DR))
                player->CastSpell(player, FROST_DR, true);

            // Retain Blood healing if Imp Blood Presence is learned
            if (hasImpBloodPres && !player->HasAura(RETAINED_HEALING))
                player->CastSpell(player, RETAINED_HEALING, true);
        }
        else // UNHOLY_PRESENCE
        {
            // Entering Unholy Presence: remove presence-specific bonuses,
            // apply retained auras for learned talents
            player->RemoveAura(BONUS_DAMAGE);
            player->RemoveAura(FROST_DR);

            if (hasImpBloodPres && !player->HasAura(RETAINED_HEALING))
                player->CastSpell(player, RETAINED_HEALING, true);

            if (hasImpFrostPres && !player->HasAura(RETAINED_ARMOR))
                player->CastSpell(player, RETAINED_ARMOR, true);
        }
    }

    void HandleEffectRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        Unit* target = GetTarget();
        if (!target)
            return;

        // Clean up all custom presence auras on presence removal
        target->RemoveAura(RETAINED_HEALING);
        target->RemoveAura(BONUS_DAMAGE);
        target->RemoveAura(RETAINED_ARMOR);
        target->RemoveAura(FROST_DR);
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
