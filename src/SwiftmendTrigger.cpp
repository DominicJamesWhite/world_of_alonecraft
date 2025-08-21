#include "ScriptMgr.h"
#include "SpellScript.h"
#include "Player.h"

class spell_swiftmend_trigger : public SpellScript
{
    PrepareSpellScript(spell_swiftmend_trigger);

    void HandleOnHit()
    {
        Unit* caster = GetCaster();
        Unit* target = GetHitUnit();

        if (!caster || !target)
            return;

        if (caster->GetGUID() == target->GetGUID())
        {
            caster->CastSpell(caster, 200016, true);
        }
    }

    void Register() override
    {
        OnHit += SpellHitFn(spell_swiftmend_trigger::HandleOnHit);
    }
};

class spell_swiftmend_trigger_loader : public SpellScriptLoader
{
public:
    spell_swiftmend_trigger_loader() : SpellScriptLoader("spell_swiftmend_trigger") { }

    SpellScript* GetSpellScript() const override
    {
        return new spell_swiftmend_trigger();
    }
};

void AddSC_spell_swiftmend_trigger()
{
    new spell_swiftmend_trigger_loader();
}
