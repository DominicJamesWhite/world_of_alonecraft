/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license: https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE-AGPL3
 */

#include "Player.h"
#include "ScriptMgr.h"

// Define the spell ID for our passive pushback immunity aura
uint32 PUSHBACK_IMMUNITY_SPELL_ID = 200021;

class PushbackImmunityPlayerScript : public PlayerScript
{
public:
    PushbackImmunityPlayerScript() : PlayerScript("PushbackImmunityPlayerScript") { }

    void OnPlayerLogin(Player* player) override
    {
        // Apply the passive aura to the player if they don't already have it
        if (!player->HasAura(PUSHBACK_IMMUNITY_SPELL_ID))
        {
            player->AddAura(PUSHBACK_IMMUNITY_SPELL_ID, player);
        }
    }
};

void AddSC_PushbackImmunity()
{
    new PushbackImmunityPlayerScript();
}
