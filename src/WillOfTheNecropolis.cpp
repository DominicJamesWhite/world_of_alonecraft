#include "ScriptMgr.h"
#include "Player.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "SpellScriptLoader.h"
#include "Unit.h"

// Will of the Necropolis Extension (talent: 49189 / 50149 / 50150)
// Heart Strike additionally grants a stacking parry buff (200109).
// One stack per Heart Strike cast (not per target hit).

enum WillOfTheNecropolisSpells
{
    // The talent learn-spells (49189/50149/50150) are SPELL_EFFECT_DUMMY +
    // LEARN_SPELL, NOT passive auras. The actually applied auras are 52284-52286.
    WILL_AURA_R1  = 52284,
    WILL_AURA_R2  = 52285,
    WILL_AURA_R3  = 52286,
    PARRY_BUFF    = 200109,
};

class spell_dk_will_of_necropolis_parry_SpellScript : public SpellScript
{
    PrepareSpellScript(spell_dk_will_of_necropolis_parry_SpellScript);

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ PARRY_BUFF });
    }

    bool Load() override
    {
        _executed = false;
        return GetCaster()->IsPlayer();
    }

    void HandleAfterHit()
    {
        if (_executed)
            return;

        Unit* caster = GetCaster();
        if (!caster || !GetHitUnit())
            return;

        // Check for any rank of Will of the Necropolis (applied absorb auras)
        if (!caster->HasAura(WILL_AURA_R1) && !caster->HasAura(WILL_AURA_R2) && !caster->HasAura(WILL_AURA_R3))
            return;

        _executed = true;

        // Apply or refresh stacking parry buff
        if (Aura* existing = caster->GetAura(PARRY_BUFF))
        {
            uint8 maxStacks = existing->GetSpellInfo()->StackAmount;
            if (existing->GetStackAmount() < maxStacks)
                existing->ModStackAmount(1);
            existing->RefreshDuration();
        }
        else
        {
            caster->CastSpell(caster, PARRY_BUFF, true);
        }
    }

    void Register() override
    {
        AfterHit += SpellHitFn(spell_dk_will_of_necropolis_parry_SpellScript::HandleAfterHit);
    }

    bool _executed;
};

class spell_dk_will_of_necropolis_parry_loader : public SpellScriptLoader
{
public:
    spell_dk_will_of_necropolis_parry_loader() : SpellScriptLoader("spell_dk_will_of_necropolis_parry") { }

    SpellScript* GetSpellScript() const override
    {
        return new spell_dk_will_of_necropolis_parry_SpellScript();
    }
};

void AddSC_dk_will_of_necropolis_parry()
{
    new spell_dk_will_of_necropolis_parry_loader();
}
