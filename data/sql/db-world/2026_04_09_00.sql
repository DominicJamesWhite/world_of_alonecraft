-- Polarity Conduit NPCs for Thaddius encounter (solo scaling)
-- Uses the Obedience Crystal model (displayId 26620) as a visual marker.
-- Spawned dynamically by ScaledEncounters C++ — no creature table rows needed.

DELETE FROM `creature_template` WHERE `entry` IN (200008, 200009);
INSERT INTO `creature_template` (`entry`, `name`, `subname`, `minlevel`, `maxlevel`, `faction`, `npcflag`, `unit_flags`, `type`, `type_flags`, `HealthModifier`, `ManaModifier`, `ArmorModifier`, `DamageModifier`, `ExperienceModifier`) VALUES
(200008, 'Positive Conduit', '', 80, 80, 35, 0, 0x00000002, 10, 0x00000400, 50, 0, 1, 1, 0),
(200009, 'Negative Conduit', '', 80, 80, 35, 0, 0x00000002, 10, 0x00000400, 50, 0, 1, 1, 0);
-- unit_flags: NON_ATTACKABLE (0x02) — selectable but can't be attacked
-- type 10 = NOT_SPECIFIED, type_flags 0x400 = NO_DEATH_LOG
-- faction 35 = friendly to all
-- HealthModifier 50 = beefy enough to survive incidental AoE

DELETE FROM `creature_template_model` WHERE `CreatureID` IN (200008, 200009);
INSERT INTO `creature_template_model` (`CreatureID`, `Idx`, `CreatureDisplayID`, `DisplayScale`, `Probability`) VALUES
(200008, 0, 26620, 1, 1),
(200009, 0, 26620, 1, 1);
