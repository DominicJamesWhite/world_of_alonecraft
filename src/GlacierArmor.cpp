#include "ScriptMgr.h"
#include "Player.h"
#include "Spell.h"
#include "SpellScriptLoader.h"
#include "Log.h"

// Glacier Armor: If player casts Ice Block (45438) while aura 11189 or 28332 is active,
// cast Healing Floes (200039) on the player.
class spell_glacier_armor_player_handler : public PlayerScript
{
public:
    spell_glacier_armor_player_handler() : PlayerScript("spell_glacier_armor_player_handler") { }

    void OnPlayerSpellCast(Player* player, Spell* spell, bool /*skipCheck*/) override
    {
        if (!player || !spell)
            return;

        SpellInfo const* spellInfo = spell->GetSpellInfo();
        if (!spellInfo)
            return;

        // Ice Block cast (spell id 45438)
        if (spellInfo->Id != 45438)
            return;

        // Glacier Armor ranks (11189, 28332)
        if (!(player->HasAura(11189) || player->HasAura(28332)))
            return;

        // Cast Healing Floes (200039) on the player
        player->CastSpell(player, 200039, true);
    }
};

// Registration helper
void AddSC_glacier_armor()
{
    new spell_glacier_armor_player_handler();
}
