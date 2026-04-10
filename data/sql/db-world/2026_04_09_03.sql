-- Register Stone Grip suppression SpellScript on both 10-man and 25-man triggers
DELETE FROM `spell_script_names` WHERE `ScriptName` = 'spell_scaled_kologarn_stone_grip';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(62166, 'spell_scaled_kologarn_stone_grip'),
(63981, 'spell_scaled_kologarn_stone_grip');
