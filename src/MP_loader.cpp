/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license: https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE-AGPL3
 */

// From SC
void AddMyPlayerScripts();
void AddSC_spell_bloomstrike();
void AddSC_aura_proc_self_hot_only();

// Add all
// cf. the naming convention https://github.com/azerothcore/azerothcore-wotlk/blob/master/doc/changelog/master.md#how-to-upgrade-4
// additionally replace all '-' in the module folder name with '_' here
void Addworld_of_alonecraftScripts()
{
    AddMyPlayerScripts();
    AddSC_spell_bloomstrike();
    AddSC_aura_proc_self_hot_only();
}
