-- Register spell_mage_arcane_stability_proc for Arcane Missiles ranks that exist in 3.3.5a
-- Note: 25344 and 25345 do not exist in the 3.3.5a Spell.dbc and are intentionally excluded
DELETE FROM `spell_script_names` WHERE `ScriptName` = 'spell_mage_arcane_stability_proc';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(5143,  'spell_mage_arcane_stability_proc'),
(5144,  'spell_mage_arcane_stability_proc'),
(5145,  'spell_mage_arcane_stability_proc'),
(8416,  'spell_mage_arcane_stability_proc'),
(8417,  'spell_mage_arcane_stability_proc'),
(10263, 'spell_mage_arcane_stability_proc'),
(10264, 'spell_mage_arcane_stability_proc'),
(10265, 'spell_mage_arcane_stability_proc'),
(42844, 'spell_mage_arcane_stability_proc'),
(42845, 'spell_mage_arcane_stability_proc'),
(42846, 'spell_mage_arcane_stability_proc');
