#include "ScriptMgr.h"
#include "Player.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "SpellScriptLoader.h"
#include "Unit.h"
#include "Log.h"

#define ICE_LANCE_HEAL_AURA 200037
#define ICE_LANCE_HEAL_SPELL 200038

class spell_mage_ice_lance_heal : public SpellScript
{
    PrepareSpellScript(spell_mage_ice_lance_heal);

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        bool valid = ValidateSpellInfo({ ICE_LANCE_HEAL_SPELL });
        return valid;
    }

    void HandleOnHit(SpellEffIndex effIndex)
    {
        if (effIndex != EFFECT_0 || GetSpellInfo()->Effects[effIndex].Effect != SPELL_EFFECT_SCHOOL_DAMAGE)
            return;

        Unit* caster = GetCaster();
        if (!caster || !caster->IsPlayer())
            return;

        Unit* target = GetHitUnit();
        if (!target)
            return;

        // Verify this is Ice Lance via flags
        if (GetSpellInfo()->SpellFamilyName != SPELLFAMILY_MAGE ||
            GetSpellInfo()->SpellFamilyFlags[0] != 0x00020000 ||
            GetSpellInfo()->SpellFamilyFlags[1] != 0x00000000 ||
            GetSpellInfo()->SpellFamilyFlags[2] != 0x00000000)
        {
            return;
        }

        // Check for aura 200037 on caster
        Aura* healAura = caster->GetAura(ICE_LANCE_HEAL_AURA);
        if (!healAura)
        {
            return;
        }

        uint8 stacks = healAura->GetStackAmount();
        if (stacks == 0)
        {
            return;
        }

        // Get damage dealt
        int32 damage = GetHitDamage();
        if (damage <= 0)
        {
            return;
        }

        // Compute heal: 300% * damage * stacks
        int32 healAmount = CalculatePct(damage, 300) * stacks;

        if (healAmount > 0)
        {
            caster->CastCustomSpell(ICE_LANCE_HEAL_SPELL, SPELLVALUE_BASE_POINT0, healAmount, caster, true);

            // Remove all stacks of the aura after heal
            if (healAura)
            {
                healAura->Remove();
            }
        }
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_mage_ice_lance_heal::HandleOnHit, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
    }
};

class spell_mage_ice_lance_heal_loader : public SpellScriptLoader
{
public:
    spell_mage_ice_lance_heal_loader() : SpellScriptLoader("spell_mage_ice_lance_heal") { }

    SpellScript* GetSpellScript() const override
    {
        return new spell_mage_ice_lance_heal();
    }
};

void AddSC_ice_lance_heal_mechanic()
{
    new spell_mage_ice_lance_heal_loader();
}
