#include "ScriptMgr.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "Unit.h"

// Priest spell constants
enum PriestSpells
{
    SPELL_PRIEST_SHADOW_WORD_DEATH = 32409
};

enum PriestSpellIcons
{
    PRIEST_ICON_ID_PAIN_AND_SUFFERING = 2874
};

// Killing Word - Shadow Word: Death damage bonus based on DoT effects on target
class spell_killing_word_shadow_word_death : public SpellScript
{
    PrepareSpellScript(spell_killing_word_shadow_word_death);

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        // Killing Word ranks and self-damage reducers (extra auras) + PW:S ranks required for reduction
        return ValidateSpellInfo({ 14523, 14784, 14785, 14748, 14768, 14769, 17, 592, 600, 3747, 6065, 6066, 10898, 10899, 10900, 10901, 25217, 25218, 27607, 48065, 48066 }); // Include extra auras and PW:S ranks required for reduction
    }

    void HandleOnHit()
    {
        Unit* caster = GetCaster();
        Unit* target = GetHitUnit();

        if (!caster || !target)
            return;

        int32 initialDamage = GetHitDamage();
        int32 finalDamage = initialDamage;

        // Check if caster has any rank of Killing Word
        uint32 killingWordRank = 0;
        if (caster->HasAura(14523)) // Rank 1
            killingWordRank = 1;
        else if (caster->HasAura(14784)) // Rank 2
            killingWordRank = 2;
        else if (caster->HasAura(14785)) // Rank 3
            killingWordRank = 3;

        // Apply Killing Word bonus if present
        if (killingWordRank > 0)
        {
            uint32 dotCount = 0;

            // Iterate over applied auras on the target
            for (auto const& auraPair : target->GetAppliedAuras())
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

                // Only count priest family spells
                if (info->SpellFamilyName != SPELLFAMILY_PRIEST)
                    continue;

                // Check for specific DoT spells by their SpellFamilyFlags
                bool isTargetDoT = false;

                // Shadow Word: Pain (flag [0] 0x00008000 [1] 0x00000000 [2] 0x00000400)
                if ((info->SpellFamilyFlags[0] & 0x00008000) && (info->SpellFamilyFlags[2] & 0x00000400))
                {
                    isTargetDoT = true;
                }
                // Mark of Penitence (flag [0] 0x00000000 [1] 0x00000000 [2] 0x04000000)
                else if (info->SpellFamilyFlags[2] & 0x04000000)
                {
                    isTargetDoT = true;
                }
                // Holy Fire (flag [0] 0x00100000 [1] 0x00000000 [2] 0x00000400)
                else if ((info->SpellFamilyFlags[0] & 0x00100000) && (info->SpellFamilyFlags[2] & 0x00000400))
                {
                    isTargetDoT = true;
                }
                // Devouring Plague (flag [0] 0x02000000 [1] 0x00001000 [2] 0x00000400)
                else if ((info->SpellFamilyFlags[0] & 0x02000000) && (info->SpellFamilyFlags[1] & 0x00001000) && (info->SpellFamilyFlags[2] & 0x00000400))
                {
                    isTargetDoT = true;
                }

                if (isTargetDoT)
                {
                    dotCount++;
                }
            }

            if (dotCount > 0)
            {
                // Calculate damage multiplier based on Killing Word rank
                float bonusPerDoT = 0.0f;
                switch (killingWordRank)
                {
                    case 1: bonusPerDoT = 0.33f; break; // 33% per DoT
                    case 2: bonusPerDoT = 0.66f; break; // 66% per DoT
                    case 3: bonusPerDoT = 1.00f; break; // 100% per DoT
                }

                // Apply damage multiplier
                float multiplier = 1.0f + (bonusPerDoT * dotCount);
                finalDamage = int32(initialDamage * multiplier);
            }
        }

        // Apply the final damage (either original or modified by Killing Word)
        SetHitDamage(finalDamage);

        // Replicate original Shadow Word: Death functionality
        // Apply self-damage to caster (reduced by Pain and Suffering if present)
        int32 selfDamage = finalDamage;

        // Pain and Suffering reduces self-damage
        if (AuraEffect* aurEff = caster->GetDummyAuraEffect(SPELLFAMILY_PRIEST, PRIEST_ICON_ID_PAIN_AND_SUFFERING, EFFECT_1))
        {
            AddPct(selfDamage, aurEff->GetAmount());
        }

        // Additional auras that reduce Shadow Word: Death self-damage (replicates Pain and Suffering behavior)
        // Requirement: Only apply if the caster currently has Power Word: Shield (any rank).
        // PW:S spell IDs: 17, 592, 600, 3747, 6065, 6066, 10898, 10899, 10900, 10901, 25217, 25218, 27607, 48065, 48066
        bool hasPowerWordShield =
            caster->HasAura(17)   || caster->HasAura(592)  || caster->HasAura(600)  || caster->HasAura(3747) ||
            caster->HasAura(6065) || caster->HasAura(6066) || caster->HasAura(10898)|| caster->HasAura(10899)||
            caster->HasAura(10900)|| caster->HasAura(10901)|| caster->HasAura(25217)|| caster->HasAura(25218)||
            caster->HasAura(27607)|| caster->HasAura(48065)|| caster->HasAura(48066);

        if (hasPowerWordShield)
        {
            // 14748, 14768 and 14769 each have a Dummy aura effect with the correct percentage.
            for (uint32 spellId : { 14748u, 14768u, 14769u })
            {
                if (AuraEffect* extra = caster->GetAuraEffect(spellId, EFFECT_0))
                {
                    AddPct(selfDamage, extra->GetAmount());
                }
            }
        }

        // Cast self-damage spell on caster
        caster->CastCustomSpell(caster, SPELL_PRIEST_SHADOW_WORD_DEATH, &selfDamage, 0, 0, true);
    }

    void Register() override
    {
        OnHit += SpellHitFn(spell_killing_word_shadow_word_death::HandleOnHit);
    }
};

class spell_killing_word_shadow_word_death_loader : public SpellScriptLoader
{
public:
    spell_killing_word_shadow_word_death_loader() : SpellScriptLoader("spell_killing_word_shadow_word_death") { }

    SpellScript* GetSpellScript() const override
    {
        return new spell_killing_word_shadow_word_death();
    }
};

void AddSC_spell_killing_word()
{
    new spell_killing_word_shadow_word_death_loader();
}
