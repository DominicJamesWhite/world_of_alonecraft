-- Alonecraft -- Feint parry bonus halved and flattened across ranks.
--
-- woa_2026_08_03_00.sql rebuilt Feint as a parry buff ramping 20 -> 50% across
-- its eight ranks, landing on Evasion's 50% at rank 8. In play that is too
-- strong: Feint has a 10 s cooldown against Evasion's 5 minutes, so the "spare
-- Evasion" was strictly better than the real one for sustained avoidance.
--
-- Every rank is now a flat 20%. The rank ramp only ever existed because the
-- base spell scaled its threat drop with level; as an avoidance cooldown there
-- is nothing to gain from making low ranks weaker, and a single number keeps
-- Sleight of Hand's boost predictable at every level.
--
-- $s1 renders BasePoints + max(1, DieSides) and DieSides is 1 on every rank, so
-- BasePoints is the displayed value minus one -> 19.
--
-- Single-column UPDATEs, not full-row INSERTs: rank 8's row also carries the
-- Effect2 AoE damage reduction and woa_2026_08_03_20.sql set SpellToolTip0 on
-- ranks 1-7. A re-INSERT here would silently revert both.
--
-- Sleight of Hand (30892/30893) is untouched -- it is an ADD_PCT_MODIFIER on
-- Feint's effects, so its 20/40% boost scales with the new value for free
-- (24% / 28% parry with the talent).

UPDATE `alonecraft_spell_dbc`
SET `EffectBasePoints1` = 19
WHERE `ID` IN (1966, 6768, 8637, 11303, 25302, 27448, 48658, 48659);
