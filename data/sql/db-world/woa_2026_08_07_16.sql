-- ==========================================================================
-- The Quartermaster — mail sender for level-up gear shipments
-- ==========================================================================
--
-- This creature exists ONLY to be a mail sender, and deliberately has no spawn.
-- MAIL_CREATURE (message type 3) stores the entry in the mail row and the
-- client resolves the sender's name and portrait with CMSG_CREATURE_QUERY,
-- which ObjectMgr answers from `creature_template` directly.  Retail uses the
-- same arrangement for The Postmaster (34337) in Player::SendItemRetrievalMail.
--
-- Do not add a spawn.  If one is ever added for a screenshot, `faction` 35
-- (friendly to all), `unit_flags` 33554432 (UNIT_FLAG_IMMUNE_TO_NPC) and
-- `flags_extra` 2 (CREATURE_FLAG_EXTRA_CIVILIAN) keep it from being attackable.
--
-- `creature_template_model` is mandatory: without a CreatureDisplayID the mail
-- portrait renders as a blank frame.  27104 is The Postmaster's own model.
--
-- The item pool this NPC mails from is generated separately by
-- tools/gen_quartermaster_pool.py -- see the companion pool SQL file.

DELETE FROM `creature_template` WHERE `entry` = 200012;
INSERT INTO `creature_template` (`entry`, `name`, `subname`, `minlevel`, `maxlevel`, `exp`, `faction`, `npcflag`, `speed_walk`, `speed_run`, `detection_range`, `rank`, `dmgschool`, `DamageModifier`, `BaseAttackTime`, `RangeAttackTime`, `BaseVariance`, `RangeVariance`, `unit_class`, `unit_flags`, `unit_flags2`, `family`, `type`, `type_flags`, `MovementType`, `HoverHeight`, `HealthModifier`, `ManaModifier`, `ArmorModifier`, `ExperienceModifier`, `RacialLeader`, `movementId`, `RegenHealth`, `CreatureImmunitiesId`, `flags_extra`, `ScriptName`) VALUES
(200012, 'The Quartermaster', 'Requisitions', 80, 80, 2, 35, 0, 1, 1.14286, 20, 0, 0, 1, 2000, 2000, 1, 1, 1, 33554432, 2048, 0, 7, 0, 0, 1, 1, 1, 1, 1, 0, 0, 1, 0, 2, '');

DELETE FROM `creature_template_model` WHERE `CreatureID` = 200012;
INSERT INTO `creature_template_model` (`CreatureID`, `Idx`, `CreatureDisplayID`, `DisplayScale`, `Probability`, `VerifiedBuild`) VALUES
(200012, 0, 27104, 1, 1, 0);
