#include "ScriptMgr.h"
#include "Player.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "SpellScriptLoader.h"
#include "Unit.h"
#include "Log.h"

// Fire Leech - Mage talent that heals for percentage of fire damage dealt
// Spell IDs: 11083 (Rank 1 - 25%), 12351 (Rank 2 - 50%)
class spell_mage_fire_leech : public AuraScript
{
    PrepareAuraScript(spell_mage_fire_leech);

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ 11083, 12351, 200028 });
    }

    bool CheckProc(ProcEventInfo& eventInfo)
    {
        // Verify we have spell info and damage info
        if (!eventInfo.GetSpellInfo() || !eventInfo.GetDamageInfo())
            return false;

        // Only proc from fire spells
        if (!(eventInfo.GetSpellInfo()->GetSchoolMask() & SPELL_SCHOOL_MASK_FIRE))
            return false;

        // Only proc when we actually deal damage (not absorbed/resisted)
        if (eventInfo.GetDamageInfo()->GetDamage() == 0)
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

        // Calculate healing amount based on rank
        int32 pct = 0;
        if (GetAura() && GetAura()->GetSpellInfo())
        {
            uint32 spellId = GetAura()->GetSpellInfo()->Id;
            if (spellId == 11083) // Rank 1
                pct = 25;
            else if (spellId == 12351) // Rank 2
                pct = 50;
        }

        int32 healAmount = CalculatePct(int32(damage), pct);

        if (healAmount > 0)
        {
            caster->CastCustomSpell(200028, SPELLVALUE_BASE_POINT0, healAmount, caster, true, nullptr, aurEff);
        }
    }

    void Register() override
    {
        DoCheckProc += AuraCheckProcFn(spell_mage_fire_leech::CheckProc);
        OnEffectProc += AuraEffectProcFn(spell_mage_fire_leech::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
    }
};

class spell_mage_fire_leech_loader : public SpellScriptLoader
{
public:
    spell_mage_fire_leech_loader() : SpellScriptLoader("spell_mage_fire_leech") { }

    AuraScript* GetAuraScript() const override
    {
        return new spell_mage_fire_leech();
    }
};

// Registration function for the Fire Leech mechanic
void AddSC_fire_leech_mechanic()
{
    new spell_mage_fire_leech_loader();
}
