#include "ScriptMgr.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "SpellScriptLoader.h"
#include "Player.h"
#include "Unit.h"
#include "Common.h"

// Spell family flags for Frost/Ice Armor (same flag used by both)
#define FROST_ICE_ARMOR_FAMILY_FLAG 0x02080000

enum PermafrostSpells
{
    SPELL_PERMAFROST_R1 = 11175,
    SPELL_PERMAFROST_R2 = 12569,
    SPELL_PERMAFROST_R3 = 12571,
    // Absorb aura spell IDs by rank
    SPELL_PERMAFROST_ABSORB_R1 = 200032,
    SPELL_PERMAFROST_ABSORB_R2 = 200033,
    SPELL_PERMAFROST_ABSORB_R3 = 200034
};

// Permafrost Absorb:
// - Absorbs 30/60/90% of incoming damage based on Permafrost ranks (R1/R2/R3)
// - Procs with chance equal to (player spell power / player level)
class spell_permafrost_absorb_aura : public AuraScript
{
    PrepareAuraScript(spell_permafrost_absorb_aura);

    bool Validate(SpellInfo const* spellInfo) override
    {
        if (spellInfo->Id != SPELL_PERMAFROST_ABSORB_R1 && spellInfo->Id != SPELL_PERMAFROST_ABSORB_R2 && spellInfo->Id != SPELL_PERMAFROST_ABSORB_R3)
            return false;

        for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
            if (spellInfo->Effects[i].ApplyAuraName == SPELL_AURA_SCHOOL_ABSORB)
                return true;

        return false;
    }

    static float GetPermafrostProcChance(Player* player)
    {
        if (!player)
            return 0.0f;

        // Use player's overall magical spell power
        float sp = player->SpellBaseDamageBonusDone(SPELL_SCHOOL_MASK_MAGIC);

        uint32 level = player->GetLevel();
        if (level == 0)
            return 0.0f;

        float chance = sp / float(level);
        if (chance < 0.0f) chance = 0.0f;
        if (chance > 100.0f) chance = 100.0f;
        return chance;
    }

    // Check if the unit currently has Frost or Ice Armor by spell family flag
    static bool HasFrostOrIceArmor(Unit* unit)
    {
        if (!unit)
            return false;

        // Frost/Ice Armor has a resistance effect; scan those effects and match by family flag
        Unit::AuraEffectList const& resists = unit->GetAuraEffectsByType(SPELL_AURA_MOD_RESISTANCE);
        for (AuraEffect const* eff : resists)
        {
            SpellInfo const* info = eff->GetSpellInfo();
            if (!info)
                continue;

            if (info->SpellFamilyName == SPELLFAMILY_MAGE && (info->SpellFamilyFlags[0] & FROST_ICE_ARMOR_FAMILY_FLAG))
                return true;
        }

        return false;
    }


    // Ensure the absorb aura has a very large pool so it doesn't get consumed after the first hit
    void CalculateAmount(AuraEffect const* /*aurEff*/, int32& amount, bool& canBeRecalculated)
    {
        canBeRecalculated = false;
        amount = 2000000000; // ~2B absorb pool
    }

    void Absorb(AuraEffect* aurEff, DamageInfo& dmgInfo, uint32& absorbAmount)
    {
        Unit* target = GetTarget();
        if (!target)
        {
            absorbAmount = 0;
            return;
        }

        Player* player = target->ToPlayer();
        if (!player)
        {
            absorbAmount = 0;
            return;
        }

        // Only apply when Frost or Ice Armor is active
        if (!HasFrostOrIceArmor(player))
        {
            absorbAmount = 0;
            return;
        }

        float pct = 0.0f;
        uint32 auraId = aurEff->GetSpellInfo()->Id;

        if (auraId == SPELL_PERMAFROST_ABSORB_R3) pct = 0.90f;
        else if (auraId == SPELL_PERMAFROST_ABSORB_R2) pct = 0.60f;
        else if (auraId == SPELL_PERMAFROST_ABSORB_R1) pct = 0.30f;

        if (pct <= 0.0f)
        {
            absorbAmount = 0;
            return;
        }

        float chance = GetPermafrostProcChance(player);
        if (!roll_chance_f(chance))
        {
            absorbAmount = 0;
            return;
        }

        uint32 damage = dmgInfo.GetDamage();
        absorbAmount = static_cast<uint32>(float(damage) * pct);
    }

    void Register() override
    {
        // Ensure the absorb aura has a very large pool (avoid consuming/removing after first hit)
        DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_permafrost_absorb_aura::CalculateAmount, EFFECT_0, SPELL_AURA_SCHOOL_ABSORB);
        DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_permafrost_absorb_aura::CalculateAmount, EFFECT_1, SPELL_AURA_SCHOOL_ABSORB);
        DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_permafrost_absorb_aura::CalculateAmount, EFFECT_2, SPELL_AURA_SCHOOL_ABSORB);

        // Hook absorb on any absorb effect index (0..2)
        OnEffectAbsorb += AuraEffectAbsorbFn(spell_permafrost_absorb_aura::Absorb, EFFECT_0);
        OnEffectAbsorb += AuraEffectAbsorbFn(spell_permafrost_absorb_aura::Absorb, EFFECT_1);
        OnEffectAbsorb += AuraEffectAbsorbFn(spell_permafrost_absorb_aura::Absorb, EFFECT_2);
    }
};

class spell_permafrost_absorb_loader : public SpellScriptLoader
{
public:
    spell_permafrost_absorb_loader() : SpellScriptLoader("spell_permafrost_absorb") { }

    AuraScript* GetAuraScript() const override
    {
        return new spell_permafrost_absorb_aura();
    }
};

// Registration
void AddSC_permafrost_absorb()
{
    new spell_permafrost_absorb_loader();
}
