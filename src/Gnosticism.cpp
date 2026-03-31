#include "ScriptMgr.h"
#include "Player.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "SpellScriptLoader.h"
#include "Unit.h"

// Gnosticism (renamed from Improved Mana Burn)
// Holy damage heals you for X%. Shadow damage restores mana equal to Y%.
// Rank 1: 5% / 5%   (spell 14750, BasePoints = 4 -> 5%)
// Rank 2: 10% / 10%  (spell 14772, BasePoints = 9 -> 10%)

enum GnosticismSpells
{
    GNOSTICISM_MANA = 200102,
    GNOSTICISM_HEAL = 200103,
    // Shadow Word: Death self-damage spell
    SW_DEATH_SELF   = 32409,
};

class spell_gnosticism_proc : public AuraScript
{
    PrepareAuraScript(spell_gnosticism_proc);

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ GNOSTICISM_MANA, GNOSTICISM_HEAL });
    }

    bool CheckProc(ProcEventInfo& eventInfo)
    {
        if (!eventInfo.GetSpellInfo() || !eventInfo.GetDamageInfo())
            return false;

        if (eventInfo.GetDamageInfo()->GetDamage() == 0)
            return false;

        uint32 schoolMask = eventInfo.GetSpellInfo()->GetSchoolMask();

        // Must be holy or shadow damage
        if (!(schoolMask & (SPELL_SCHOOL_MASK_HOLY | SPELL_SCHOOL_MASK_SHADOW)))
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

        // BasePoints stores pct-1 (AC convention), so amount = BasePoints + 1
        int32 pct = aurEff->GetAmount();
        int32 amount = CalculatePct(static_cast<int32>(damage), pct);
        if (amount <= 0)
            return;

        uint32 schoolMask = eventInfo.GetSpellInfo()->GetSchoolMask();

        if (schoolMask & SPELL_SCHOOL_MASK_HOLY)
        {
            caster->CastCustomSpell(GNOSTICISM_HEAL, SPELLVALUE_BASE_POINT0, amount, caster, true, nullptr, aurEff);
        }
        else if (schoolMask & SPELL_SCHOOL_MASK_SHADOW)
        {
            caster->CastCustomSpell(GNOSTICISM_MANA, SPELLVALUE_BASE_POINT0, amount, caster, true, nullptr, aurEff);
        }
    }

    void Register() override
    {
        DoCheckProc += AuraCheckProcFn(spell_gnosticism_proc::CheckProc);
        OnEffectProc += AuraEffectProcFn(spell_gnosticism_proc::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
    }
};

class spell_gnosticism_proc_loader : public SpellScriptLoader
{
public:
    spell_gnosticism_proc_loader() : SpellScriptLoader("spell_gnosticism_proc") { }

    AuraScript* GetAuraScript() const override
    {
        return new spell_gnosticism_proc();
    }
};

void AddSC_gnosticism()
{
    new spell_gnosticism_proc_loader();
}
