-- Register spell_killing_word_shadow_word_death for Shadow Word: Death (all ranks)
-- Note: 32379 is Rank 1 of the player-cast spell. 32409 is the self-damage backlash effect.
DELETE FROM `spell_script_names` WHERE `ScriptName` = 'spell_killing_word_shadow_word_death';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(-32379, 'spell_killing_word_shadow_word_death');
