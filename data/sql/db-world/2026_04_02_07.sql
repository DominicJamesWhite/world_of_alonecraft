-- Spiritsurge Earth Guardian: use custom scaled-down Greater Earth Elemental model
-- instead of the Earth Elemental Totem creature (15430)

-- Custom creature: Mini Greater Earth Elemental (cloned from 15352)
DELETE FROM `creature_template` WHERE `entry` = 200000;
INSERT INTO `creature_template` (`entry`, `name`, `subname`, `minlevel`, `maxlevel`, `exp`, `faction`, `npcflag`, `speed_walk`, `speed_run`, `detection_range`, `rank`, `dmgschool`, `DamageModifier`, `BaseAttackTime`, `RangeAttackTime`, `BaseVariance`, `RangeVariance`, `unit_class`, `unit_flags`, `unit_flags2`, `family`, `type`, `type_flags`, `MovementType`, `HoverHeight`, `HealthModifier`, `ManaModifier`, `ArmorModifier`, `ExperienceModifier`, `RacialLeader`, `movementId`, `RegenHealth`, `CreatureImmunitiesId`, `flags_extra`, `ScriptName`) VALUES
(200000, 'Earth Guardian', '', 66, 66, 1, 14, 0, 1.55556, 1.14286, 20, 0, 0, 1, 2000, 2000, 1, 1, 1, 0, 2048, 0, 4, 0, 0, 1, 0.8, 1, 1, 0, 0, 0, 1, -5, 0, 'npc_pet_shaman_earth_elemental');

DELETE FROM `creature_template_model` WHERE `CreatureID` = 200000;
INSERT INTO `creature_template_model` (`CreatureID`, `Idx`, `CreatureDisplayID`, `DisplayScale`, `Probability`, `VerifiedBuild`) VALUES
(200000, 0, 14511, 0.75, 1, 0);

-- Update summon spell: use custom creature + 8s duration (DurationIndex 21)
UPDATE `alonecraft_spell_dbc` SET `EffectMiscValue1` = 200000 WHERE `ID` = 200218;

-- Update talent and buff icons to 4110
UPDATE `alonecraft_spell_dbc` SET `SpellIconID` = 4110 WHERE `ID` IN (30872, 30873, 200217);
