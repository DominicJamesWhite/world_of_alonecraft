#include "Player.h"
#include "ScriptMgr.h"
#include "Spell.h"
#include "SpellAuraEffects.h"
#include "SpellScript.h"
#include "Log.h"
#include "Unit.h"
#include "SpellMgr.h"
#include "SpellScriptLoader.h"

enum MageAegisEffects
{
    SPELL_ARCANE_AEGIS_OF_ANTONIDAS = 200025,
    SPELL_ICE_AEGIS_OF_ANTONIDAS    = 200026,
    SPELL_MOLTEN_AEGIS_OF_ANTONIDAS = 200027
};

// Mage armor spell family flags
enum MageArmorFamilyFlags : uint32
{
    SPELL_FAMILY_FLAG_MOLTEN_ARMOR = 0x00040000,
    SPELL_FAMILY_FLAG_ICE_ARMOR    = 0x02080000, // This also covers Frost Armor
    SPELL_FAMILY_FLAG_MAGE_ARMOR   = 0x10000000
};

class spell_mage_aegis_of_antonidas : public SpellScript
{
    PrepareSpellScript(spell_mage_aegis_of_antonidas);

    void HandleAfterCast()
    {
        Unit* caster = GetCaster();

        for (auto const& auraPair : caster->GetAppliedAuras())
        {
            if (AuraApplication const* aurApp = auraPair.second)
            {
                if (Aura* aura = aurApp->GetBase())
                {
                    if (SpellInfo const* spellInfo = aura->GetSpellInfo())
                    {
                        if (spellInfo->SpellFamilyFlags[0] & SPELL_FAMILY_FLAG_MOLTEN_ARMOR)
                        {
                            caster->CastSpell(caster, SPELL_MOLTEN_AEGIS_OF_ANTONIDAS, true);
                            return;
                        }

                        if (spellInfo->SpellFamilyFlags[0] & SPELL_FAMILY_FLAG_ICE_ARMOR)
                        {
                            caster->CastSpell(caster, SPELL_ICE_AEGIS_OF_ANTONIDAS, true);
                            return;
                        }

                        if (spellInfo->SpellFamilyFlags[0] & SPELL_FAMILY_FLAG_MAGE_ARMOR)
                        {
                            caster->CastSpell(caster, SPELL_ARCANE_AEGIS_OF_ANTONIDAS, true);
                            return;
                        }
                    }
                }
            }
        }

    }

    void Register() override
    {
        AfterCast += SpellCastFn(spell_mage_aegis_of_antonidas::HandleAfterCast);
    }
};

class spell_mage_aegis_of_antonidas_loader : public SpellScriptLoader
{
public:
    spell_mage_aegis_of_antonidas_loader() : SpellScriptLoader("spell_mage_aegis_of_antonidas") { }

    SpellScript* GetSpellScript() const override
    {
        return new spell_mage_aegis_of_antonidas();
    }
};

void AddSC_spell_mage_aegis_of_antonidas()
{
    new spell_mage_aegis_of_antonidas_loader();
}
