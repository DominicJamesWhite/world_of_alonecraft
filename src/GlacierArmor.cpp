#include "ScriptMgr.h"
#include "Player.h"
#include "SpellScript.h"
#include "SpellScriptLoader.h"
#include "Log.h"

// Glacier Armor: If player casts Ice Block (45438) while aura 11189 or 28332 is active,
// cast Healing Floes (200039) on the player.
// Registered on Ice Block (45438) via spell_script_names.

enum GlacierArmorSpells
{
    GLACIER_ARMOR_R1 = 11189,
    GLACIER_ARMOR_R2 = 28332,
    HEALING_FLOES    = 200039,
};

class spell_glacier_armor_ice_block : public SpellScript
{
    PrepareSpellScript(spell_glacier_armor_ice_block);

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ HEALING_FLOES });
    }

    void HandleAfterCast()
    {
        Unit* caster = GetCaster();
        if (!caster)
        {
            LOG_DEBUG("scripts", "GlacierArmor::HandleAfterCast - no caster");
            return;
        }

        Player* player = caster->ToPlayer();
        if (!player)
            return;

        if (!(player->HasAura(GLACIER_ARMOR_R1) || player->HasAura(GLACIER_ARMOR_R2)))
        {
            return;
        }

        player->CastSpell(player, HEALING_FLOES, true);
    }

    void Register() override
    {
        AfterCast += SpellCastFn(spell_glacier_armor_ice_block::HandleAfterCast);
    }
};

class spell_glacier_armor_ice_block_loader : public SpellScriptLoader
{
public:
    spell_glacier_armor_ice_block_loader() : SpellScriptLoader("spell_glacier_armor_ice_block") { }

    SpellScript* GetSpellScript() const override
    {
        return new spell_glacier_armor_ice_block();
    }
};

void AddSC_glacier_armor()
{
    new spell_glacier_armor_ice_block_loader();
}
