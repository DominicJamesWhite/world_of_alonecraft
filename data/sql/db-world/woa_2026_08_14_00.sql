--
-- Simulator sparring dummy: the solo-clear target.
--
-- sim.py has computed a solo-clear fraction since it was written (target died
-- AND actor lived, with margins), and it has always reported 0 -- because the
-- only target available was 2000100, which hits for 0.01x damage and holds 13.9
-- million health. Nothing can lose to it and nothing can kill it, so the metric
-- that decides solo viability had no fight to measure.
--
-- The alternative was to point the clear pass at a real Blizzard elite. That was
-- rejected: a retail creature's health and damage are incidental numbers we do
-- not own, several carry scripts or summon adds, and re-tuning the fork's
-- difficulty would mean picking a different creature and losing comparability
-- with every earlier run. This dummy's two numbers are dials instead.
--
-- Differences from 2000100, and only these:
--   DamageModifier  2.0 rather than 0.01. It has to be able to win.
--   HealthModifier  26 rather than 1000, so ~363k health against the level-83
--                   basehp2 of 13945. At the 4-8k DPS the current matrix
--                   reports that is a 45-90 second fight, which is the length
--                   the sustain pass already uses.
--   flags_extra     64 (NO_XP) rather than 66 (NO_XP | CIVILIAN). A civilian
--                   never aggroes, which is right for a punching bag and wrong
--                   for this.
--   ScriptName      sim_sparring_dummy rather than sim_target_dummy. That is
--                   also what tells SimRunner which of the two this is: the
--                   inert script is the thing that makes a target inert, so it
--                   is the honest discriminator, and the entry number is not.
--                   The AI is AggressorAI with EnterEvadeMode suppressed -- it
--                   has to fight back AND still never reset, because a solo
--                   caster breaks line of sight and distance constantly.
--   unit_flags2     0 rather than 2048 (UNIT_FLAG2_DISABLE_TURN). A creature
--                   that cannot turn cannot face a target, so it never swings.
--                   The first version of this dummy inherited the punching
--                   bag's 2048 and dealt EXACTLY zero damage over three
--                   75-second fights -- which reads as "too weak" and is
--                   actually "never attacked". Worth knowing the difference:
--                   damage_taken of 0.0 is a tuning result, damage_taken of
--                   exactly 0 is a broken flag.
--
-- BOTH NUMBERS ARE PROVISIONAL until the first matrix run calibrates them, and
-- they are deliberately the only thing to change when it does. The pass/fail
-- line wanted is roughly: a spec running its intended rotation in its intended
-- build clears, and the same spec on the stock build does not comfortably. If
-- every spec clears at 100% the dummy is too weak to separate anything; if none
-- does, it is measuring the dummy.
--
-- Changing either number invalidates comparison with earlier clear runs. Say so
-- in the commit, and re-baseline.
--

DELETE FROM `creature_template` WHERE `entry` = 2000110;
INSERT INTO `creature_template`
  (`entry`, `name`, `subname`, `minlevel`, `maxlevel`, `exp`, `faction`, `npcflag`,
   `speed_walk`, `speed_run`, `detection_range`, `rank`, `dmgschool`, `DamageModifier`,
   `BaseAttackTime`, `RangeAttackTime`, `BaseVariance`, `RangeVariance`, `unit_class`,
   `unit_flags`, `unit_flags2`, `family`, `type`, `type_flags`, `MovementType`,
   `HoverHeight`, `HealthModifier`, `ManaModifier`, `ArmorModifier`,
   `ExperienceModifier`, `RacialLeader`, `movementId`, `RegenHealth`,
   `CreatureImmunitiesId`, `flags_extra`, `ScriptName`)
VALUES
(2000110, 'Simulator Sparring Dummy', 'Boss Level 83', 83, 83, 2, 14, 0,
 1, 1.14286, 20, 3, 0, 2.0,
 2000, 2000, 1, 1, 1,
 0, 0, 0, 7, 0, 0,
 1, 26, 1, 1,
 0, 0, 0, 0,
 0, 64, 'sim_sparring_dummy');

-- Crowd-control immunity, and it is the difference between a solo-clear metric
-- and a kiting exhibition.
--
-- Measured without it: the frost mage took *literally zero damage* across three
-- 80-second fights. Frost Nova, back up, keep casting, repeat -- the dummy never
-- landed a single hit, so its clear pass measured nothing about survival at all
-- and no amount of raising DamageModifier would have changed that. A target that
-- never connects cannot be made more dangerous.
--
-- Every real solo target worth balancing against is a boss or an elite, and
-- those are CC-immune. A dummy that can be permanently rooted is measuring the
-- one situation the fork does not need numbers for.
--
-- The mask is written out rather than copied from an existing row, because what
-- is deliberately ABSENT matters as much as what is present:
--   in   CHARM DISORIENTED DISTRACT FEAR GRIP ROOT SLOW_ATTACK SLEEP SNARE STUN
--        FREEZE KNOCKOUT POLYMORPH BANISH SHACKLE TURN HORROR DAZE SAPPED
--   out  BLEED, INFECTED -- damage, and it has to be able to land
--   out  SHIELD, IMMUNE_SHIELD, INVULNERABILITY -- would make it unkillable
--   out  DISARM, SILENCE, INTERRUPT -- it has no weapon and casts nothing
--   out  TAUNTED -- harmless solo, and a tank pass will want it working
--
-- 0x49967DF6 = 1234599414.
--
-- ID 2000110 matches the creature entry. Positive ids in this table currently
-- top out at 1971; the negative band is generated from legacy
-- mechanic_immune_mask values and must not be reused.
DELETE FROM `creature_immunities` WHERE `ID` = 2000110;
INSERT INTO `creature_immunities`
  (`ID`, `SchoolMask`, `DispelTypeMask`, `MechanicsMask`, `Effects`, `Auras`,
   `ImmuneAoE`, `ImmuneChain`, `Comment`)
VALUES
(2000110, 0, 0, 1234599414, '', '', 0, 0,
 'Simulator sparring dummy: immune to crowd control, not to damage');

UPDATE `creature_template` SET `CreatureImmunitiesId` = 2000110 WHERE `entry` = 2000110;

-- A creature with no `creature_template_model` row cannot be summoned at all:
-- "has no model defined ... can't load", and the simulator reports only
-- "could not summon creature 2000110" with the fight silently never starting.
--
-- woa_2026_08_13_00.sql DELETEs the whole 2000100-2000199 band and re-inserts
-- only 2000100-2000104, and it sorts first, so this row has to be added here
-- rather than there. Same display id as the other dummies -- nothing looks at
-- it, but the server refuses to load without one.
DELETE FROM `creature_template_model` WHERE `CreatureID` = 2000110;
INSERT INTO `creature_template_model`
  (`CreatureID`, `Idx`, `CreatureDisplayID`, `DisplayScale`, `Probability`, `VerifiedBuild`)
VALUES
(2000110, 0, 25207, 1, 1, 0);
