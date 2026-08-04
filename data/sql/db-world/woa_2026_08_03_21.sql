-- Alonecraft 4.61 -- Nathrezim Foresight buff icon.
--
-- Match the buff to the ability that grants it: Sacrifice the Weak (47220)
-- uses SpellIconID 3171, while Nathrezim Foresight was still on 2307, the
-- generic icon it inherited when it was created.
--
-- ActiveIconID is 0 on this spell, so the buff bar reads SpellIconID. If an
-- ActiveIconID is ever set here it takes precedence and this change would
-- appear to do nothing.
--
-- Nathrezim Pact (200403) is left on 2307 deliberately -- it is the hidden
-- SPELL_ATTR0_DO_NOT_DISPLAY driver aura and never renders an icon.

UPDATE `alonecraft_spell_dbc`
SET `SpellIconID` = 3171
WHERE `ID` = 200507;
