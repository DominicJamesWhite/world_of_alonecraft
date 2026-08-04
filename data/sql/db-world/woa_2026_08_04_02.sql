-- Alonecraft 4.61 -- Fel Synergy: flat 100% chance, both magnitudes rank.
--
-- woa_2026_08_02_03.sql rewrote the description to say "100% chance" on both
-- ranks, but never touched ProcChance -- rank 1 was still Blizzard's 50.  The
-- talent has no spell_proc row of its own (core ships -47230 with ProcFlags
-- and Chance both 0, and SpellMgr.cpp:2079-2084 backfills each from the rank's
-- own SpellInfo), so rank 1 really did heal the pet half as often as it
-- claimed.  Same mistake as Imperious Flames: the text was written off the top
-- rank.
--
-- Resolved by making the chance genuinely flat and moving the progression into
-- the two magnitudes instead:
--
--   Rank   ProcChance   pet heal (s1)      owner leech (s2)
--     1       100         7%  (bp 6)         20%  (bp 19)
--     2       100        15%  (bp 14)        40%  (bp 39)
--
-- Was: chance 50/100, pet heal 15/15 (flat), owner leech 20/40.  The leech is
-- unchanged; rank 1 trades a guaranteed proc for a smaller pet heal, and the
-- pet heal now ranks instead of sitting flat.
--
-- The description now uses $s1/$s2 rather than literal numbers, so it cannot
-- drift from the data again.  The chance is stated as plain text because it is
-- the same at both ranks and $h would be the only way to vary it.
--
-- No C++ change needed.  The pet-heal half is core's spell_warl_fel_synergy
-- reading EFFECT_0; the owner-leech half is spell_warl_demon_fel_synergy
-- (WarlockDemonPets.cpp:247) reading EFFECT_1.  Both take the amount off the
-- aura, so they follow the new base points.
--
-- Ordering note: single-column UPDATEs only, no full-row re-INSERT, so this
-- cannot clobber the other columns woa_2026_08_02_03.sql set.

UPDATE `alonecraft_spell_dbc` SET
    `ProcChance` = 100,
    `EffectBasePoints1` = 6,
    `EffectBasePoints2` = 19,
    `SpellDescription0` = 'You heal your pet for $s1% of the spell damage you deal, and $s2% of the damage dealt by your demon heals you.'
WHERE `ID` = 47230;

UPDATE `alonecraft_spell_dbc` SET
    `ProcChance` = 100,
    `EffectBasePoints1` = 14,
    `EffectBasePoints2` = 39,
    `SpellDescription0` = 'You heal your pet for $s1% of the spell damage you deal, and $s2% of the damage dealt by your demon heals you.'
WHERE `ID` = 47231;
