#include "ScriptMgr.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "SpellScriptLoader.h"
#include "Unit.h"

// Syncretism (NEW Discipline Priest talent)
// After dealing Shadow damage, next Holy spell deals bonus Shadow damage = 50% SP.
// After dealing Holy damage, next Shadow spell deals bonus Holy damage = 50% SP.

enum SyncretismSpells
{
    SYNCRETISM_PASSIVE      = 200094,
    SHADOW_SYNCRETIC_BUFF   = 200097,  // Applied after Shadow dmg; consumed by Holy
    HOLY_SYNCRETIC_BUFF     = 200098,  // Applied after Holy dmg; consumed by Shadow
    SYNCRETISM_SHADOW_DMG   = 200099,  // Hidden instant Shadow damage
    SYNCRETISM_HOLY_DMG     = 200100,  // Hidden instant Holy damage
    SW_DEATH_SELF           = 32409,
};

class spell_syncretism_proc : public AuraScript
{
    PrepareAuraScript(spell_syncretism_proc);

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

        // Don't proc from our own damage spells (prevent infinite loop)
        uint32 spellId = eventInfo.GetSpellInfo()->Id;
        if (spellId == SYNCRETISM_SHADOW_DMG || spellId == SYNCRETISM_HOLY_DMG)
            return false;

        return true;
    }

    void HandleProc(AuraEffect const* /*aurEff*/, ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();

        Unit* caster = GetTarget();
        if (!caster)
            return;

        Unit* target = eventInfo.GetActionTarget();
        if (!target || target == caster)
            return;

        uint32 schoolMask = eventInfo.GetSpellInfo()->GetSchoolMask();
        int32 spellPower = caster->SpellBaseDamageBonusDone(SPELL_SCHOOL_MASK_ALL);
        int32 bonus = CalculatePct(spellPower, 50);

        if (bonus <= 0)
            return;

        if (schoolMask & SPELL_SCHOOL_MASK_SHADOW)
        {
            // Dealt Shadow damage: consume Holy Syncretic buff if present, then apply Shadow Syncretic
            if (caster->HasAura(HOLY_SYNCRETIC_BUFF))
            {
                caster->RemoveAurasDueToSpell(HOLY_SYNCRETIC_BUFF);
                caster->CastCustomSpell(target, SYNCRETISM_HOLY_DMG, &bonus, nullptr, nullptr, true);
            }
            caster->CastSpell(caster, SHADOW_SYNCRETIC_BUFF, true);
        }
        else if (schoolMask & SPELL_SCHOOL_MASK_HOLY)
        {
            // Dealt Holy damage: consume Shadow Syncretic buff if present, then apply Holy Syncretic
            if (caster->HasAura(SHADOW_SYNCRETIC_BUFF))
            {
                caster->RemoveAurasDueToSpell(SHADOW_SYNCRETIC_BUFF);
                caster->CastCustomSpell(target, SYNCRETISM_SHADOW_DMG, &bonus, nullptr, nullptr, true);
            }
            caster->CastSpell(caster, HOLY_SYNCRETIC_BUFF, true);
        }
    }

    void Register() override
    {
        DoCheckProc += AuraCheckProcFn(spell_syncretism_proc::CheckProc);
        OnEffectProc += AuraEffectProcFn(spell_syncretism_proc::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
    }
};

class spell_syncretism_proc_loader : public SpellScriptLoader
{
public:
    spell_syncretism_proc_loader() : SpellScriptLoader("spell_syncretism_proc") { }

    AuraScript* GetAuraScript() const override
    {
        return new spell_syncretism_proc();
    }
};

void AddSC_syncretism()
{
    new spell_syncretism_proc_loader();
}
