-- ============================================================
-- Warrior (Protection): Improved Disciplines doubles under a block cooldown
-- ============================================================
-- Extends woa_2026_08_09_39.sql.  The conversion is unchanged at 50/100% of
-- defense rating; while Shield Block (2565) or Shield Wall (871) is active it
-- is doubled.
--
-- Why it pays out there.  The haste is meant to convert overcapped avoidance
-- into rage income, and the moment a warrior most needs the extra global is the
-- moment they have pressed a block cooldown -- which is also when the incoming
-- damage that funds Shield Specialization and Bloodrage is highest.  Doubling
-- inside those windows makes the talent a burst lever rather than a flat
-- passive.
--
-- Barricade (200652) is deliberately NOT one of the buffs, even though it is
-- Alonecraft's second Shield Block: it is 10 seconds up on a 10 second
-- cooldown, so counting it would make the doubling permanent and delete the
-- distinction this change exists to create.
--
-- Nothing changes in the DBC but the prose.  The per-rank percentage stays 50 /
-- 100 in base points, because that is what the script reads before doubling it
-- -- writing the doubled number here would double it twice.
--
-- The second sentence is plain prose rather than a variable on purpose: $s1
-- renders the value stored in base points, which is the UN-doubled number, so
-- "$s1%" in the doubled clause would print 50/100 and read as no change at all.
--
-- Single-column change to rows that already exist, so UPDATE rather than a
-- 234-column re-INSERT (CLAUDE.md).
--
-- Spells:
--   12312 / 12803 = Improved Disciplines ranks 1-2
--   2565 / 871    = Shield Block / Shield Wall (the doubling window)
-- ============================================================

UPDATE `alonecraft_spell_dbc`
   SET `SpellDescription0` = 'While in Defensive Stance, you gain melee haste rating equal to $s1% of your defense rating.  This is doubled while Shield Block or Shield Wall is active.'
 WHERE `ID` IN (12312, 12803);
