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

        // Iterate over applied auras on the caster
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

            // Only count auras cast by the caster on themselves
            if (aura->GetCasterGUID() != caster->GetGUID())
                continue;

            // Lifebloom:   FamilyFlags[0] bit 0x0000000000000400
            // Regrowth:    FamilyFlags[0] bit 0x0000000000000010
            // Rejuvenation:FamilyFlags[0] bit 0x0000000000000004
            // Wild Growth: FamilyFlags[1] bit 0x0000000000000020
            if ((info->SpellFamilyFlags[0] & 0x0000000000000400ULL) || // Lifebloom
                (info->SpellFamilyFlags[0] & 0x0000000000000010ULL) || // Regrowth
                (info->SpellFamilyFlags[0] & 0x0000000000000004ULL) || // Rejuvenation
                (info->SpellFamilyFlags[1] & 0x0000000000000020ULL))   // Wild Growth
            {
                bonusCount++;
            }
        }

        // Apply damage multiplier (+25% per HoT)
        float multiplier = 1.0f + (0.25f * bonusCount);
        SetHitDamage(int32(GetHitDamage() * multiplier));
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
