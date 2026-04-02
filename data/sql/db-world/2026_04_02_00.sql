-- ============================================================
-- Add buff tooltips to custom shaman spells
-- These show when hovering the buff icon on the buff bar
-- ============================================================

-- Cleared Mind (Focused Mind buff - next LB/CL boosted)
UPDATE `alonecraft_spell_dbc` SET `SpellToolTip0` = 'Your next Lightning Bolt or Chain Lightning deals bonus damage and costs less mana.' WHERE `ID` = 200209;

-- Ancestral Fury (guaranteed crit buff from Ancestral Awakening)
UPDATE `alonecraft_spell_dbc` SET `SpellToolTip0` = 'Your next damaging spell is guaranteed to critically hit.' WHERE `ID` = 200210;

-- Surging Focus (Healing Focus - instant free HW)
UPDATE `alonecraft_spell_dbc` SET `SpellToolTip0` = 'Your next Healing Wave is instant cast and costs no mana.' WHERE `ID` = 200211;

-- Focused Assault (Healing Focus - damage buff)
UPDATE `alonecraft_spell_dbc` SET `SpellToolTip0` = 'Your damage is increased by 25%.' WHERE `ID` = 200212;

-- Spiritsurge (empowered shocks buff)
UPDATE `alonecraft_spell_dbc` SET `SpellToolTip0` = 'Your shock spells are empowered by the elements.' WHERE `ID` = 200217;

-- Elemental Freeze (Spiritsurge Frost Shock root)
UPDATE `alonecraft_spell_dbc` SET `SpellToolTip0` = 'Frozen in place.' WHERE `ID` = 200219;

-- Tidal Surge (heal crit buff from Tidal Waves)
UPDATE `alonecraft_spell_dbc` SET `SpellToolTip0` = 'Your next healing spell has an increased chance to critically hit.' WHERE `ID` = 200220;

-- Riptide Damage DoT (hostile Riptide)
UPDATE `alonecraft_spell_dbc` SET `SpellToolTip0` = 'Suffering Nature damage every 3 sec.' WHERE `ID` = 200221;


-- ============================================================
-- Fix Cleansing Earth (200214) AoE targeting
-- TargetB must be 15 (TARGET_UNIT_SRC_AREA_ENEMY) to select enemies.
-- Also zero out stale Effect2 fields inherited from base spell.
-- ============================================================
UPDATE `alonecraft_spell_dbc`
SET `EffectImplicitTargetB1` = 15,
    `EffectApplyAuraName2` = 0,
    `EffectBasePoints2` = 0
WHERE `ID` = 200214;


-- ============================================================
-- Defence of Nature: SpellScript on shield triggered spells
-- ============================================================
-- Registers on all shield proc spells so the script fires only
-- when a shield actually triggers (not on every damage taken).
-- Earth Shield heal (379), Lightning Shield damage (26363-26372,
-- 49280-49281), Water Shield mana (23575, 33737, 52128, 52130,
-- 52132, 52133, 52135, 52137, 57961).

DELETE FROM `spell_script_names` WHERE `ScriptName` = 'spell_sha_defence_of_nature';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
    -- Earth Shield heal
    (379,   'spell_sha_defence_of_nature'),
    -- Lightning Shield damage (all ranks)
    (26363, 'spell_sha_defence_of_nature'),
    (26364, 'spell_sha_defence_of_nature'),
    (26365, 'spell_sha_defence_of_nature'),
    (26366, 'spell_sha_defence_of_nature'),
    (26367, 'spell_sha_defence_of_nature'),
    (26369, 'spell_sha_defence_of_nature'),
    (26370, 'spell_sha_defence_of_nature'),
    (26371, 'spell_sha_defence_of_nature'),
    (26372, 'spell_sha_defence_of_nature'),
    (49280, 'spell_sha_defence_of_nature'),
    (49281, 'spell_sha_defence_of_nature'),
    -- Water Shield mana restore (all ranks)
    (23575, 'spell_sha_defence_of_nature'),
    (33737, 'spell_sha_defence_of_nature'),
    (52128, 'spell_sha_defence_of_nature'),
    (52130, 'spell_sha_defence_of_nature'),
    (52132, 'spell_sha_defence_of_nature'),
    (52133, 'spell_sha_defence_of_nature'),
    (52135, 'spell_sha_defence_of_nature'),
    (52137, 'spell_sha_defence_of_nature'),
    (57961, 'spell_sha_defence_of_nature');
