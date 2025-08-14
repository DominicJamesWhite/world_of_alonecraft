#include "ScriptMgr.h"
#include "SpellScript.h"        // defines SpellScript, AuraScript, SpellScriptLoader
#include "SpellAuraEffects.h"   // aura helpers
#include "Unit.h"

// AuraScript that allows proc only when caster's HoT heals themselves
class aura_proc_self_hot_only : public AuraScript
{
    PrepareAuraScript(aura_proc_self_hot_only);

    bool CheckProc(ProcEventInfo& eventInfo)
    {
        Unit* actor = eventInfo.GetActor();              // the healer (the aura owner in this use-case)
        Unit* healed = eventInfo.GetActionTarget();      // who got healed
        SpellInfo const* trigger = eventInfo.GetSpellInfo(); // the spell that caused the proc
        SpellInfo const* auraInfo = GetSpellInfo();      // this aura's SpellInfo

        if (!actor || !healed || !trigger || !auraInfo)
            return false;

        // Limit this script to your specific proc aura IDs
        switch (auraInfo->Id)
        {
            case 200006:
            case 200007:
            case 200008:
            case 200009:
            case 200010:
                break;
            default:
                return false;
        }

        // Only trigger if the HoT heal landed on the caster (self-heal)
        if (actor->GetGUID() != healed->GetGUID())
            return false;

        return true; // allow the proc
    }

    void Register() override
    {
        DoCheckProc += AuraCheckProcFn(aura_proc_self_hot_only::CheckProc);
    }
};

// Loader so the name in `spell_script_names` can attach this AuraScript
class aura_proc_self_hot_only_loader : public SpellScriptLoader
{
public:
    aura_proc_self_hot_only_loader() : SpellScriptLoader("aura_proc_self_hot_only") { }

    AuraScript* GetAuraScript() const override
    {
        return new aura_proc_self_hot_only();
    }
};

// Entry point called by your module's loader
void AddSC_aura_proc_self_hot_only()
{
    new aura_proc_self_hot_only_loader();
}
