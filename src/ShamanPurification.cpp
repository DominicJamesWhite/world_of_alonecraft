#include "ScriptMgr.h"
#include "Player.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "SpellScriptLoader.h"
#include "Unit.h"

// Purification (extended) — Shaman Restoration talent
// Ranks: 16178 / 16210 / 16211 / 16212 / 16213
// Mechanic: Each tick of an Earthliving Weapon HoT on the caster (self-heal)
// has a chance (by Purification rank) to reset all shock cooldowns.
//
// AuraScript registered on the Earthliving proc heal spells:
//   51945, 51990, 51997, 51998, 51999, 52000

enum PurificationSpells
{
    PURIFICATION_R1 = 16178,
    PURIFICATION_R2 = 16210,
    PURIFICATION_R3 = 16211,
    PURIFICATION_R4 = 16212,
    PURIFICATION_R5 = 16213,

    EARTH_SHOCK_BASE = 8042,
    FLAME_SHOCK_BASE = 8050,
    FROST_SHOCK_BASE = 8056,
};

class spell_sha_purification_shock_reset_AuraScript : public AuraScript
{
    PrepareAuraScript(spell_sha_purification_shock_reset_AuraScript);

    void HandlePeriodic(AuraEffect const* /*aurEff*/)
    {
        Unit* target = GetTarget();   // who the HoT is ticking on
        Unit* caster = GetCaster();   // who cast the Earthliving heal
        if (!target || !caster)
            return;

        // Only proc when the Earthliving HoT is healing the caster themselves
        if (target->GetGUID() != caster->GetGUID())
            return;

        Player* player = caster->ToPlayer();
        if (!player)
            return;

        // Determine chance based on Purification rank
        int32 chance = 0;
        if (player->HasAura(PURIFICATION_R5))
            chance = 20;
        else if (player->HasAura(PURIFICATION_R4))
            chance = 16;
        else if (player->HasAura(PURIFICATION_R3))
            chance = 12;
        else if (player->HasAura(PURIFICATION_R2))
            chance = 8;
        else if (player->HasAura(PURIFICATION_R1))
            chance = 4;

        if (chance <= 0)
            return;

        if (!roll_chance_i(chance))
            return;

        // Reset shock cooldowns (all ranks, like Rime does for Howling Blast)
        uint32 shockBaseSpells[] = { EARTH_SHOCK_BASE, FLAME_SHOCK_BASE, FROST_SHOCK_BASE };
        for (uint32 baseId : shockBaseSpells)
        {
            SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(baseId);
            while (spellInfo)
            {
                player->RemoveSpellCooldown(spellInfo->Id, true);
                spellInfo = spellInfo->GetNextRankSpell();
            }
        }
    }

    void Register() override
    {
        OnEffectPeriodic += AuraEffectPeriodicFn(spell_sha_purification_shock_reset_AuraScript::HandlePeriodic, EFFECT_0, SPELL_AURA_PERIODIC_HEAL);
    }
};

class spell_sha_purification_shock_reset_loader : public SpellScriptLoader
{
public:
    spell_sha_purification_shock_reset_loader() : SpellScriptLoader("spell_sha_purification_shock_reset") { }

    AuraScript* GetAuraScript() const override
    {
        return new spell_sha_purification_shock_reset_AuraScript();
    }
};

void AddSC_sha_purification()
{
    new spell_sha_purification_shock_reset_loader();
}
