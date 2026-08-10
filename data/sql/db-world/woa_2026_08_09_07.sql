-- ============================================================
-- Warrior (Protection): swap Improved Bloodrage and Toughness
-- ============================================================
-- TODO.md: "SWAP Improved Bloodrage (0, 0) and Toughness (3, 2) positions."
--
-- TODO.md writes talent positions as (column, tier).  Both talents live in the
-- Protection tree, TalentTab 163:
--
--   Toughness          (140, 5 ranks) Tier 2 Col 3  ->  Tier 0 Col 0
--   Improved Bloodrage (142, 2 ranks) Tier 0 Col 0  ->  Tier 2 Col 3
--
-- Why.  Improved Bloodrage is being redesigned into a Shield Slam damage
-- ramp (woa_2026_08_09_13.sql) -- an offensive payoff that only makes sense
-- once you have a rotation to spend it on, and one that wants to sit behind ten
-- points of investment.  Toughness is the opposite: flat armour, now also the
-- Strength-to-block-value scaler (woa_2026_08_09_12.sql), and it is what a
-- level-10 warrior who just picked the tree should be able to buy immediately.
-- Tier 0 is also where a 5-rank talent belongs, because tier 0 is the only
-- tier with nothing gating it -- 5 ranks there unlock the next tier on their
-- own.
--
-- Prerequisite arrows: verified safe.  All 895 Talent.dbc records were checked
-- and NO talent lists 140 or 142 in PrereqTalent_1/2/3 -- the only two arrows in
-- the whole Protection tree are 148 -> 152 and 1871 -> 1666, neither of which
-- touches this pair.  Nothing else has to move.
--
-- NOTE: the client requires Talent.dbc records sorted by
-- (TabID, TierID, ColumnIndex) -- incorrect ordering blanks the ENTIRE talent
-- tree for the class, not just the moved rows.  build_dbc.py re-sorts
-- automatically, so the two records below are written in ID order and no
-- attempt is made to order them by position.
--
-- talent_dbc overrides replace the whole record, so every column is restated.
-- Both keep Flags = 0: neither is an active ability, so neither goes in the
-- spellbook.
-- ============================================================

-- Toughness: Tier 2 Col 3 -> Tier 0 Col 0
DELETE FROM `talent_dbc` WHERE `ID` = 140;
INSERT INTO `talent_dbc` (`ID`, `TabID`, `TierID`, `ColumnIndex`, `SpellRank_1`, `SpellRank_2`, `SpellRank_3`, `SpellRank_4`, `SpellRank_5`, `SpellRank_6`, `SpellRank_7`, `SpellRank_8`, `SpellRank_9`, `PrereqTalent_1`, `PrereqTalent_2`, `PrereqTalent_3`, `PrereqRank_1`, `PrereqRank_2`, `PrereqRank_3`, `Flags`, `RequiredSpellID`, `CategoryMask_1`, `CategoryMask_2`) VALUES
(140, 163, 0, 0, 12299, 12761, 12762, 12763, 12764, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

-- Improved Bloodrage: Tier 0 Col 0 -> Tier 2 Col 3
DELETE FROM `talent_dbc` WHERE `ID` = 142;
INSERT INTO `talent_dbc` (`ID`, `TabID`, `TierID`, `ColumnIndex`, `SpellRank_1`, `SpellRank_2`, `SpellRank_3`, `SpellRank_4`, `SpellRank_5`, `SpellRank_6`, `SpellRank_7`, `SpellRank_8`, `SpellRank_9`, `PrereqTalent_1`, `PrereqTalent_2`, `PrereqTalent_3`, `PrereqRank_1`, `PrereqRank_2`, `PrereqRank_3`, `Flags`, `RequiredSpellID`, `CategoryMask_1`, `CategoryMask_2`) VALUES
(142, 163, 2, 3, 12301, 12818, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
