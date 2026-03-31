#include "ScriptMgr.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "SpellScriptLoader.h"
#include "Unit.h"

// Shadow Affinity (redesigned)
// Effect 1: MOD_HEALING_DONE_PCT (passive, handled by DBC aura)
// Effect 2: DUMMY proc — Shadow damage restores mana equal to X% of damage dealt
// Rank 1: 5%  (spell 15318, BasePoints2 = 4)
// Rank 2: 10% (spell 15272, BasePoints2 = 9)
// Rank 3: 15% (spell 15320, BasePoints2 = 14)

enum ShadowAffinitySpells
{
    SHADOW_AFFINITY_MANA = 200101,
    SW_DEATH_SELF        = 32409,
};

class spell_shadow_affinity_proc : public AuraScript
{
    PrepareAuraScript(spell_shadow_affinity_proc);

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SHADOW_AFFINITY_MANA });
    }

    bool CheckProc(ProcEventInfo& eventInfo)
    {
        if (!eventInfo.GetSpellInfo() || !eventInfo.GetDamageInfo())
            return false;

        if (eventInfo.GetDamageInfo()->GetDamage() == 0)
            return false;

        uint32 schoolMask = eventInfo.GetSpellInfo()->GetSchoolMask();

        if (!(schoolMask & SPELL_SCHOOL_MASK_SHADOW))
            return false;

        // Ignore SW:Death self-damage
        if (eventInfo.GetSpellInfo()->Id == SW_DEATH_SELF &&
            eventInfo.GetActionTarget() == eventInfo.GetActor())
            return false;

        return true;
    }

    void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();

        Unit* caster = GetTarget();
        if (!caster)
            return;

        DamageInfo* damageInfo = eventInfo.GetDamageInfo();
        if (!damageInfo)
            return;

        uint32 damage = damageInfo->GetDamage();
        if (damage == 0)
            return;

        int32 pct = aurEff->GetAmount();
        int32 amount = CalculatePct(static_cast<int32>(damage), pct);
        if (amount <= 0)
            return;

        caster->CastCustomSpell(SHADOW_AFFINITY_MANA, SPELLVALUE_BASE_POINT0, amount, caster, true, nullptr, aurEff);
    }

    void Register() override
    {
        DoCheckProc += AuraCheckProcFn(spell_shadow_affinity_proc::CheckProc);
        OnEffectProc += AuraEffectProcFn(spell_shadow_affinity_proc::HandleProc, EFFECT_1, SPELL_AURA_DUMMY);
    }
};

class spell_shadow_affinity_proc_loader : public SpellScriptLoader
{
public:
    spell_shadow_affinity_proc_loader() : SpellScriptLoader("spell_shadow_affinity_proc") { }

    AuraScript* GetAuraScript() const override
    {
        return new spell_shadow_affinity_proc();
    }
};

void AddSC_shadow_affinity()
{
    new spell_shadow_affinity_proc_loader();
}
