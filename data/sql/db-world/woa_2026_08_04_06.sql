-- Alonecraft 4.61 -- Demonic Brutality: threat bonus reaches Torment/Suffering.
--
-- The 200412 carrier is SPELL_AURA_MOD_THREAT with MiscValue 1 (physical).  It
-- already covers every damage-derived threat the demons generate, including the
-- Felguard's Cleave (30213 chain) and Intercept (30151 chain) -- both are
-- SchoolMask 1, so their threat runs through
-- ThreatManager::CalculateModifiedThreat -> _singleSchoolModifiers[NORMAL].
--
-- Torment (3716 chain) and Suffering (17735 chain) are the exception.  Neither
-- generates threat from damage; both carry a flat SPELL_EFFECT_THREAT, and
-- Spell::EffectThreat (SpellEffects.cpp:3709) calls AddThreat with
-- ignoreModifiers = true, which skips CalculateModifiedThreat outright
-- (ThreatManager.cpp:39).  No aura, spell_threat row or SPELLMOD_THREAT can
-- reach them -- widening 200412's MiscValue to 127 would do nothing here.
--
-- spell_warl_demon_brutality_threat adds the talent-scaled bonus as a second
-- AddThreat on the SPELL_EFFECT_THREAT hit.  Negative spell IDs cover all
-- ranks: 3716 -> 47984 (R1-R8), 17735 -> 47990 (R1-R8).

DELETE FROM `spell_script_names`
WHERE `ScriptName` = 'spell_warl_demon_brutality_threat';

INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(-3716,  'spell_warl_demon_brutality_threat'),
(-17735, 'spell_warl_demon_brutality_threat');

-- Descriptions: the talent no longer applies to melee only.  $s2 is the
-- EFFECT_1 amount (67 / 134 / 200), so the numbers stay rank-correct.
UPDATE `alonecraft_spell_dbc`
SET `SpellDescription0` = 'Your Voidwalker''s and Felguard''s attacks, and your Voidwalker''s Torment and Suffering, generate $s2% additional threat.  The cooldown of your Voidwalker''s Suffering ability is reduced by 30 sec.'
WHERE `ID` = 18705;

UPDATE `alonecraft_spell_dbc`
SET `SpellDescription0` = 'Your Voidwalker''s and Felguard''s attacks, and your Voidwalker''s Torment and Suffering, generate $s2% additional threat.  The cooldown of your Voidwalker''s Suffering ability is reduced by 60 sec.'
WHERE `ID` = 18706;

UPDATE `alonecraft_spell_dbc`
SET `SpellDescription0` = 'Your Voidwalker''s and Felguard''s attacks, and your Voidwalker''s Torment and Suffering, generate $s2% additional threat.  The cooldown of your Voidwalker''s Suffering ability is reduced by 90 sec.'
WHERE `ID` = 18707;
