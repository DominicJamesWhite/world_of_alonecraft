#include "ScriptMgr.h"
#include "Player.h"
#include "SpellAuraEffects.h"
#include "SpellScript.h"
#include "SpellScriptLoader.h"

enum MartyrdomSpells
{
    MARTYRDOM_R1       = 14531,
    MARTYRDOM_R2       = 14774,
    MARTYRS_RESOLVE    = 200096,
};

// When Power Word: Shield on yourself ends (any removal type),
// if you have the Martyrdom talent, gain Martyr's Resolve (40% DR for 5s).
class spell_martyrdom_pws_remove_AuraScript : public AuraScript
{
    PrepareAuraScript(spell_martyrdom_pws_remove_AuraScript);

    void HandleRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        Unit* target = GetTarget();
        if (!target)
            return;

        // Self-cast only: shield must be on the caster
        if (GetCasterGUID() != target->GetGUID())
            return;

        Player* player = target->ToPlayer();
        if (!player)
            return;

        if (!player->HasAura(MARTYRDOM_R1) && !player->HasAura(MARTYRDOM_R2))
            return;

        player->CastSpell(player, MARTYRS_RESOLVE, true);
    }

    void Register() override
    {
        AfterEffectRemove += AuraEffectRemoveFn(
            spell_martyrdom_pws_remove_AuraScript::HandleRemove,
            EFFECT_0, SPELL_AURA_SCHOOL_ABSORB, AURA_EFFECT_HANDLE_REAL);
    }
};

class spell_martyrdom_pws_remove_loader : public SpellScriptLoader
{
public:
    spell_martyrdom_pws_remove_loader() : SpellScriptLoader("spell_martyrdom_pws_remove") { }

    AuraScript* GetAuraScript() const override
    {
        return new spell_martyrdom_pws_remove_AuraScript();
    }
};

void AddSC_martyrdom()
{
    new spell_martyrdom_pws_remove_loader();
}
