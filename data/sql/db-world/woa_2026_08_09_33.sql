-- ============================================================
-- Barricade: put it back on the global cooldown
-- ============================================================
-- The off-GCD choice was made when Barricade was a toggle: flipping a stance
-- on and off should not cost a Shield Slam each way.  Now that it is a rage
-- spender on a 10 second cooldown, pressing it IS a rotational decision, and a
-- spender that costs nothing to weave in has no opportunity cost at all.
--
-- 133 / 1500 is the standard warrior ability GCD, matching Shield Slam (23922)
-- and Revenge (6572).
--
-- Correction to the header of woa_2026_08_09_31.sql, which justified the
-- off-GCD setting by claiming "Shield Block itself is on the GCD".  It is not:
-- 2565 and Shield Wall 871 both ship with StartRecoveryCategory 0 and
-- StartRecoveryTime 0 in 3.3.5a.  The donor was already off-GCD and the
-- explicit zeroes in that file were a no-op.  The change here is a deliberate
-- design decision, not a correction of a donor leftover.
--
-- Only the ability needs this.  200652 and 200653 are triggered casts and
-- never touch the global cooldown.
--
-- UPDATE rather than a re-INSERT: a full 234-column INSERT would restate every
-- other column as whatever the generating tool believed at the time.
-- ============================================================

UPDATE `alonecraft_spell_dbc`
SET `StartRecoveryCategory` = 133,
    `StartRecoveryTime` = 1500
WHERE `ID` = 200651;
