-- Alonecraft 4.61 -- Demonic Aegis: the demon shares the crit avoidance.
--
-- The talent's crit half (EFFECT_1 of 30143/30144/30145, 2/4/6% by rank) now
-- applies to the summoned demon as well as the warlock.
--
--
-- WHY THREE CARRIERS AND NOT ONE
--
--   200417, the buff the warlock gets, cannot be reused as a pet aura.  Its
--   base points are all zero -- spell_warl_demonic_aegis_armor writes the real
--   amounts at runtime with CastCustomSpell (WarlockDemonology.cpp:200) -- and
--   Unit::CastPetAura (Unit.cpp:13776-13782) passes base points through for
--   exactly one spell in the entire core, 35696 Demonic Knowledge.  Every other
--   pet aura is a plain CastSpell, so the amount has to already be in the DBC.
--
--   Hence one carrier per rank, each with its own fixed amount, rather than the
--   single-carrier + DoEffectCalcAmount shape Demonic Brutality and Demonic
--   Resilience use.  That shape needs a C++ AuraScript to read the owner's rank;
--   this one needs no C++ at all.
--
--     rank   talent   carrier   crit avoidance
--       1    30143    200508        -2%
--       2    30144    200509        -4%
--       3    30145    200510        -6%
--
--   The literal numbers in the carrier tooltips are safe here in a way they
--   were not for Imperious Flames: each carrier holds exactly one rank's value,
--   and the tooltip is set in the same UPDATE as the base points it describes.
--   The talent descriptions themselves use $s2, which already renders 2/4/6.
--
--
-- NOT GATED ON DEMON ARMOR
--
--   The warlock's own bonus only exists while Demon Armor is up, because it is
--   applied from an AuraScript on the Demon Armor rank chain.  The demon's is
--   applied from spell_pet_auras, so it is present whenever the talent is, Fel
--   Armor included.  Deliberate: spell_pet_auras re-applies on every summon and
--   on respec for free, which a cast from the Demon Armor script would not --
--   a demon summoned after Demon Armor was already up would get nothing.
--
--
-- CARRIER CONSTRUCTION
--
--   Cloned from 200417 through a temporary table rather than transcribed as
--   three more 234-column INSERTs.  Effect1 (the armor multiplier) is cleared;
--   Effect2 (aura 187, MOD_ATTACKER_MELEE_CRIT_CHANCE) and Effect3 (aura 179,
--   MOD_ATTACKER_SPELL_CRIT_CHANCE, school mask 126) keep 200417's layout and
--   gain the per-rank amount.  DieSides stays 0, so CalcValue is the base point
--   exactly -- SpellInfo.cpp:431-437 only adds to the roll when DieSides is
--   non-zero.

DROP TEMPORARY TABLE IF EXISTS `woa_aegis_pet_tmp`;
CREATE TEMPORARY TABLE `woa_aegis_pet_tmp`
    AS SELECT * FROM `alonecraft_spell_dbc` WHERE `ID` = 200417;

UPDATE `woa_aegis_pet_tmp` SET
    `Effect1` = 0, `EffectApplyAuraName1` = 0, `EffectMiscValue1` = 0,
    `EffectImplicitTargetA1` = 0, `EffectBasePoints1` = 0, `EffectDieSides1` = 0,
    `EffectDieSides2` = 0, `EffectDieSides3` = 0,
    `SpellDescription0` = 'Chance to be critically hit reduced.';

DELETE FROM `alonecraft_spell_dbc` WHERE `ID` IN (200508, 200509, 200510);

UPDATE `woa_aegis_pet_tmp` SET `ID` = 200508,
    `EffectBasePoints2` = -2, `EffectBasePoints3` = -2,
    `SpellToolTip0` = 'Chance to be critically hit reduced by 2%.';
INSERT INTO `alonecraft_spell_dbc` SELECT * FROM `woa_aegis_pet_tmp`;

UPDATE `woa_aegis_pet_tmp` SET `ID` = 200509,
    `EffectBasePoints2` = -4, `EffectBasePoints3` = -4,
    `SpellToolTip0` = 'Chance to be critically hit reduced by 4%.';
INSERT INTO `alonecraft_spell_dbc` SELECT * FROM `woa_aegis_pet_tmp`;

UPDATE `woa_aegis_pet_tmp` SET `ID` = 200510,
    `EffectBasePoints2` = -6, `EffectBasePoints3` = -6,
    `SpellToolTip0` = 'Chance to be critically hit reduced by 6%.';
INSERT INTO `alonecraft_spell_dbc` SELECT * FROM `woa_aegis_pet_tmp`;

DROP TEMPORARY TABLE `woa_aegis_pet_tmp`;


-- Anchor each carrier to its own talent rank.  effectId 1 is the talent's
-- SPELL_AURA_DUMMY crit effect, which is what SpellMgr.cpp:2472 requires;
-- pet 0 means every demon, not just the tanking two.

DELETE FROM `spell_pet_auras` WHERE `aura` IN (200508, 200509, 200510);

INSERT INTO `spell_pet_auras` (`spell`, `effectId`, `pet`, `aura`) VALUES
(30143, 1, 0, 200508),
(30144, 1, 0, 200509),
(30145, 1, 0, 200510);


-- Talent tooltips: name the demon, and move the two numbers to $s1/$s2.

UPDATE `alonecraft_spell_dbc` SET
    `SpellDescription0` = 'Your Demon Armor increases your armor by an additional $s1% and reduces your chance to be critically hit by $s2%.  Your demon''s chance to be critically hit is reduced by $s2% at all times.'
WHERE `ID` IN (30143, 30144, 30145);
