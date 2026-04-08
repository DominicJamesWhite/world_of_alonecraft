/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license: https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE-AGPL3
 */

#include "ScriptMgr.h"
#include "Player.h"
#include "Config.h"
#include "Chat.h"

enum MyPlayerAcoreString
{
    HELLO_WORLD = 35410
};

// Class potion spell IDs, indexed by class ID (1-11)
static const uint32 ClassPotionSpells[] =
{
    0,      // 0:  unused
    200200, // 1:  Warrior  - Warrior's Resolve
    200201, // 2:  Paladin  - Light's Restoration
    200202, // 3:  Hunter   - Survivalist's Brew
    200203, // 4:  Rogue    - Shadow Draught
    200204, // 5:  Priest   - Sacred Renewal
    200205, // 6:  DK       - Blood of the Fallen
    200206, // 7:  Shaman   - Elemental Surge
    200207, // 8:  Mage     - Arcane Infusion
    200208, // 9:  Warlock  - Fel Restoration
    0,      // 10: unused
    200306, // 11: Druid    - Nature's Renewal
};

// Add player scripts
class MyPlayer : public PlayerScript
{
public:
    MyPlayer() : PlayerScript("MyPlayer") { }

    void OnPlayerLogin(Player* player) override
    {
        if (sConfigMgr->GetOption<bool>("MyModule.Enable", false))
            ChatHandler(player->GetSession()).PSendSysMessage(HELLO_WORLD);

        // Teach class-specific Diablo-style potion spell
        uint8 playerClass = player->getClass();
        if (playerClass >= 1 && playerClass <= 11)
        {
            uint32 spellId = ClassPotionSpells[playerClass];
            if (spellId && !player->HasSpell(spellId))
                player->learnSpell(spellId);
        }
    }
};

// Add all scripts in one
void AddMyPlayerScripts()
{
    new MyPlayer();
}
