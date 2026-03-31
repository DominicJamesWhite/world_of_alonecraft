-- Register spell_mage_molten_shields_fire_ward for Fire Ward (all ranks via negative first-rank ID)
DELETE FROM `spell_script_names` WHERE `ScriptName` = 'spell_mage_molten_shields_fire_ward';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(-543, 'spell_mage_molten_shields_fire_ward');
