-- ============================================================
-- Warrior (Protection): retune Controlled Aggression to 12/25%
-- ============================================================
-- Shipped in woa_2026_08_09_43.sql at 50/100% of shield block value as attack
-- power, which was overtuned: block value is the tree's central number and
-- three other talents already multiply it (Toughness feeds it from Strength,
-- Shield Mastery adds a flat percentage, Barricade adds 20% more), so the
-- conversion compounds with all of them before it is applied.
--
--   Rank 1: 50% -> 12%
--   Rank 2: 100% -> 25%
--
-- EffectBasePoints is written one under the intended number: CalcValue is
-- BasePoints + max(1, DieSides) and DieSides is 1 here, so 11/24 renders and
-- applies as 12/25.  The script (spell_warr_controlled_aggression in
-- WarriorProtConversions.cpp) reads this straight off the aura effect amount,
-- and the description is '$s1%', so both the tooltip and the effect follow the
-- number automatically -- nothing else has to change.
--
-- Single-column change to an existing custom spell, so this is an UPDATE, not
-- a 234-column DELETE + INSERT: a full re-INSERT would silently revert every
-- other column of these rows.
-- ============================================================

UPDATE `alonecraft_spell_dbc` SET `EffectBasePoints1` = 11 WHERE `ID` = 12301;
UPDATE `alonecraft_spell_dbc` SET `EffectBasePoints1` = 24 WHERE `ID` = 12818;
