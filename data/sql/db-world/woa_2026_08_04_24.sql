-- Alonecraft 4.61 -- Bladework (14185): halve the energy cost, 60 -> 30.
--
-- woa_2026_08_03_10.sql set Bladework's cost at 60 energy when it turned
-- Preparation into a combo point spender. In play that is far too steep for
-- what is a setup button rather than a damage finisher -- it competes with
-- Eviscerate for the same points *and* costs more energy than one.
--
-- Dirty Deeds still discounts it by 10/20 (its EFFECT_1 is a flat
-- SPELLMOD_COST on the rogue family bit Bladework carries), so the talented
-- cost drops from 50/40 to 20/10.
--
-- No description edit: the client renders the cost from ManaCost in the
-- spell header, and 14185's SpellDescription0 never quoted the number.
--
-- Ordering note: single-column UPDATE, no full-row re-INSERT, so this cannot
-- clobber the columns woa_2026_08_03_10.sql and woa_2026_08_04_21.sql set.

UPDATE `alonecraft_spell_dbc` SET
    `ManaCost` = 30
WHERE `ID` = 14185;
