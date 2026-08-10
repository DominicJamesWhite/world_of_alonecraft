-- ============================================================
-- Warrior (Protection): halve Sword and Board's rage, halve Barricade
-- ============================================================
--
-- Sword and Board
-- ---------------
-- The buff 50227 is an ADD_PCT_MODIFIER (108) with EffectMiscValue1 = 23
-- (SPELLMOD_EFFECT3), i.e. a percentage on Shield Slam's energize effect.  It
-- shipped at +100% in woa_2026_08_08_07.sql, which the rage audit already
-- flagged as pricing the proc at ~45 rage against retail's 20 -- and that was
-- before the proc also refreshed Shield Slam's cooldown.  Halved to +50%.
--
-- The three talent ranks (46951/46952/46953) need no change: they differ only
-- in proc chance and their descriptions read the buff with $50227s1, and the
-- buff's own tooltip is '$s1%', so both follow this number automatically.
--
-- Barricade
-- ---------
-- Benefit halved across the board; the 20 rage floor and 80 rage cap are
-- unchanged, so the same dump buys half as much.
--
--   block chance and value:  20% + 1% per extra rage   ->  10% + 1% per 2 rage
--   heal on block:           50% of block value         ->  25%
--
-- The block percentage is computed in WarriorBarricade.cpp, not here: it is a
-- function of rage actually spent, which nothing static can know.  The divisor
-- there moves from 10 to 20 in the same change.  At the 20 rage floor that is
-- 200/20 = 10%, and at the 80 rage cap 800/20 = 40% -- exactly "10%, then
-- another 1% for every 2 rage".
--
-- The heal percentage does live here, in Effect3's base points, which is why
-- it needs no code change.
--
-- Note the base points go to 24, not 25.  EffectDieSides3 is 0 and CalcValue is
-- BasePoints + max(1, DieSides), so the previous 50 was actually paying 51%.
-- 24 is a true 25%.  The three description strings are plain text rather than
-- $s3 because the buff's own base points are what the script reads, so they are
-- restated here by hand.
--
-- Single-column changes to existing custom spells, so these are UPDATEs, not
-- 234-column DELETE + INSERTs, which would revert every other column.
-- ============================================================

-- Sword and Board: +100% -> +50% rage on the next Shield Slam
UPDATE `alonecraft_spell_dbc` SET `EffectBasePoints1` = 49 WHERE `ID` = 50227;

-- Barricade: heal on block 50% (really 51%) -> 25% of block value
UPDATE `alonecraft_spell_dbc` SET `EffectBasePoints3` = 24 WHERE `ID` = 200652;

-- Barricade: the three player-visible strings
UPDATE `alonecraft_spell_dbc`
   SET `SpellDescription0` = 'Become a living barricade, increasing your shield block chance and value by 10% and converting each 2 additional points of rage into 1 additional percent (up to a maximum cost of 80 rage).  When you block with Barricade active, you heal for 25% of your block value.'
 WHERE `ID` = 200651;

UPDATE `alonecraft_spell_dbc`
   SET `SpellDescription0` = 'Shield block chance and block value increased.  Blocking an attack heals you for 25% of your block value.',
       `SpellToolTip0`     = 'Shield block chance and block value increased.  Blocking heals you for 25% of your block value.'
 WHERE `ID` = 200652;

UPDATE `alonecraft_spell_dbc` SET `SpellDescription0` = 'Heals you for 25% of your block value.' WHERE `ID` = 200653;
