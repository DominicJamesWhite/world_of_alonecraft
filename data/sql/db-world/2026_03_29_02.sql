-- Register spell_mage_arcane_stability_proc for all Arcane Missiles ranks (1-13)
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
(25344, 'spell_mage_arcane_stability_proc'),
(25345, 'spell_mage_arcane_stability_proc'),
(42844, 'spell_mage_arcane_stability_proc'),
(42845, 'spell_mage_arcane_stability_proc'),
(42846, 'spell_mage_arcane_stability_proc');
