-- ============================================================
-- Tuning: Taste for Blood, second 50% cut -- 10/20/30% -> 5/10/15% of RAP
-- ============================================================
--
-- woa_2026_08_12_08.sql already halved this from 20/40/60% to 10/20/30% and it
-- is still roughly double where it should be.  Same cut again, same reason.
--
-- Why it keeps overshooting what the tooltip implies: the payout is per PET
-- SWING but the coefficient is the HUNTER's ranged attack power.  Throughput is
-- therefore set by pet attack speed and by how many swing-like events the pet
-- generates -- neither of which appears anywhere in the number the player
-- reads.  A percentage that looks modest against one hit is large against a
-- 1.2s-swing pet plus its specials.  Any future retune of this talent should
-- move THIS number rather than adding a cap; the shape is fine, the constant
-- was set against the wrong denominator.
--
-- $s1 renders BasePoints + 1, so 4/9/14 display as 5/10/15.  Single-column
-- change, so UPDATE rather than a 234-column re-INSERT -- the full rows live in
-- the Beast Mastery pass and must not be reverted to their generated state.

UPDATE `alonecraft_spell_dbc` SET `EffectBasePoints1` = 4  WHERE `ID` = 19549;
UPDATE `alonecraft_spell_dbc` SET `EffectBasePoints1` = 9  WHERE `ID` = 19550;
UPDATE `alonecraft_spell_dbc` SET `EffectBasePoints1` = 14 WHERE `ID` = 19551;
