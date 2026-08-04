-- Alonecraft 4.61 -- revert the Soul Leech retune.
--
-- woa_2026_08_03_17.sql cut Soul Leech to a 50% heal at 10/20/30% chance.
-- Reverting to the original tuning: a 200% heal at 5/10/15% chance by rank.
-- The big, rare heal is the intended feel; the smaller, more frequent version
-- was tested and rejected.
--
-- EffectBasePoints1 199 + DieSides 1 = 200%.  ProcChance drives $h in the
-- description ("a $h% chance to return health equal to $s1% of the damage
-- caused"), so both the tooltip and the behaviour follow these two fields --
-- no text change is needed.
--
-- Single-column UPDATEs so the SpellVisual / tooltip / description work done
-- in the surrounding files is not disturbed.

UPDATE `alonecraft_spell_dbc`
SET `EffectBasePoints1` = 199, `ProcChance` = 5
WHERE `ID` = 30293;

UPDATE `alonecraft_spell_dbc`
SET `EffectBasePoints1` = 199, `ProcChance` = 10
WHERE `ID` = 30295;

UPDATE `alonecraft_spell_dbc`
SET `EffectBasePoints1` = 199, `ProcChance` = 15
WHERE `ID` = 30296;
