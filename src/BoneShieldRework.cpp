#include "ScriptMgr.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "SpellScriptLoader.h"
#include "Unit.h"

// Bone Harvest (200117) — hidden passive learned alongside Bone Shield.
// On auto-attack (proc via spell_proc with ICD), adds a charge to
// Bone Shield (49222) if it's active. Bone Shield itself is untouched.
//
// NOTE: Vanilla Bone Shield has a known double-charge-consumption issue.
// The Bone Harvest ICD is tuned to compensate for this.

enum BoneHarvestSpells
{
    SPELL_DK_BONE_SHIELD = 49222,
};

class spell_dk_bone_harvest_AuraScript : public AuraScript
{
    PrepareAuraScript(spell_dk_bone_harvest_AuraScript);

    void HandleProc(ProcEventInfo& /*eventInfo*/)
    {
        PreventDefaultAction();

        Unit* target = GetTarget();
        if (!target)
            return;

        Aura* boneShield = target->GetAura(SPELL_DK_BONE_SHIELD);
        if (!boneShield)
            return;

        uint8 max = boneShield->CalcMaxCharges();
        if (boneShield->GetCharges() >= max)
            return;

        boneShield->ModCharges(1);
    }

    void Register() override
    {
        OnProc += AuraProcFn(spell_dk_bone_harvest_AuraScript::HandleProc);
    }
};

class spell_dk_bone_harvest_loader : public SpellScriptLoader
{
public:
    spell_dk_bone_harvest_loader() : SpellScriptLoader("spell_dk_bone_harvest") { }

    AuraScript* GetAuraScript() const override
    {
        return new spell_dk_bone_harvest_AuraScript();
    }
};

void AddSC_dk_bone_shield_rework()
{
    new spell_dk_bone_harvest_loader();
}
