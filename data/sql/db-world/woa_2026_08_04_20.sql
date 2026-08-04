-- ===========================================================================
-- Warlock / Demonology: Demonic Aegis auras inherit the talent's icon
-- ===========================================================================
--
-- 200417 (the warlock's buff, woa_2026_08_02_02.sql) was built from scratch
-- rather than cloned with gen_sql.py dbc --base, so every unspecified column
-- came out 0 -- including SpellIconID, which the generator then wrote as 1.
-- Icon 1 is the client's placeholder, so the buff bar showed the default "?"
-- artwork instead of anything recognisable.
--
-- The talent ranks 30143 / 30144 / 30145 all use SpellIconID 89, so point the
-- auras at the same icon and the buff reads as the talent that granted it.
--
-- 200508 / 200509 / 200510 are included too.  woa_2026_08_04_04.sql cloned them
-- from 200417 via a temporary table to carry the crit avoidance onto the demon,
-- so they inherited the same placeholder icon and show it on the pet frame.
--
-- Single-column UPDATE, not DELETE + full-row INSERT: a re-INSERT of 200417
-- would revert woa_2026_08_03_18.sql, and one of 200508-200510 would revert the
-- per-rank amounts woa_2026_08_04_04.sql set on each clone.
--
-- ===========================================================================

UPDATE `alonecraft_spell_dbc` SET `SpellIconID` = 89
    WHERE `ID` IN (200417, 200508, 200509, 200510);
