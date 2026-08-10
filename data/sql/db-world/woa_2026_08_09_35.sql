-- ============================================================
-- Barricade: halve the block heal, and say so in the tooltips
-- ============================================================
-- Healing for the full block value on every block was far too strong once the
-- block-chance half of the talent was actually working -- an 80 rage Barricade
-- raises block chance AND block value, so the heal compounds twice over.
-- Halved to 50%.
--
-- The percentage is stored in the buff's Effect3 base points rather than
-- hardcoded in C++, so retuning it again is a one-column UPDATE with no
-- rebuild of anything but the DBC.  Effect3 is the SPELL_AURA_DUMMY the
-- heal-on-block proc hangs off; it carried no value before, so it was free
-- real estate.  EffectDieSides3 stays 0 -- SpellEffectInfo::CalcValue adds
-- nothing then, so GetAmount() returns exactly 50.
--
-- The 50 is written literally into both tooltips rather than as `$s3`, because
-- the client renders `$sN` as BasePoints + max(1, DieSides) and would print
-- 51.  Storing 49 to make the client read 50 would make the server heal for
-- 49%; the server value is the one that matters, so the text is literal.
--
-- CastCustomSpell passes values for effects 1 and 2 only (the block chance and
-- block value percentages) and nullptr for the third, so Effect3 keeps its DBC
-- base points.  That is what makes this work at all.
-- ============================================================

UPDATE `alonecraft_spell_dbc`
SET `EffectBasePoints3` = 50,
    `EffectDieSides3` = 0
WHERE `ID` = 200652;

-- Tooltips follow the number.
UPDATE `alonecraft_spell_dbc`
SET `SpellDescription0` = 'Become a living barricade, increasing your shield block chance and value by 20% and converting each extra point of rage into 1 additional percent (up to a maximum cost of 80 rage).  When you block with Barricade active, you heal for 50% of your block value.'
WHERE `ID` = 200651;

UPDATE `alonecraft_spell_dbc`
SET `SpellDescription0` = 'Shield block chance and block value increased.  Blocking an attack heals you for 50% of your block value.',
    `SpellToolTip0` = 'Shield block chance and block value increased.  Blocking heals you for 50% of your block value.'
WHERE `ID` = 200652;
