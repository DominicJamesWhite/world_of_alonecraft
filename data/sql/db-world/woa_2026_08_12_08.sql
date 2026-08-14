-- ============================================================
-- Beast Mastery: Taste for Blood retune, 20/40/60% -> 10/20/30% of RAP
-- ============================================================
--
-- Measured at roughly twice the intended damage in play.  The talent scales off
-- the HUNTER's ranged attack power on every qualifying pet swing, so its output
-- rides pet attack speed: a cat at ~1.0s under Beast Cleave gets far more
-- procs per second than the 1s ICD suggests when read off the tooltip alone.
--
-- UPDATE rather than a full-row re-INSERT, per CLAUDE.md: a 234-column INSERT
-- silently restores every other column to whatever the generating tool thought
-- they were.  This is one column on three rows.
--
-- Ordering is safe.  The updater applies files by filename, and the original
-- full-row INSERT lives in woa_2026_08_11_09.sql, which sorts BEFORE this file
-- -- so this UPDATE lands on top of it rather than being overwritten by it.
--
-- Base points are 9/19/29, not 10/20/30.  spell_hun_taste_for_blood reads
-- aurEff->GetAmount(), which is CalcValue = BasePoints + max(1, DieSides), and
-- DieSides is 1 on all three ranks.  The same +1 is what makes $s1 render
-- 10/20/30 in the description, so the tooltip and the damage stay in step
-- automatically and the description text needs no edit.
--
-- Nothing else moves: the pet carrier (200745), its payload (200746), the proc
-- row and its 1s ICD are all unchanged.

UPDATE `alonecraft_spell_dbc` SET `EffectBasePoints1` = 9  WHERE `ID` = 19549;
UPDATE `alonecraft_spell_dbc` SET `EffectBasePoints1` = 19 WHERE `ID` = 19550;
UPDATE `alonecraft_spell_dbc` SET `EffectBasePoints1` = 29 WHERE `ID` = 19551;
