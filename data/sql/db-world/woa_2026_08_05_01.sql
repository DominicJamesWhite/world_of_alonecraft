-- Mana Feed (30326): Health Funnel half 5% -> 20% of the demon's maximum mana
-- per tick.
--
-- EFFECT_1 (EffectBasePoints2) is the SPELL_AURA_DUMMY holder read by
-- spell_warl_health_funnel_mana_feed (WarlockHealthFunnel.cpp).  DieSides is 1,
-- so the amount the script sees is BasePoints + 1 -- 19 yields 20.
--
-- EFFECT_0 (the demon-damage-returns-mana half, 5% of damage) is unchanged.
--
-- Single-column edit, so UPDATE rather than a full-row re-INSERT: a re-INSERT
-- would revert every other column set by woa_2026_08_02_03.sql.

UPDATE `alonecraft_spell_dbc`
SET `EffectBasePoints2` = 19,
    `SpellDescription0` = 'When your demons deal damage they return 5% of it to you as Mana, and your Health Funnel restores 20% of your demon''s maximum Mana each tick.'
WHERE `ID` = 30326;
