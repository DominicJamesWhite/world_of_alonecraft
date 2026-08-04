-- Alonecraft 4.61 -- Bladework (14185): counter dodged and parried attacks.
--
-- Marked for Counterattack (200504) only fired when the marked enemy
-- actually landed a blow. A rogue who dodges or parries got nothing, which
-- is backwards -- those are exactly the openings a counterattack is for.
--
-- The gate was the spell_proc row's HitMask, left at 0. SpellMgr.cpp:931
-- substitutes a default when HitMask is unset, and for a DONE proc that
-- default is PROC_HIT_NORMAL | PROC_HIT_CRITICAL | PROC_HIT_ABSORB -- avoided
-- attacks are excluded. Spelling the mask out restores those three and adds
-- PROC_HIT_DODGE (0x10) and PROC_HIT_PARRY (0x20):
--
--   0x001 PROC_HIT_NORMAL
--   0x002 PROC_HIT_CRITICAL
--   0x010 PROC_HIT_DODGE      <- new
--   0x020 PROC_HIT_PARRY      <- new
--   0x400 PROC_HIT_ABSORB
--   ----- = 1075
--
-- Misses are deliberately left out: a miss is the attacker's failure, not
-- the rogue's defence, and including it would make the mark burn charges on
-- swings the rogue did nothing about.
--
-- Both proc paths already run on an avoided attack, so no C++ is needed:
-- melee swings proc from Unit::AttackerStateUpdate (Unit.cpp:2847) with the
-- outcome-derived hit mask regardless of damage, and dodged/parried melee
-- abilities take the miss branch at Spell.cpp:2897, which builds its
-- DamageInfo straight from the SpellMissInfo. Damage being 0 is not a
-- problem either -- the row's SpellTypeMask is 0, so the
-- PROC_SPELL_TYPE_DAMAGE filter never applies.

DELETE FROM `spell_proc` WHERE `SpellId` = 200504;
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(200504, 0, 0, 0, 0, 0, 20, 0, 2, 1075, 0, 0, 0, 100, 0, 5);

-- ============================================================
-- Tooltips. Single-column UPDATEs, never a full-row re-INSERT.
-- ============================================================

UPDATE `alonecraft_spell_dbc`
SET `SpellDescription0` = 'Mark all enemies within 10 yards for rapid counterattack.  Their next attacks on you, one per combo point spent, are immediately countered for $200505s1% weapon damage, or 180% if a dagger is equipped.  Attacks you dodge or parry are countered as well.  Lasts $200504d.'
WHERE `ID` = 14185;

UPDATE `alonecraft_spell_dbc`
SET `SpellDescription0` = 'Your next attacks on the marked rogue will be immediately countered, whether they land or are dodged or parried.'
WHERE `ID` = 200504;
