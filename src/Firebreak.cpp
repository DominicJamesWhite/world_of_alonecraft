#include "ScriptMgr.h"
#include "Player.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "SpellScriptLoader.h"
#include "Unit.h"
#include "Log.h"

#define FIREBREAK_AURA_R1 11103
#define FIREBREAK_AURA_R2 12357
#define FIREBREAK_AURA_R3 12358
#define EMBER_SCARS_DOT_ID 200023

void RemoveEmberScarsStack(Player* player, uint8 stacksToRemove)
{
    Aura* emberScars = player->GetAura(EMBER_SCARS_DOT_ID);
    if (!emberScars || emberScars->GetStackAmount() == 0)
        return;

    AuraEffect* effect = emberScars->GetEffect(EFFECT_0);
    if (!effect)
        return;

    uint32 currentTickDamage = effect->GetAmount();
    uint8 currentStacks = emberScars->GetStackAmount();

    if (currentStacks < stacksToRemove)
    {
        stacksToRemove = currentStacks;
    }

    uint32 damageReductionPercent = stacksToRemove * 20;
    uint32 newTickDamage = (currentTickDamage * (100 - damageReductionPercent)) / 100;
    uint8 newStacks = currentStacks - stacksToRemove;

    if (newStacks == 0 || newTickDamage == 0)
    {
        emberScars->Remove();
        return;
    }

    emberScars->SetStackAmount(newStacks);
    effect->ChangeAmount(newTickDamage);
}

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

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_firebreak_damage_booster::OnDamage, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
    }
};

class spell_firebreak_spell_cast_handler : public PlayerScript
{
public:
    spell_firebreak_spell_cast_handler() : PlayerScript("spell_firebreak_spell_cast_handler") { }

    void OnPlayerSpellCast(Player* player, Spell* spell, bool skipCheck) override
    {
        if (!(player->HasAura(FIREBREAK_AURA_R1) || player->HasAura(FIREBREAK_AURA_R2) || player->HasAura(FIREBREAK_AURA_R3)))
            return;

        SpellInfo const* spellInfo = spell->GetSpellInfo();
        if (spellInfo->SpellFamilyName == SPELLFAMILY_MAGE && spellInfo->SpellFamilyFlags[0] & 0x00000002)
        {
            RemoveEmberScarsStack(player, 1);
        }
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
    new spell_firebreak_spell_cast_handler();
}
