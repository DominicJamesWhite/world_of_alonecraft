-- Alonecraft 4.61 -- Molten Core buff: the Shadow Bolt line on the buff bar.
--
--   woa_2026_08_04_12.sql added the Molten Fury line to SpellDescription0 of
--   47383 / 71162 / 71165 and deliberately left SpellToolTip0 alone, on the
--   grounds that the tooltip describes the Incinerate/Soul Fire empowerment
--   and that part did not change.
--
--   Wrong call: SpellToolTip is exactly the string the buff bar renders, so
--   hovering Molten Core on the warlock still showed the pre-redesign text.
--   The description is the spellbook/talent-pane string and is not what the
--   player was looking at.
--
--   Added as a third "Shadow Bolt - ..." line, matching the shape of the two
--   lines already there.  Note it is not strictly conditional on the buff:
--   Shadow Bolt applies Molten Fury whenever the talent is learned, buff up or
--   not.  It reads here as a summary of what the talent does to each spell,
--   which is also how the description is worded.
--
--   3/6/10% is literal, not $s.  The DoT amount lives on the talent's Effect2
--   (47245-47247), not on these buffs, and the client would resolve $s against
--   the buff's own base points.

UPDATE `alonecraft_spell_dbc` SET
    `SpellToolTip0` = 'Incinerate - Increases damage done by $47383s1% and reduces cast time by $47383s3%.\n\nSoul Fire - Increases damage done by $47383s1% and increases critical strike chance by $47383s2%.\n\nShadow Bolt - Afflicts the target with Molten Fury, dealing 3% of the damage done over 15 sec.'
WHERE `ID` = 47383;

UPDATE `alonecraft_spell_dbc` SET
    `SpellToolTip0` = 'Incinerate - Increases damage done by $71162s1% and reduces cast time by $71162s3%.\n\nSoul Fire - Increases damage done by $71162s1% and increases critical strike chance by $71162s2%.\n\nShadow Bolt - Afflicts the target with Molten Fury, dealing 6% of the damage done over 15 sec.'
WHERE `ID` = 71162;

UPDATE `alonecraft_spell_dbc` SET
    `SpellToolTip0` = 'Incinerate - Increases damage done by $71165s1% and reduces cast time by $71165s3%.\n\nSoul Fire - Increases damage done by $71165s1% and increases critical strike chance by $71165s2%.\n\nShadow Bolt - Afflicts the target with Molten Fury, dealing 10% of the damage done over 15 sec.'
WHERE `ID` = 71165;
