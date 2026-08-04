-- Alonecraft 4.61 -- Retune the Shadow Dance cooldown reduction on
-- Camouflage (13975, 14062, 14063) and Elusiveness (13981, 14066).
--
-- woa_2026_08_03_01.sql gave both talents a new effect-3 ADD_FLAT_MODIFIER
-- (aura 107, EffectMiscValue 11 = SPELLMOD_COOLDOWN) covering Shadow Dance.
-- Neither magnitude was chosen for Shadow Dance: Camouflage's effect 3
-- inherited its effect 2 value (the Stealth reduction, -2/-4/-6 s) and
-- Elusiveness' inherited its effect 1 value (the Vanish/Blind reduction,
-- -30/-60 s). One shaves 6 s off Shadow Dance, the other 60 s.
--
-- Elusiveness rank 2 is a bug, not just a tuning miss. Shadow Dance's
-- RecoveryTime is exactly 60000, so -60 s zeroes it -- Player.cpp:10985
-- clamps a negative rec to 0 and then returns *before* setting any cooldown
-- at all. 2/2 Elusiveness makes Shadow Dance free-cast.
--
-- The root cause is that Elusiveness' effect 3 serves two cooldowns of very
-- different length from a single base-points value: Evasion (180 s, and note
-- it lives in CategoryRecoveryTime, category 66) and Shadow Dance (60 s).
-- No number is right for both.
--
--
-- ELUSIVENESS -- re-slot, then retune
--
--   Evasion moves off effect 3 and onto effect 2, which is already the
--   Cloak of Shadows reduction at -15/-30 s. Cloak is 90 s and Evasion is
--   180 s, so one value serves both and Evasion's reduction is unchanged
--   from what shipped. That leaves effect 3 for Shadow Dance alone at
--   -5/-10 s.
--
--   Effect 1 (Vanish/Blind, mask A1 = 16779264, -30/-60 s) is untouched.
--
--   CLASS-MASK COLUMN TRAP (same one called out in woa_2026_08_03_01.sql):
--   EffectSpellClassMask{X}{n} is effect X, word n. Evasion is word A bit 5,
--   so moving it from effect 3 to effect 2 means C1 -> B1, not C1 -> C2.
--   EffectSpellClassMaskB2 = 65536 (Cloak of Shadows) must survive, so B1 is
--   set rather than the whole word cleared and rewritten.
--
--   The description has to change with the slots -- Evasion moves out of the
--   S3 clause and into the S2 one.
--
--
-- CAMOUFLAGE -- retune only
--
--   Masks and effect layout are already right (C2 = 33554944 covers both
--   Shadowstep word B bit 9 and Shadow Dance word B bit 25); only the
--   magnitude moves, to -10 s at rank 3. Shadowstep is a 30 s cooldown and
--   Shadow Dance 60 s, and -10 s is a sensible share of both. Ranks 1-2
--   interpolate in even 3 s steps.
--
--   No description edit: the existing text already reads the value out of
--   effect 3 via $/1000;s3.
--
--
-- Resulting cooldowns, fully talented Subtlety:
--   Shadow Dance      60 s - 10 (Elusiveness 2/2) - 10 (Camouflage 3/3) = 40 s
--   Shadowstep        30 s - 10                                        = 20 s
--   Evasion          180 s - 30                                        = 150 s  (unchanged)
--   Cloak of Shadows  90 s - 30                                        = 60 s   (unchanged)
--
-- Ordering note: single-column UPDATEs, no full-row re-INSERT, so this
-- cannot clobber the other ~230 columns woa_2026_08_03_01.sql set on these
-- five rows. This filename sorts after that one, so these values win.
--
-- Base points carry the usual -1 offset (-10001 = -10 s) against
-- EffectDieSides3 = 1, which woa_2026_08_03_01.sql already set.

-- Camouflage rank 1: Shadowstep and Shadow Dance -2 s -> -4 s
UPDATE `alonecraft_spell_dbc` SET
    `EffectBasePoints3` = -4001
WHERE `ID` = 13975;

-- Camouflage rank 2: Shadowstep and Shadow Dance -4 s -> -7 s
UPDATE `alonecraft_spell_dbc` SET
    `EffectBasePoints3` = -7001
WHERE `ID` = 14062;

-- Camouflage rank 3: Shadowstep and Shadow Dance -6 s -> -10 s
UPDATE `alonecraft_spell_dbc` SET
    `EffectBasePoints3` = -10001
WHERE `ID` = 14063;

-- Elusiveness rank 1: Evasion effect 3 -> effect 2, Shadow Dance -30 s -> -5 s
UPDATE `alonecraft_spell_dbc` SET
    `EffectSpellClassMaskB1` = 32,
    `EffectSpellClassMaskC1` = 0,
    `EffectBasePoints3`      = -5001,
    `SpellDescription0`      = 'Reduces the cooldown of your Vanish and Blind abilities by $/1000;S1 sec, your Cloak of Shadows and Evasion abilities by $/1000;S2 sec, and your Shadow Dance ability by $/1000;S3 sec.'
WHERE `ID` = 13981;

-- Elusiveness rank 2: Evasion effect 3 -> effect 2, Shadow Dance -60 s -> -10 s
UPDATE `alonecraft_spell_dbc` SET
    `EffectSpellClassMaskB1` = 32,
    `EffectSpellClassMaskC1` = 0,
    `EffectBasePoints3`      = -10001,
    `SpellDescription0`      = 'Reduces the cooldown of your Vanish and Blind abilities by $/1000;S1 sec, your Cloak of Shadows and Evasion abilities by $/1000;S2 sec, and your Shadow Dance ability by $/1000;S3 sec.'
WHERE `ID` = 14066;
