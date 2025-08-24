#include "Player.h"
#include "ScriptMgr.h"
#include "SpellAuraEffects.h"
#include "SpellScript.h"
#include "Unit.h"
#include "SpellMgr.h"
#include "SpellScriptLoader.h"

class spell_dru_lifebloom_mod : public AuraScript
{
    PrepareAuraScript(spell_dru_lifebloom_mod);

    enum DruidSpells
    {
        SPELL_DRUID_LIFEBLOOM_FINAL_HEAL = 33778,
        SPELL_DRUID_LIFEBLOOM_ENERGIZE   = 64372
    };

    void AfterRemove(AuraEffect const* aurEff, AuraEffectHandleModes /*mode*/)
    {
        AuraRemoveMode removeMode = GetTargetApplication()->GetRemoveMode();

        if (removeMode != AURA_REMOVE_BY_EXPIRE)
        {
            return;
        }

        Unit* caster = GetCaster();
        Unit* target = GetTarget();

        if (!caster || !target)
        {
            return;
        }

        int32 stack = GetStackAmount();
        int32 healAmount = aurEff->GetAmount();
        SpellInfo const* finalHeal = sSpellMgr->GetSpellInfo(SPELL_DRUID_LIFEBLOOM_FINAL_HEAL);

        healAmount = caster->SpellHealingBonusDone(target, finalHeal, healAmount, HEAL, aurEff->GetEffIndex(), 0.0f, stack);
        healAmount = target->SpellHealingBonusTaken(caster, finalHeal, healAmount, HEAL, stack);

        int32 damageAmount = 0;
        if (caster->HasAura(51183))
            damageAmount = healAmount;
        else if (caster->HasAura(51182))
            damageAmount = healAmount * 0.8f;
        else if (caster->HasAura(51181))
            damageAmount = healAmount * 0.6f;
        else if (caster->HasAura(51180))
            damageAmount = healAmount * 0.4f;
        else if (caster->HasAura(51179))
            damageAmount = healAmount * 0.2f;

        if (caster == target && damageAmount > 0)
        {
            target->CastCustomSpell(target, SPELL_DRUID_LIFEBLOOM_FINAL_HEAL, &healAmount, &damageAmount, nullptr, true, nullptr, aurEff, GetCasterGUID());
        }
        else
        {
            target->CastCustomSpell(target, SPELL_DRUID_LIFEBLOOM_FINAL_HEAL, &healAmount, nullptr, nullptr, true, nullptr, aurEff, GetCasterGUID());
        }

        // restore mana
        int32 returnmana = (GetSpellInfo()->ManaCostPercentage * caster->GetCreateMana() / 100) * stack / 2;
        caster->CastCustomSpell(caster, SPELL_DRUID_LIFEBLOOM_ENERGIZE, &returnmana, nullptr, nullptr, true, nullptr, aurEff, GetCasterGUID());
    }

    void Register() override
    {
        AfterEffectRemove += AuraEffectRemoveFn(spell_dru_lifebloom_mod::AfterRemove, EFFECT_1, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
    }
};

class mod_lifebloom_loader : public SpellScriptLoader
{
public:
    mod_lifebloom_loader() : SpellScriptLoader("spell_dru_lifebloom") { }

    AuraScript* GetAuraScript() const override
    {
        return new spell_dru_lifebloom_mod();
    }
};

void AddSC_mod_lifebloom()
{
    new mod_lifebloom_loader();
}
