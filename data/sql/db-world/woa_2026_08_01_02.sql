-- ===========================================================================
-- Warlock: swap the tree positions of Emberstorm and Molten Skin
-- ===========================================================================
--
-- TODO.md: "SWAP Emberstorm and Molten Skin positions"
--
-- Both talents live in the Destruction tree (TabID 301):
--   Emberstorm   (966,  5 ranks) Tier 5 Col 2  ->  Tier 1 Col 1
--   Molten Skin  (1887, 3 ranks) Tier 1 Col 1  ->  Tier 5 Col 2
--
-- Molten Skin is separately redesigned into "Infernal Bargain", an active
-- shard dump -- so it belongs deep in the tree, while Emberstorm (flat fire
-- damage / cast time) is a natural tier-1 filler.  The swap is a clean 1-for-1
-- exchange: each slot is vacated by exactly one member of the pair, so no
-- third talent is displaced.
--
-- Prerequisite arrows: verified safe.  All 895 Talent.dbc records were
-- checked and NO talent lists 966 or 1887 in PrereqTalent_1/2/3.  The
-- Destruction prereq graph (1817<-985, 981<-967, 968<-961, 1889<-1678,
-- 1888<-968) is untouched by this change.
--
-- All other columns are carried over verbatim from Talent.dbc; every one of
-- them is 0 for both talents.
--
-- NOTE: the client requires Talent.dbc records sorted by
-- (TabID, TierID, ColumnIndex) -- incorrect ordering blanks the entire talent
-- tree.  build_dbc.py re-sorts automatically, so these INSERTs are
-- deliberately NOT hand-ordered.
-- ===========================================================================

-- Emberstorm: Tier 5 Col 2 -> Tier 1 Col 1
DELETE FROM `talent_dbc` WHERE `ID` = 966;
INSERT INTO `talent_dbc` (`ID`, `TabID`, `TierID`, `ColumnIndex`, `SpellRank_1`, `SpellRank_2`, `SpellRank_3`, `SpellRank_4`, `SpellRank_5`, `SpellRank_6`, `SpellRank_7`, `SpellRank_8`, `SpellRank_9`, `PrereqTalent_1`, `PrereqTalent_2`, `PrereqTalent_3`, `PrereqRank_1`, `PrereqRank_2`, `PrereqRank_3`, `Flags`, `RequiredSpellID`, `CategoryMask_1`, `CategoryMask_2`) VALUES
(966, 301, 1, 1, 17954, 17955, 17956, 17957, 17958, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

-- Molten Skin (redesigned as Infernal Bargain): Tier 1 Col 1 -> Tier 5 Col 2
-- Flags is also raised from 0 to 1 (addToSpellBook): Infernal Bargain is
-- redesigned from a passive into an ACTIVE ability, so learning the talent
-- must put it in the spellbook.
DELETE FROM `talent_dbc` WHERE `ID` = 1887;
INSERT INTO `talent_dbc` (`ID`, `TabID`, `TierID`, `ColumnIndex`, `SpellRank_1`, `SpellRank_2`, `SpellRank_3`, `SpellRank_4`, `SpellRank_5`, `SpellRank_6`, `SpellRank_7`, `SpellRank_8`, `SpellRank_9`, `PrereqTalent_1`, `PrereqTalent_2`, `PrereqTalent_3`, `PrereqRank_1`, `PrereqRank_2`, `PrereqRank_3`, `Flags`, `RequiredSpellID`, `CategoryMask_1`, `CategoryMask_2`) VALUES
(1887, 301, 5, 2, 63349, 63350, 63351, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0);
