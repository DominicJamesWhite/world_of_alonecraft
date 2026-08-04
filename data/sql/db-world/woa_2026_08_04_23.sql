-- Alonecraft 4.61 -- Waylay: drop the chance clause from the talent text.
--
-- TODO.md now reads "Your Ambush and Backstab hits unbalance your target,
-- reducing their chance to hit you by 2/4% and increasing your chance to
-- critically strike against them by 2/4%" -- the proc is a flat 100%, so
-- spelling out a chance only invites the reader to look for the roll.
--
-- Only SpellDescription0 on the two talent ranks changes. The debuff rows
-- (200502/200503) already word it this way, and their SpellToolTip0 -- the
-- text the debuff bar actually renders -- is untouched.
--
-- The amounts stay as $200502sN / $200503sN references so they keep tracking
-- the debuff's base points rather than hardcoding 2 and 4.
--
-- Ordering note: UPDATEs only, no full-row re-INSERT, so this cannot clobber
-- the other columns woa_2026_08_03_03.sql set.

-- Waylay rank 1 (51692) -- triggers 200502
UPDATE `alonecraft_spell_dbc` SET
    `SpellDescription0` = 'Your Ambush and Backstab hits unbalance your target, reducing their chance to hit you by $200502s1% and increasing your chance to critically strike against them by $200502s2% for $200502d.'
WHERE `ID` = 51692;

-- Waylay rank 2 (51696) -- triggers 200503
UPDATE `alonecraft_spell_dbc` SET
    `SpellDescription0` = 'Your Ambush and Backstab hits unbalance your target, reducing their chance to hit you by $200503s1% and increasing your chance to critically strike against them by $200503s2% for $200503d.'
WHERE `ID` = 51696;
