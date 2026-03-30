#include "Player.h"
#include "ScriptMgr.h"
#include "Spell.h"
#include "SpellAuraEffects.h"
#include "SpellScript.h"
#include "SpellMgr.h"
#include "SpellScriptLoader.h"

enum ShatteredBarrierSpells
{
    SPELL_SHATTERED_BARRIER = 55080
};

// Invocation spell family flags from MagicAttunement.cpp
enum InvocationFamilyFlags : uint32
{
    SPELL_FAMILY_FLAG_INVOCATION_0 = 0x00002000,
    SPELL_FAMILY_FLAG_INVOCATION_1 = 0x00000000,
    SPELL_FAMILY_FLAG_INVOCATION_2 = 0x00000008
};

class spell_mage_shattered_barrier_cooldown_reset : public AuraScript
{
    PrepareAuraScript(spell_mage_shattered_barrier_cooldown_reset);

public:
    spell_mage_shattered_barrier_cooldown_reset() : AuraScript() { }

    void OnApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        Player* player = GetCaster()->ToPlayer();
        if (!player)
            return;

        // Iterate over the player's spells to find Invocation and reset its cooldown
        for (auto const& spellPair : player->GetSpellMap())
        {
            uint32 spellId = spellPair.first;
            SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId);

            if (spellInfo &&
                spellInfo->SpellFamilyName == SPELLFAMILY_MAGE &&
                spellInfo->SpellFamilyFlags[0] == SPELL_FAMILY_FLAG_INVOCATION_0 &&
                spellInfo->SpellFamilyFlags[1] == SPELL_FAMILY_FLAG_INVOCATION_1 &&
                spellInfo->SpellFamilyFlags[2] == SPELL_FAMILY_FLAG_INVOCATION_2)
            {
                if (player->HasSpellCooldown(spellId))
                {
                    player->RemoveSpellCooldown(spellId, true);
                }
            }
        }
    }

    void Register() override
    {
        OnEffectApply += AuraEffectApplyFn(spell_mage_shattered_barrier_cooldown_reset::OnApply, EFFECT_1, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
    }
};

class spell_mage_shattered_barrier_loader : public SpellScriptLoader
{
public:
    spell_mage_shattered_barrier_loader() : SpellScriptLoader("spell_mage_shattered_barrier_cooldown_reset") { }

    AuraScript* GetAuraScript() const override
    {
        // Hook into Shattered Barrier (55080)
        return new spell_mage_shattered_barrier_cooldown_reset();
    }
};

void AddSC_spell_mage_shattered_barrier_cooldown_reset()
{
    new spell_mage_shattered_barrier_loader();
}
