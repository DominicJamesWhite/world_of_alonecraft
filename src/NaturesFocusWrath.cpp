#include "ScriptMgr.h"
#include "SpellScript.h"
#include "Player.h"
#include "Group.h"
#include "Chat.h"

class spell_natures_focus_wrath : public SpellScript
{
    PrepareSpellScript(spell_natures_focus_wrath);

    void HandleAfterCast()
    {
        Unit* caster = GetCaster();
        if (!caster || !caster->ToPlayer())
            return;

        Player* player = caster->ToPlayer();

        Unit::AuraApplicationMap const& auras = caster->GetAppliedAuras();
        for (Unit::AuraApplicationMap::const_iterator itr = auras.begin(); itr != auras.end(); ++itr)
        {
            AuraApplication* aurApp = itr->second;
            Aura* aura = aurApp->GetBase();

            if (aura->GetCasterGUID() != caster->GetGUID())
                continue;

            const SpellInfo* spellInfo = aura->GetSpellInfo();
            if (!spellInfo)
                continue;

            if (spellInfo->SpellFamilyFlags[0] & 0x00000010) // Rejuvenation
            {
                aura->SetDuration(aura->GetMaxDuration());
            }
        }
    }

    void Register() override
    {
        AfterCast += SpellCastFn(spell_natures_focus_wrath::HandleAfterCast);
    }
};

class spell_natures_focus_wrath_loader : public SpellScriptLoader
{
public:
    spell_natures_focus_wrath_loader() : SpellScriptLoader("spell_natures_focus_wrath") { }

    SpellScript* GetSpellScript() const override
    {
        return new spell_natures_focus_wrath();
    }
};

void AddSC_spell_natures_focus_wrath()
{
    new spell_natures_focus_wrath_loader();
}
