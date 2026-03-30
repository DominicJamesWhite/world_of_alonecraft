#include "ScriptMgr.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "SpellScriptLoader.h"
#include "Player.h"
#include "Unit.h"
#include "Common.h"

enum MagicAbsorptionSpells
{
    // Absorb aura spell IDs by rank
    SPELL_MAGIC_ABSORPTION_ABSORB_R1 = 200035, // 25%
    SPELL_MAGIC_ABSORPTION_ABSORB_R2 = 200036  // 50%
};

// Magic Absorption (refactored):
// - Absorbs 25% / 50% of incoming damage based on aura spell ID (200035 / 200036)
// - Procs with chance equal to player's Arcane spell crit chance
// - Large absorb pool so aura persists across many hits
class spell_magic_absorption_absorb_aura : public AuraScript
{
    PrepareAuraScript(spell_magic_absorption_absorb_aura);

    bool Validate(SpellInfo const* spellInfo) override
    {
        if (spellInfo->Id != SPELL_MAGIC_ABSORPTION_ABSORB_R1 && spellInfo->Id != SPELL_MAGIC_ABSORPTION_ABSORB_R2)
            return false;

        return spellInfo->Effects[EFFECT_0].ApplyAuraName == SPELL_AURA_SCHOOL_ABSORB;
    }

    static float GetArcaneSpellCritChance(Player* player)
    {
        // Use Arcane school spell crit chance
        return player->GetFloatValue(PLAYER_SPELL_CRIT_PERCENTAGE1 + static_cast<uint8>(SPELL_SCHOOL_ARCANE));
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

        // Determine percent by aura spell id (200035/200036)
        float pct = 0.0f;
        uint32 auraId = aurEff->GetSpellInfo()->Id;

        if (auraId == SPELL_MAGIC_ABSORPTION_ABSORB_R2) pct = 0.50f;
        else if (auraId == SPELL_MAGIC_ABSORPTION_ABSORB_R1) pct = 0.25f;

        if (pct <= 0.0f)
        {
            absorbAmount = 0;
            return;
        }

        float critChance = GetArcaneSpellCritChance(player);
        if (!roll_chance_f(critChance))
        {
            absorbAmount = 0;
            return;
        }

        uint32 damage = dmgInfo.GetDamage();
        absorbAmount = static_cast<uint32>(float(damage) * pct);
    }

    void Register() override
    {
        DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_magic_absorption_absorb_aura::CalculateAmount, EFFECT_0, SPELL_AURA_SCHOOL_ABSORB);
        OnEffectAbsorb += AuraEffectAbsorbFn(spell_magic_absorption_absorb_aura::Absorb, EFFECT_0);
    }
};

class spell_magic_absorption_absorb_loader : public SpellScriptLoader
{
public:
    spell_magic_absorption_absorb_loader() : SpellScriptLoader("spell_magic_absorption_absorb") { }

    AuraScript* GetAuraScript() const override
    {
        return new spell_magic_absorption_absorb_aura();
    }
};

// Registration
void AddSC_magic_absorption()
{
    new spell_magic_absorption_absorb_loader();
}
