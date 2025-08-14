#include "ScriptMgr.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "Unit.h"

class spell_bloomstrike : public SpellScript
{
    PrepareSpellScript(spell_bloomstrike);

    void HandleOnHit()
    {
        Unit* caster = GetCaster();
        Unit* target = GetHitUnit();

        if (!caster || !target)
            return;

        uint32 bonusCount = 0;
        int32 initialDamage = GetHitDamage(); // Store initial damage

        // Iterate over applied auras on the target
        for (auto const& auraPair : caster->GetAppliedAuras())
        {
            AuraApplication* auraApp = auraPair.second;
            if (!auraApp)
                continue;

            Aura* aura = auraApp->GetBase();
            if (!aura)
                continue;

            const SpellInfo* info = aura->GetSpellInfo();
            if (!info)
                continue;

            // Only count auras cast by the Bloomstrike caster on the target
            if (aura->GetCasterGUID() != caster->GetGUID())
                continue;

            // Check if the aura is a periodic heal effect
            bool isPeriodicHeal = false;
            for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
            {
                if (info->Effects[i].ApplyAuraName == SPELL_AURA_PERIODIC_HEAL)
                {
                    isPeriodicHeal = true;
                    break;
                }
            }

            // Check for specific HoT spells by their SpellFamilyFlags individually
            if (isPeriodicHeal)
            {
                if (info->SpellFamilyFlags[1] & 0x00000010) // Lifebloom
                {
                    bonusCount++;
                }
                if (info->SpellFamilyFlags[0] & 0x00000010) // Rejuvenation
                {
                    bonusCount++;
                }
                if (info->SpellFamilyFlags[0] & 0x00000040) // Regrowth
                {
                    bonusCount++;
                }
                if (info->SpellFamilyFlags[1] & 0x04000000) // Wild Growth
                {
                    bonusCount++;
                }
            }
        }

        // Apply damage multiplier (+25% per HoT)
        float multiplier = 1.0f + (0.25f * bonusCount);
        int32 finalDamage = int32(initialDamage * multiplier);
        SetHitDamage(finalDamage);
    }

    void Register() override
    {
        OnHit += SpellHitFn(spell_bloomstrike::HandleOnHit);
    }
};

class spell_bloomstrike_loader : public SpellScriptLoader
{
public:
    spell_bloomstrike_loader() : SpellScriptLoader("spell_bloomstrike") { }

    SpellScript* GetSpellScript() const override
    {
        return new spell_bloomstrike();
    }
};

void AddSC_spell_bloomstrike()
{
    new spell_bloomstrike_loader();
}
