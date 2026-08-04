-- Alonecraft 4.61 -- Molten Fury is a Molten Core spender, not a passive rider.
--
--   As shipped in woa_2026_08_02_03.sql the Shadow Bolt DoT fired on every
--   cast, unconditionally, for as long as the talent was learned.  That made it
--   a flat damage rider rather than part of the Molten Core rotation.
--
--   spell_warl_molten_core::HandleMoltenFury now requires an active Molten Core
--   buff and spends one of its charges, so Shadow Bolt joins Incinerate and
--   Soul Fire as a consumer of the proc.  The other two are consumed by the
--   engine, because the buff's ADD_PCT_MODIFIER effects carry a SpellClassMask
--   that matches them; Shadow Bolt is deliberately not in that mask (it would
--   also collect the damage bonus), so the charge is dropped in C++.
--
--   Text follows the behaviour, in all three places that describe it:
--     * the talent itself (47245-47247), spellbook and talent pane
--     * the buff's description (47383 / 71162 / 71165), spellbook
--     * the buff's tooltip, which is what the buff bar renders
--
--   The talent rows are single-column edits, so UPDATE -- a full-row re-INSERT
--   would revert everything woa_2026_08_02_03.sql and later set.

-- ============================================================
-- The talent (47245 / 47246 / 47247)
-- ============================================================
UPDATE `alonecraft_spell_dbc` SET
    `SpellDescription0` = 'Increases the duration of your Immolate by 3 sec, your Corruption has a 4% chance to empower your next Incinerate or Soul Fire, and your Shadow Bolt consumes a charge of Molten Core to afflict the target with Molten Fury, dealing 3% of the damage done over 15 sec.'
WHERE `ID` = 47245;

UPDATE `alonecraft_spell_dbc` SET
    `SpellDescription0` = 'Increases the duration of your Immolate by 6 sec, your Corruption has a 8% chance to empower your next Incinerate or Soul Fire, and your Shadow Bolt consumes a charge of Molten Core to afflict the target with Molten Fury, dealing 6% of the damage done over 15 sec.'
WHERE `ID` = 47246;

UPDATE `alonecraft_spell_dbc` SET
    `SpellDescription0` = 'Increases the duration of your Immolate by 9 sec, your Corruption has a 12% chance to empower your next Incinerate or Soul Fire, and your Shadow Bolt consumes a charge of Molten Core to afflict the target with Molten Fury, dealing 10% of the damage done over 15 sec.'
WHERE `ID` = 47247;

-- ============================================================
-- The buff (47383 / 71162 / 71165) -- description and buff-bar tooltip
-- ============================================================
UPDATE `alonecraft_spell_dbc` SET
    `SpellDescription0` = 'Increases the duration of your Immolate by $/1000;47245s2 sec, and you have a 4% chance to gain the Molten Core effect when your Corruption deals damage. The Molten Core effect empowers your next 3 Incinerate, Soul Fire or Shadow Bolt spells cast within $47383d.\n\nIncinerate - Increases damage done by $47383s1% and reduces cast time by $47383s3%.\n\nSoul Fire - Increases damage done by $47383s1% and increases critical strike chance by $47383s2%.\n\nShadow Bolt - Afflicts the target with Molten Fury, dealing 3% of the damage done over 15 sec.',
    `SpellToolTip0`     = 'Incinerate - Increases damage done by $47383s1% and reduces cast time by $47383s3%.\n\nSoul Fire - Increases damage done by $47383s1% and increases critical strike chance by $47383s2%.\n\nShadow Bolt - Afflicts the target with Molten Fury, dealing 3% of the damage done over 15 sec.'
WHERE `ID` = 47383;

UPDATE `alonecraft_spell_dbc` SET
    `SpellDescription0` = 'Increases the duration of your Immolate by $/1000;47246s2 sec, and you have a 8% chance to gain the Molten Core effect when your Corruption deals damage. The Molten Core effect empowers your next 3 Incinerate, Soul Fire or Shadow Bolt spells cast within $71162d.\n\nIncinerate - Increases damage done by $71162s1% and reduces cast time by $71162s3%.\n\nSoul Fire - Increases damage done by $71162s1% and increases critical strike chance by $71162s2%.\n\nShadow Bolt - Afflicts the target with Molten Fury, dealing 6% of the damage done over 15 sec.',
    `SpellToolTip0`     = 'Incinerate - Increases damage done by $71162s1% and reduces cast time by $71162s3%.\n\nSoul Fire - Increases damage done by $71162s1% and increases critical strike chance by $71162s2%.\n\nShadow Bolt - Afflicts the target with Molten Fury, dealing 6% of the damage done over 15 sec.'
WHERE `ID` = 71162;

UPDATE `alonecraft_spell_dbc` SET
    `SpellDescription0` = 'Increases the duration of your Immolate by $/1000;47247s2 sec, and you have a 12% chance to gain the Molten Core effect when your Corruption deals damage. The Molten Core effect empowers your next 3 Incinerate, Soul Fire or Shadow Bolt spells cast within $71165d.\n\nIncinerate - Increases damage done by $71165s1% and reduces cast time by $71165s3%.\n\nSoul Fire - Increases damage done by $71165s1% and increases critical strike chance by $71165s2%.\n\nShadow Bolt - Afflicts the target with Molten Fury, dealing 10% of the damage done over 15 sec.',
    `SpellToolTip0`     = 'Incinerate - Increases damage done by $71165s1% and reduces cast time by $71165s3%.\n\nSoul Fire - Increases damage done by $71165s1% and increases critical strike chance by $71165s2%.\n\nShadow Bolt - Afflicts the target with Molten Fury, dealing 10% of the damage done over 15 sec.'
WHERE `ID` = 71165;
