#include "ScriptMgr.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "Unit.h"

enum CustomSpells
{
    SPELL_PROC_1 = 17123,
    SPELL_PROC_2 = 17124
};

// AuraScript that only allows proc if the target has the caster's Moonfire aura
class aura_proc_moonfire_only : public AuraScript
{
    PrepareAuraScript(aura_proc_moonfire_only);

    bool CheckProc(ProcEventInfo& eventInfo)
    {
        Unit* caster = GetCaster();
        Unit* target = eventInfo.GetProcTarget();
        SpellInfo const* auraInfo = GetSpellInfo();

        if (!caster || !target || !auraInfo)
            return false;

        // Restrict to your specific proc auras
        switch (auraInfo->Id)
        {
            case SPELL_PROC_1:
            case SPELL_PROC_2:
                break;
            default:
                return false;
        }

        // Look through target’s auras for a caster-owned Moonfire
        for (auto const& auraPair : target->GetAppliedAuras())
        {
            AuraApplication* auraApp = auraPair.second;
            if (!auraApp)
                continue;

            Aura* aura = auraApp->GetBase();
            if (!aura)
                continue;

            SpellInfo const* info = aura->GetSpellInfo();
            if (!info)
                continue;

            // Must be from this caster
            if (aura->GetCasterGUID() != caster->GetGUID())
                continue;

            // Moonfire check by family flags
            // Druid = SpellFamilyName 7, Moonfire = SpellFamilyFlags0 bit 0x40
            if (info->SpellFamilyName == SPELLFAMILY_DRUID && (info->SpellFamilyFlags[0] & 0x00000002))
                return true;
        }

        return false; // no Moonfire found
    }

    void Register() override
    {
        DoCheckProc += AuraCheckProcFn(aura_proc_moonfire_only::CheckProc);
    }
};

// Loader for DB binding (spell_script_names)
class aura_proc_moonfire_only_loader : public SpellScriptLoader
{
public:
    aura_proc_moonfire_only_loader() : SpellScriptLoader("aura_proc_moonfire_only") { }

    AuraScript* GetAuraScript() const override
    {
        return new aura_proc_moonfire_only();
    }
};

// Entry point called by your module’s loader (ScriptLoader.cpp)
void AddSC_aura_proc_moonfire_only()
{
    new aura_proc_moonfire_only_loader();
}
