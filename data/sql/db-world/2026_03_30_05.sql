-- Register spell_dru_lifebloom for Lifebloom (all ranks via negative first-rank ID)
DELETE FROM `spell_script_names` WHERE `ScriptName` = 'spell_dru_lifebloom';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(-33763, 'spell_dru_lifebloom');
