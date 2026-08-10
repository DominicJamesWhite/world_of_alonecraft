-- ============================================================
-- Warrior (Arms): parry-engine retune
-- ============================================================
-- The three talents shipped on 2026-08-07 all key off parry, so each one makes
-- the other two better: Two-Handed Weapon Specialization buys parry with crit,
-- Small Victories converts parries into Victory Rush uptime, and Riposte
-- converts them into uncapped AoE weapon damage.  Multiplied together the tree
-- outperformed everything else on offer, so all three coefficients come down.
--
-- Nothing structural changes -- no new effects, no new scripts, no talent
-- moves.  Every edit here is a single column on a row an earlier file already
-- inserted, so these are UPDATEs.  A full 234-column re-INSERT would silently
-- revert whatever else those files set.
--
-- Spells:
--   200600 / 200601 / 200602  = Riposte counterattacks, 40/70/100% -> 20/40/60%
--   12300 / 12959 / 12960     = Riposte talent (description only)
--   12163 / 12711 / 12712     = Two-Handed Weapon Specialization, 33/66/100%
--                               of crit as parry -> 20/40/60%
--   16462 - 16466             = Small Victories, 20-100% -> 10-50%
-- ============================================================

-- ============================================================
-- Riposte: 20 / 40 / 60% weapon damage
-- ============================================================
-- EffectDieSides1 is 1 on these rows, so CalcValue = EffectBasePoints1 + 1.
-- 19 / 39 / 59 therefore reads 20 / 40 / 60 in both places that matter: the
-- ApplyPct in SPELL_EFFECT_WEAPON_PERCENT_DAMAGE (SpellEffects.cpp:3611) and
-- the $s1 in the counterattack tooltip.  No text edit needed here.

UPDATE `alonecraft_spell_dbc` SET `EffectBasePoints1` = 19 WHERE `ID` = 200600;
UPDATE `alonecraft_spell_dbc` SET `EffectBasePoints1` = 39 WHERE `ID` = 200601;
UPDATE `alonecraft_spell_dbc` SET `EffectBasePoints1` = 59 WHERE `ID` = 200602;

-- ============================================================
-- Riposte: 3 second internal cooldown
-- ============================================================
-- Cooldown is the "cannot occur more than once every N sec" clause, enforced by
-- Aura::IsProcOnCooldown (SpellAuras.cpp:2083-2101).  It shipped at 2000; the
-- damage cut alone was not enough while the cadence stayed this fast.
--
-- Full DELETE + INSERT rather than an UPDATE: this is a spell_proc row, not a
-- DBC override, and the file that created it uses the same pair.  Restating all
-- 16 columns keeps the two files diffable against each other.

DELETE FROM `spell_proc` WHERE `SpellId` = -12300;
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(-12300, 0, 0, 0, 0, 0, 0, 0, 2, 32, 0, 0, 0, 0, 3000, 0);

-- The talent description names the cadence in prose -- there is no tooltip
-- variable for a spell_proc cooldown -- so it has to be edited alongside.
-- REPLACE rather than a literal string so the edit stays idempotent and does
-- not depend on the rest of the sentence.

UPDATE `alonecraft_spell_dbc`
SET `SpellDescription0` = REPLACE(`SpellDescription0`, 'once every 2 sec', 'once every 3 sec')
WHERE `ID` IN (12300, 12959, 12960);

-- ============================================================
-- Two-Handed Weapon Specialization: 20 / 40 / 60% of crit as parry
-- ============================================================
-- EffectDieSides2 is 1, so CalcValue = EffectBasePoints2 + 1.  The script does
-- not read base points directly -- spell_warr_2h_spec_parry::CalculateAmount
-- (WarriorParryConversions.cpp:74-101) receives the already-calculated amount
-- and multiplies crit by it -- so 19 / 39 / 59 gives 20 / 40 / 60 in the script
-- and in the $s2 of the talent description at the same time.
--
-- Effect 1, the 2/4/6% two-handed damage bonus set by woa_2026_08_08_02.sql, is
-- deliberately untouched.  Only the parry conversion was the problem.

UPDATE `alonecraft_spell_dbc` SET `EffectBasePoints2` = 19 WHERE `ID` = 12163;
UPDATE `alonecraft_spell_dbc` SET `EffectBasePoints2` = 39 WHERE `ID` = 12711;
UPDATE `alonecraft_spell_dbc` SET `EffectBasePoints2` = 59 WHERE `ID` = 12712;

-- ============================================================
-- Small Victories: 10 / 20 / 30 / 40 / 50% chance on parry
-- ============================================================
-- The per-rank chance lives in ProcChance, not in the spell_proc row -- that
-- row's Chance is 0, meaning "inherit from the DBC".  ProcChance is also what
-- $h renders, so this one column moves the mechanic and the tooltip together
-- and no description edit is needed.
--
-- Effect 1, the flat parry bonus, is untouched.

UPDATE `alonecraft_spell_dbc` SET `ProcChance` = 10 WHERE `ID` = 16462;
UPDATE `alonecraft_spell_dbc` SET `ProcChance` = 20 WHERE `ID` = 16463;
UPDATE `alonecraft_spell_dbc` SET `ProcChance` = 30 WHERE `ID` = 16464;
UPDATE `alonecraft_spell_dbc` SET `ProcChance` = 40 WHERE `ID` = 16465;
UPDATE `alonecraft_spell_dbc` SET `ProcChance` = 50 WHERE `ID` = 16466;
