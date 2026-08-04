-- Alonecraft 4.61 -- Demonic Brutality: threat bonus extends to the Felguard.
--
-- woa_2026_08_02_04.sql anchored the 200412 threat carrier to pet 1860
-- (Voidwalker) only.  The Felguard is the other tanking demon -- Demonic
-- Embrace's dodge half is already scoped to both (1860 and 17252) -- so the
-- threat half now matches.
--
-- This is a spell_pet_auras change and nothing else.  spell_warl_demon_brutality
-- (WarlockDemonPets.cpp:282) never looks at the pet's entry; it reads the
-- owner's talent rank in DoEffectCalcAmount and writes the amount into the
-- SPELL_AURA_MOD_THREAT effect.  So a new anchor row is the entire mechanism --
-- no C++ change, and the Felguard gets the same 67/133/200% by rank.
--
-- The Suffering cooldown half of the talent (EffectBasePoints1, a
-- SPELLMOD_COOLDOWN on the Voidwalker's own ability) is untouched and stays
-- Voidwalker-only by nature -- the Felguard has no Suffering.  The descriptions
-- are reworded to say which demon each clause applies to, and the threat number
-- moves to $s2 so it can no longer drift from the data.
--
-- Ordering note: the DELETE is scoped to the three (spell, pet, aura) triples
-- added here rather than reusing woa_2026_08_02_04.sql's
-- `WHERE aura BETWEEN 200406 AND 200424`, so re-running either file in any
-- order leaves both sets of rows intact.

DELETE FROM `spell_pet_auras`
WHERE `spell` IN (18705, 18706, 18707) AND `pet` = 17252 AND `aura` = 200412;

INSERT INTO `spell_pet_auras` (`spell`, `effectId`, `pet`, `aura`) VALUES
(18705, 1, 17252, 200412),
(18706, 1, 17252, 200412),
(18707, 1, 17252, 200412);


-- Tooltips: name both demons on the threat clause, pin Suffering to the
-- Voidwalker, and swap the hardcoded 67/133/200 for $s2.

UPDATE `alonecraft_spell_dbc` SET
    `SpellDescription0` = 'Your Voidwalker''s and Felguard''s melee attacks generate $s2% additional threat, and the cooldown of your Voidwalker''s Suffering ability is reduced by 30 sec.'
WHERE `ID` = 18705;

UPDATE `alonecraft_spell_dbc` SET
    `SpellDescription0` = 'Your Voidwalker''s and Felguard''s melee attacks generate $s2% additional threat, and the cooldown of your Voidwalker''s Suffering ability is reduced by 60 sec.'
WHERE `ID` = 18706;

UPDATE `alonecraft_spell_dbc` SET
    `SpellDescription0` = 'Your Voidwalker''s and Felguard''s melee attacks generate $s2% additional threat, and the cooldown of your Voidwalker''s Suffering ability is reduced by 90 sec.'
WHERE `ID` = 18707;
