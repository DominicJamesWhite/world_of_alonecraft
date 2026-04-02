-- Nature's Guardian Phase 2: Fix creature levels, ScriptNames, and unit_class

-- Earth Guardian: change level range to 1-80, use module-owned ScriptName
UPDATE `creature_template` SET `minlevel` = 1, `maxlevel` = 80,
    `ScriptName` = 'npc_natures_guardian_earth' WHERE `entry` = 200000;

-- Fire Guardian: change level range to 1-80, use module-owned ScriptName
UPDATE `creature_template` SET `minlevel` = 1, `maxlevel` = 80,
    `ScriptName` = 'npc_natures_guardian_fire' WHERE `entry` = 200001;

-- Water Guardian: change level range to 1-80, use module-owned ScriptName,
-- unit_class = 2 (caster, needs mana for Waterbolt)
UPDATE `creature_template` SET `minlevel` = 1, `maxlevel` = 80,
    `unit_class` = 2, `ScriptName` = 'npc_natures_guardian_water' WHERE `entry` = 200002;

-- Air Guardian: change level range to 1-80, use module-owned ScriptName,
-- unit_class = 2 (caster, needs mana for Lightning Bolt)
UPDATE `creature_template` SET `minlevel` = 1, `maxlevel` = 80,
    `unit_class` = 2, `ScriptName` = 'npc_natures_guardian_air' WHERE `entry` = 200003;

-- Nature's Guardian talent: set rank-specific BasePoints and spellpower description
-- DieSides1=1 so $s1 = BasePoints+1 (single value, no range display)
-- BasePoints1 = N-1 so tooltip $s1 displays N
-- Rank 1 = 20%, Rank 2 = 40%, Rank 3 = 60%, Rank 4 = 80%, Rank 5 = 100%
UPDATE `alonecraft_spell_dbc` SET `EffectBasePoints1` = 19, `EffectDieSides1` = 1,
    `SpellDescription0` = 'All your totems summon lesser elemental guardians. Guardian damage scales with $s1% of your spell power.'
    WHERE `ID` = 30881;
UPDATE `alonecraft_spell_dbc` SET `EffectBasePoints1` = 39, `EffectDieSides1` = 1,
    `SpellDescription0` = 'All your totems summon lesser elemental guardians. Guardian damage scales with $s1% of your spell power.'
    WHERE `ID` = 30883;
UPDATE `alonecraft_spell_dbc` SET `EffectBasePoints1` = 59, `EffectDieSides1` = 1,
    `SpellDescription0` = 'All your totems summon lesser elemental guardians. Guardian damage scales with $s1% of your spell power.'
    WHERE `ID` = 30884;
UPDATE `alonecraft_spell_dbc` SET `EffectBasePoints1` = 79, `EffectDieSides1` = 1,
    `SpellDescription0` = 'All your totems summon lesser elemental guardians. Guardian damage scales with $s1% of your spell power.'
    WHERE `ID` = 30885;
UPDATE `alonecraft_spell_dbc` SET `EffectBasePoints1` = 99, `EffectDieSides1` = 1,
    `SpellDescription0` = 'All your totems summon lesser elemental guardians. Guardian damage scales with $s1% of your spell power.'
    WHERE `ID` = 30886;
