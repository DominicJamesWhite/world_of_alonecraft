-- Spiritsurge: spell_script_names registrations (replaces PlayerScript)

-- Earthliving heal procs -> apply Spiritsurge buff
DELETE FROM `spell_script_names` WHERE `ScriptName` = 'spell_sha_spiritsurge_proc';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
    (51945, 'spell_sha_spiritsurge_proc'),
    (51990, 'spell_sha_spiritsurge_proc'),
    (51997, 'spell_sha_spiritsurge_proc'),
    (51998, 'spell_sha_spiritsurge_proc'),
    (51999, 'spell_sha_spiritsurge_proc'),
    (52000, 'spell_sha_spiritsurge_proc');

-- Earth Shock (all ranks) -> summon earth elemental
DELETE FROM `spell_script_names` WHERE `ScriptName` = 'spell_sha_spiritsurge_earth';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
    (-8042, 'spell_sha_spiritsurge_earth');

-- Flame Shock (all ranks) -> spread to nearby enemies
DELETE FROM `spell_script_names` WHERE `ScriptName` = 'spell_sha_spiritsurge_flame';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
    (-8050, 'spell_sha_spiritsurge_flame');

-- Frost Shock (all ranks) -> apply root
DELETE FROM `spell_script_names` WHERE `ScriptName` = 'spell_sha_spiritsurge_frost';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
    (-8056, 'spell_sha_spiritsurge_frost');
