-- Gift of Prophecy (talent 3000, Paladin Holy tab 382, tier 6 col 3):
-- rank 3 pointed at the rank 2 spell.
--
-- The base Talent.dbc row reads 200056 / 200058 / 200058, so the talent
-- exported and rendered as a 3-rank talent whose last two ranks were the same
-- spell -- identical tooltip, and the rank 3 proc rate unreachable.
--
-- Spell 200059 ("Gift of Prophecy Rank 3") exists in Spell.dbc with its own
-- description and its own spell_proc row at 6 PPM (rank 1 = 2, rank 2 = 4),
-- so the intent was three distinct ranks and the third slot simply got the
-- wrong id.
--
-- talent_dbc overrides replace the whole record (build_dbc.py rebuilds each
-- row from TALENT_COLUMNS), so every column of the base row is restated here.
-- All zeros beyond SpellRank_3: no prereqs, no flags, no required spell.

DELETE FROM `talent_dbc` WHERE `ID` = 3000;
INSERT INTO `talent_dbc` (
    `ID`, `TabID`, `TierID`, `ColumnIndex`,
    `SpellRank_1`, `SpellRank_2`, `SpellRank_3`, `SpellRank_4`, `SpellRank_5`,
    `SpellRank_6`, `SpellRank_7`, `SpellRank_8`, `SpellRank_9`,
    `PrereqTalent_1`, `PrereqTalent_2`, `PrereqTalent_3`,
    `PrereqRank_1`, `PrereqRank_2`, `PrereqRank_3`,
    `Flags`, `RequiredSpellID`, `CategoryMask_1`, `CategoryMask_2`
) VALUES (
    3000, 382, 6, 3,
    200056, 200058, 200059, 0, 0,
    0, 0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0, 0
);
