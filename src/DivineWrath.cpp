#include "ScriptMgr.h"
#include "Player.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "SpellScriptLoader.h"
#include "Unit.h"
#include "Log.h"

class divine_wrath_handler : public PlayerScript
{
public:
    divine_wrath_handler() : PlayerScript("divine_wrath_handler") { }

    void OnPlayerSpellCast(Player* player, Spell* spell, bool skipCheck) override
    {
        if (!player || !spell)
            return;

        // Check if the player has Divine Wrath aura (rank 1 or 2)
        if (!player->HasAura(200042) && !player->HasAura(200043))
            return;

        uint32 originalSpellId = spell->GetSpellInfo()->Id;
        uint32 replacementSpellId = 0;

        switch (originalSpellId)
        {
            case 2812:  // Holy Wrath Rank 1
                replacementSpellId = 200044;
                break;
            case 10318: // Holy Wrath Rank 2
                replacementSpellId = 200045;
                break;
            case 27139: // Holy Wrath Rank 3
                replacementSpellId = 200046;
                break;
            case 48816: // Holy Wrath Rank 4
                replacementSpellId = 200047;
                break;
            case 48817: // Holy Wrath Rank 5
                replacementSpellId = 200048;
                break;
            default:
                return; // Not a Holy Wrath spell, do nothing
        }

        if (replacementSpellId != 0)
        {
            // Cancel the original spell cast
            spell->cancel();

            // Cast the replacement spell
            player->CastSpell(player, replacementSpellId, false);
        }
    }
};

void AddSC_divine_wrath()
{
    new divine_wrath_handler();
}
