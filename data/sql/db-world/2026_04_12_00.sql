-- Register Yogg-Saron illusion-teleport SpellScript on all three portal spells
-- (Chamber/Icecrown/Stormwind).  Despawns outside tentacles when the player
-- enters an illusion at low group sizes.
DELETE FROM `spell_script_names` WHERE `ScriptName` = 'spell_scaled_yogg_teleport_to_illusion';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(63997, 'spell_scaled_yogg_teleport_to_illusion'),
(63998, 'spell_scaled_yogg_teleport_to_illusion'),
(63989, 'spell_scaled_yogg_teleport_to_illusion');
