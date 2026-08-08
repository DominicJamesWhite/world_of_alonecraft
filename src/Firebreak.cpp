#include "ScriptMgr.h"
#include "Player.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "SpellScriptLoader.h"
#include "Unit.h"
#include "Log.h"
#include "EmberScars.h"

using namespace Alonecraft::Mage;

#define FIREBREAK_AURA_R1 11103
#define FIREBREAK_AURA_R2 12357
#define FIREBREAK_AURA_R3 12358

class spell_firebreak_damage_booster : public SpellScript
{
    PrepareSpellScript(spell_firebreak_damage_booster);

public:
    spell_firebreak_damage_booster() : SpellScript() { }

    void OnDamage(SpellEffIndex effIndex)
    {
        PreventHitDefaultEffect(effIndex);

        Unit* caster = GetCaster();
        if (!caster)
            return;

        Player* player = caster->ToPlayer();
        if (!player)
            return;

        Aura* emberScars = player->GetAura(EMBER_SCARS_DOT_ID);
        if (!emberScars)
            return;

        uint8 stacks = emberScars->GetStackAmount();
        if (stacks == 0)
            return;

        float damageBonus = 0.0f;
        if (player->HasAura(FIREBREAK_AURA_R1))
            damageBonus = 0.20f;
        else if (player->HasAura(FIREBREAK_AURA_R2))
            damageBonus = 0.40f;
        else if (player->HasAura(FIREBREAK_AURA_R3))
            damageBonus = 0.60f;

        if (damageBonus == 0.0f)
            return;

        int32 extraDamage = GetHitDamage() * (damageBonus * stacks);
        SetHitDamage(GetHitDamage() + extraDamage);
    }

    // Remove one Ember Scars stack when Fire Blast lands (replaces PlayerScript)
    void HandleAfterCast()
    {
        Unit* caster = GetCaster();
        if (!caster)
        {
            LOG_DEBUG("scripts", "Firebreak::HandleAfterCast - no caster");
            return;
        }

        Player* player = caster->ToPlayer();
        if (!player)
            return;

        if (!(player->HasAura(FIREBREAK_AURA_R1) || player->HasAura(FIREBREAK_AURA_R2) || player->HasAura(FIREBREAK_AURA_R3)))
            return;

        RemoveEmberScarsStacks(player, 1);
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_firebreak_damage_booster::OnDamage, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
        AfterCast += SpellCastFn(spell_firebreak_damage_booster::HandleAfterCast);
    }
};

class spell_firebreak_loader : public SpellScriptLoader
{
    public:
        spell_firebreak_loader() : SpellScriptLoader("spell_firebreak") { }

        SpellScript* GetSpellScript() const override
        {
            return new spell_firebreak_damage_booster();
        }
};

void AddSC_firebreak_mechanic()
{
    new spell_firebreak_loader();
}
