--
-- Simulator target dummies.
--
-- The offline combat simulator (worldserver --sim) needs a target whose armor,
-- level and health are known and fixed, so a DPS number reflects the spec under
-- test and nothing else.  Retail training dummies are level-appropriate rather
-- than boss-appropriate, and their stats are not ours to pin.
--
-- These entries are deliberately NOT given a `creature` spawn row: they are
-- summoned at runtime by the simulator and despawned with it, so nothing leaks
-- into the live world.
--
-- Entry band 2000100-2000199 is reserved for simulator fixtures.
--
-- unit_flags   0. It must NOT be 33554432 -- that is UNIT_FLAG_NOT_SELECTABLE
--              (0x02000000), copied from a vendor NPC template, and it makes the
--              dummy impossible to target, so every sim reports exactly 0 DPS.
-- flags_extra  66 = CIVILIAN (0x02, never aggroes) | NO_XP (0x40).
-- RegenHealth  0 -- a dummy that heals itself understates every DPS measurement.
-- DamageModifier 0.01. The dummy must AGGRO to be useful: a playerbot only runs
--              its rotation in BOT_STATE_COMBAT, and its target-selection values
--              read from units that are actually attacking it -- forcing combat
--              from outside is reasserted away by the AI each tick. So the dummy
--              fights, but hits for ~nothing, keeping the DPS measurement clean.
--
-- Armor is set by `ArmorModifier` against the level-83 base curve; the level-83
-- boss value (10643) is the standard raid reference point.
--

DELETE FROM `creature_template` WHERE `entry` BETWEEN 2000100 AND 2000199;
INSERT INTO `creature_template`
  (`entry`, `name`, `subname`, `minlevel`, `maxlevel`, `exp`, `faction`, `npcflag`,
   `speed_walk`, `speed_run`, `detection_range`, `rank`, `dmgschool`, `DamageModifier`,
   `BaseAttackTime`, `RangeAttackTime`, `BaseVariance`, `RangeVariance`, `unit_class`,
   `unit_flags`, `unit_flags2`, `family`, `type`, `type_flags`, `MovementType`,
   `HoverHeight`, `HealthModifier`, `ManaModifier`, `ArmorModifier`,
   `ExperienceModifier`, `RacialLeader`, `movementId`, `RegenHealth`,
   `CreatureImmunitiesId`, `flags_extra`, `ScriptName`)
VALUES
-- Boss-level stationary dummy: the standard single-target DPS reference.
(2000100, 'Simulator Dummy', 'Boss Level 83', 83, 83, 2, 14, 0,
 1, 1.14286, 20, 3, 0, 0.01,
 2000, 2000, 1, 1, 1,
 0, 2048, 0, 7, 0, 0,
 1, 1000, 1, 1,
 0, 0, 0, 0,
 0, 66, ''),
-- Level ladder dummies, for the sub-80 half of the game that 10x XP makes most
-- of the playtime.
(2000101, 'Simulator Dummy', 'Level 20', 22, 22, 0, 14, 0,
 1, 1.14286, 20, 3, 0, 0.01,
 2000, 2000, 1, 1, 1,
 0, 2048, 0, 7, 0, 0,
 1, 1000, 1, 1,
 0, 0, 0, 0,
 0, 66, ''),
(2000102, 'Simulator Dummy', 'Level 40', 42, 42, 0, 14, 0,
 1, 1.14286, 20, 3, 0, 0.01,
 2000, 2000, 1, 1, 1,
 0, 2048, 0, 7, 0, 0,
 1, 1000, 1, 1,
 0, 0, 0, 0,
 0, 66, ''),
(2000103, 'Simulator Dummy', 'Level 60', 62, 62, 0, 14, 0,
 1, 1.14286, 20, 3, 0, 0.01,
 2000, 2000, 1, 1, 1,
 0, 2048, 0, 7, 0, 0,
 1, 1000, 1, 1,
 0, 0, 0, 0,
 0, 66, ''),
(2000104, 'Simulator Dummy', 'Level 70', 73, 73, 0, 14, 0,
 1, 1.14286, 20, 3, 0, 0.01,
 2000, 2000, 1, 1, 1,
 0, 2048, 0, 7, 0, 0,
 1, 1000, 1, 1,
 0, 0, 0, 0,
 0, 66, '');

DELETE FROM `creature_template_model` WHERE `CreatureID` BETWEEN 2000100 AND 2000199;
INSERT INTO `creature_template_model`
  (`CreatureID`, `Idx`, `CreatureDisplayID`, `DisplayScale`, `Probability`, `VerifiedBuild`)
VALUES
(2000100, 0, 25207, 1, 1, 0),
(2000101, 0, 25207, 1, 1, 0),
(2000102, 0, 25207, 1, 1, 0),
(2000103, 0, 25207, 1, 1, 0),
(2000104, 0, 25207, 1, 1, 0);
