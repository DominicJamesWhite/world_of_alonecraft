-- ============================================================
-- Harvest of Souls: talent_dbc, spell_bonus_data, spell_script_names
-- ============================================================

-- Talent 2039 (On a Pale Horse -> Harvest of Souls)
-- Point at custom learner spells 200127/200128 instead of vanilla 49146/51267
DELETE FROM `talent_dbc` WHERE `ID` = 2039;
INSERT INTO `talent_dbc` (`ID`, `TabID`, `TierID`, `ColumnIndex`, `SpellRank_1`, `SpellRank_2`, `SpellRank_3`, `SpellRank_4`, `SpellRank_5`, `SpellRank_6`, `SpellRank_7`, `SpellRank_8`, `SpellRank_9`, `PrereqTalent_1`, `PrereqTalent_2`, `PrereqTalent_3`, `PrereqRank_1`, `PrereqRank_2`, `PrereqRank_3`, `Flags`, `RequiredSpellID`, `CategoryMask_1`, `CategoryMask_2`) VALUES
(2039, 400, 3, 1, 200127, 200128, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

-- AP scaling for Harvest of Souls drain (matches Blood Plague 55078: ap_dot_bonus=0.06325)
DELETE FROM `spell_bonus_data` WHERE `entry` = 200131;
INSERT INTO `spell_bonus_data` (`entry`, `direct_bonus`, `dot_bonus`, `ap_bonus`, `ap_dot_bonus`, `comments`) VALUES
(200131, 0, 0, 0, 0.06325, 'Alonecraft - Harvest of Souls drain (matches Blood Plague AP scaling)');

-- Register SpellScript on all Death Strike ranks
DELETE FROM `spell_script_names` WHERE `ScriptName` = 'spell_dk_harvest_of_souls';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(-49998, 'spell_dk_harvest_of_souls');
