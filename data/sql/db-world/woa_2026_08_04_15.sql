-- Alonecraft 4.61 -- Molten Core talent description, rewritten.
--
--   woa_2026_08_04_14.sql crammed all three effects into one sentence.  Restyled
--   to match the buff's own layout: a lead paragraph, then one line per
--   empowered spell.  Only SpellDescription0 of the three talent ranks changes;
--   the buff (47383 / 71162 / 71165) is left exactly as it is.
--
--   Values are variables wherever the number really is in the client's DBC:
--     * $/1000;4724Xs2   Immolate extension, from the talent's own Effect2
--     * $s3              Shadow Bolt share, from the talent's own Effect3
--                        (base points 2/5/9 -> 3/6/10, the +1 rule)
--     * $<buff>s1/s2/s3  Incinerate and Soul Fire numbers, read off the buff,
--                        exactly as the buff's own description reads them
--     * $<buff>d         how long the Molten Core effect lasts
--     * $200420d         how long Molten Fury ticks
--
--   The proc chance stays literal 4/8/12.  $h would resolve to the talent's
--   ProcChance, which woa_2026_08_02_03.sql raised to 100 so the Shadow Bolt
--   effect could fire on every cast -- the real roll lives in
--   spell_warl_molten_core::CheckOriginalProc.  This is the same reason the
--   buff descriptions were de-variabled in woa_2026_08_04_12.sql.
--
--   Note Molten Fury runs for 15 sec, not 10: DurationIndex 8, the same index
--   the Molten Core buff itself uses.  $200420d renders whichever it really is.

UPDATE `alonecraft_spell_dbc` SET
    `SpellDescription0` = 'Increases the duration of your Immolate by $/1000;47245s2 sec, and you have a 4% chance to gain the Molten Core effect when your Corruption deals damage. The Molten Core effect empowers your next 3 Incinerate, Shadow Bolt or Soul Fire spells cast within $47383d.\n\nIncinerate - Increases damage done by $47383s1% and reduces cast time by $47383s3%.\n\nShadow Bolt - $s3% of Shadow Bolt damage is done to your target over $200420d.\n\nSoul Fire - Increases damage done by $47383s1% and increases critical strike chance by $47383s2%.'
WHERE `ID` = 47245;

UPDATE `alonecraft_spell_dbc` SET
    `SpellDescription0` = 'Increases the duration of your Immolate by $/1000;47246s2 sec, and you have a 8% chance to gain the Molten Core effect when your Corruption deals damage. The Molten Core effect empowers your next 3 Incinerate, Shadow Bolt or Soul Fire spells cast within $71162d.\n\nIncinerate - Increases damage done by $71162s1% and reduces cast time by $71162s3%.\n\nShadow Bolt - $s3% of Shadow Bolt damage is done to your target over $200420d.\n\nSoul Fire - Increases damage done by $71162s1% and increases critical strike chance by $71162s2%.'
WHERE `ID` = 47246;

UPDATE `alonecraft_spell_dbc` SET
    `SpellDescription0` = 'Increases the duration of your Immolate by $/1000;47247s2 sec, and you have a 12% chance to gain the Molten Core effect when your Corruption deals damage. The Molten Core effect empowers your next 3 Incinerate, Shadow Bolt or Soul Fire spells cast within $71165d.\n\nIncinerate - Increases damage done by $71165s1% and reduces cast time by $71165s3%.\n\nShadow Bolt - $s3% of Shadow Bolt damage is done to your target over $200420d.\n\nSoul Fire - Increases damage done by $71165s1% and increases critical strike chance by $71165s2%.'
WHERE `ID` = 47247;
