-- ============================================================
-- Warrior (Protection): halve three stat-conversion talents
-- ============================================================
-- The three "one stat pays out as another" talents from 4.67/4.68 all shipped
-- at a 1:1 max-rank conversion, which stacked into far more than the sum of its
-- parts: every one of them reads a character-sheet field that the rest of the
-- tree already inflates.  Each is halved, ranks kept linear.
--
--   One-Handed Weapon Specialization  10/20/30/40/50%  ->  5/10/15/20/25%
--     of dodge + parry as critical strike chance (effect 2; the 2/4/6/8/10%
--     one-handed damage on effect 1 is unchanged).
--   Improved Disciplines              50/100%          ->  25/50%
--     of defense rating as melee haste rating, still doubled under Shield
--     Block or Shield Wall.
--   Puncture                          10/20/30%        ->  5/10/15%
--     of Stamina as Strength.
--
-- EffectBasePoints is written one under the intended number: CalcValue is
-- BasePoints + max(1, DieSides) and DieSides is 1 on all of these.  Every
-- description is written with $sN, and each script takes the percentage
-- straight off the aura effect amount in DoEffectCalcAmount, so the tooltip and
-- the effect both follow these numbers with no other change.
--
-- Single-column changes to existing custom spells, so these are UPDATEs, not
-- 234-column DELETE + INSERTs, which would revert every other column.
-- ============================================================

-- One-Handed Weapon Specialization: crit share halved (effect 2)
UPDATE `alonecraft_spell_dbc` SET `EffectBasePoints2` = 4  WHERE `ID` = 16538;
UPDATE `alonecraft_spell_dbc` SET `EffectBasePoints2` = 9  WHERE `ID` = 16539;
UPDATE `alonecraft_spell_dbc` SET `EffectBasePoints2` = 14 WHERE `ID` = 16540;
UPDATE `alonecraft_spell_dbc` SET `EffectBasePoints2` = 19 WHERE `ID` = 16541;
UPDATE `alonecraft_spell_dbc` SET `EffectBasePoints2` = 24 WHERE `ID` = 16542;

-- Improved Disciplines: defense rating -> haste rating share halved
UPDATE `alonecraft_spell_dbc` SET `EffectBasePoints1` = 24 WHERE `ID` = 12312;
UPDATE `alonecraft_spell_dbc` SET `EffectBasePoints1` = 49 WHERE `ID` = 12803;

-- Puncture: Stamina -> Strength share halved
UPDATE `alonecraft_spell_dbc` SET `EffectBasePoints1` = 4  WHERE `ID` = 12308;
UPDATE `alonecraft_spell_dbc` SET `EffectBasePoints1` = 9  WHERE `ID` = 12810;
UPDATE `alonecraft_spell_dbc` SET `EffectBasePoints1` = 14 WHERE `ID` = 12811;
