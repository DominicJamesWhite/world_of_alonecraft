-- Alonecraft 4.61 -- Imperious Flames: make the three ranks actually rank.
--
-- woa_2026_08_02_03.sql rewrote all three Improved Imp ranks (18694/18695/
-- 18696) with the same EffectBasePoints1 = 99, so rank 1 already handed out
-- the full +100% Firebolt bonus and ranks 2 and 3 bought nothing. The talent
-- pane showed "100%" three times because the description hardcoded the number
-- as literal text instead of using $s1.
--
-- Both halves are fixed here:
--
--   Rank   BasePoints   $s1 (= BasePoints + max(1, DieSides), DieSides = 1)
--     1        32                33%
--     2        66                67%
--     3        99               100%
--
-- The design target (TODO.md) is "Firebolt does 2x damage vs your Immolate" at
-- full investment, which is rank 3's +100%.
--
-- The Felguard's Immolation Aura half of the talent stays rank-agnostic --
-- WarlockDemonPets.cpp grants it on any of the three ranks -- so the wording
-- keeps it as a flat clause with no variable.
--
-- No C++ change needed: spell_warl_imperious_flames already reads the amount
-- off the talent aura (AddPct(damage, talent->GetAmount())) rather than
-- assuming 100.
--
-- Ordering note: single-column UPDATEs only, no full-row re-INSERT, so this
-- cannot clobber the other columns woa_2026_08_02_03.sql set.

UPDATE `alonecraft_spell_dbc` SET
    `EffectBasePoints1` = 32,
    `SpellDescription0` = 'Your Imp''s Firebolt deals $s1% additional damage to targets afflicted by your Immolate, and your Felguard learns Immolation Aura.'
WHERE `ID` = 18694;

UPDATE `alonecraft_spell_dbc` SET
    `EffectBasePoints1` = 66,
    `SpellDescription0` = 'Your Imp''s Firebolt deals $s1% additional damage to targets afflicted by your Immolate, and your Felguard learns Immolation Aura.'
WHERE `ID` = 18695;

UPDATE `alonecraft_spell_dbc` SET
    `EffectBasePoints1` = 99,
    `SpellDescription0` = 'Your Imp''s Firebolt deals $s1% additional damage to targets afflicted by your Immolate, and your Felguard learns Immolation Aura.'
WHERE `ID` = 18696;
