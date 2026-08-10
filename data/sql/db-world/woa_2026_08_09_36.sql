-- ============================================================
-- Barricade: clear two inherited Shield Block tooltips
-- ============================================================
-- Both 200651 and 200653 were still carrying Shield Block's SpellToolTip,
-- "Block chance and block value increased by $s1%.", straight from the clone.
-- 200653's description also still said "your block value" after the heal was
-- halved to 50% in woa_2026_08_09_35.sql.
--
-- Neither string is rendered today -- SpellToolTip is buff-bar text and
-- neither of these spells applies an aura, and 200653 is never in a spellbook
-- -- so this is tidiness rather than a visible bug.  It is worth doing anyway:
-- an inherited tooltip that happens to be unreachable is the same latent trap
-- that put "Holy spell damage reduced by $s1%" on Wailing Soul, and `$s1` here
-- would resolve against base points of 0 and print 1%.
--
-- The buff (200652) is the one spell of the three whose tooltip is actually
-- shown, and it was set correctly in _35.
-- ============================================================

UPDATE `alonecraft_spell_dbc`
SET `SpellToolTip0` = ''
WHERE `ID` = 200651;

UPDATE `alonecraft_spell_dbc`
SET `SpellDescription0` = 'Heals you for 50% of your block value.',
    `SpellToolTip0` = ''
WHERE `ID` = 200653;
