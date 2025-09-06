/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license: https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE-AGPL3
 */

// From SC
void AddMyPlayerScripts();
void AddSC_spell_bloomstrike();
void AddSC_aura_proc_self_hot_only();
void AddSC_aura_proc_moonfire_only();
void AddSC_spell_swiftmend_trigger();
void AddSC_spell_tree_of_life_ap();
void AddSC_PushbackImmunity();
void AddSC_mod_lifebloom();
void AddSC_spell_natures_focus_wrath();
void AddSC_spell_no_swing_reset_arakkoa();
void AddSC_spell_mage_mage_armor_custom();
void AddSC_molten_armor_mechanic();
void AddSC_spell_mage_aegis_of_antonidas();
void AddSC_frost_ice_armor_enhanced();
void AddSC_arcane_fortitude_mana_shield();
void AddSC_magic_absorption();
void AddSC_magic_attunement();
void AddSC_arcane_stability_mechanic();
void AddSC_fire_leech_mechanic();
void AddSC_firebreak_mechanic();
void AddSC_molten_shields_fire_ward();
void AddSC_spark_of_alar();
void AddSC_spell_mage_ablative_armor();

// Add all
// cf. the naming convention https://github.com/azerothcore/azerothcore-wotlk/blob/master/doc/changelog/master.md#how-to-upgrade-4
// additionally replace all '-' in the module folder name with '_' here
void Addworld_of_alonecraftScripts()
{
    AddSC_spell_mage_ablative_armor();
    AddMyPlayerScripts();
    AddSC_spell_bloomstrike();
    AddSC_aura_proc_self_hot_only();
    AddSC_aura_proc_moonfire_only();
    AddSC_spell_swiftmend_trigger();
    AddSC_spell_tree_of_life_ap();
    AddSC_PushbackImmunity();
    AddSC_mod_lifebloom();
    AddSC_spell_natures_focus_wrath();
    AddSC_spell_no_swing_reset_arakkoa();
    AddSC_spell_mage_mage_armor_custom();
    AddSC_molten_armor_mechanic();
    AddSC_spell_mage_aegis_of_antonidas();
    AddSC_frost_ice_armor_enhanced();
    AddSC_arcane_fortitude_mana_shield();
    AddSC_magic_absorption();
    AddSC_magic_attunement();
    AddSC_arcane_stability_mechanic();
    AddSC_fire_leech_mechanic();
    AddSC_firebreak_mechanic();
    AddSC_molten_shields_fire_ward();
    AddSC_spark_of_alar();
}
