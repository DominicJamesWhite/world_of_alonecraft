-- PlayerScript refactors: GlacierArmor, MagicAttunement, TidalWaves
-- Registers new AuraScript/SpellScript replacements and spell_proc entries.

-- ============================================================
-- GlacierArmor: SpellScript on Ice Block (replaces PlayerScript)
-- ============================================================
DELETE FROM `spell_script_names` WHERE `spell_id` = 45438 AND `ScriptName` = 'spell_glacier_armor_ice_block';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
    (45438, 'spell_glacier_armor_ice_block');  -- Ice Block -> check Glacier Armor, cast Healing Floes

-- ============================================================
-- MagicAttunement: AuraScript proc (replaces PlayerScript)
-- Remove old per-rank script names, add unified proc script
-- ============================================================
DELETE FROM `spell_script_names` WHERE `spell_id` IN (11247, 12606)
    AND `ScriptName` IN ('spell_magic_attunement_rank_1', 'spell_magic_attunement_rank_2', 'spell_magic_attunement_proc');
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
    (11247, 'spell_magic_attunement_proc'),  -- Magic Attunement R1
    (12606, 'spell_magic_attunement_proc');  -- Magic Attunement R2

-- spell_proc: fire on any spell cast (NONE_POS 0x400 | NONE_NEG 0x1000 | MAGIC_POS 0x4000 | MAGIC_NEG 0x10000 = 0x15400)
DELETE FROM `spell_proc` WHERE `SpellId` IN (11247, 12606);
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`,
    `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
    (11247, 0, 0, 0, 0, 0, 0x15400, 0, 2, 0, 0, 0, 0, 100, 0, 0),  -- Magic Attunement R1: 100% on any spell cast
    (12606, 0, 0, 0, 0, 0, 0x15400, 0, 2, 0, 0, 0, 0, 100, 0, 0);  -- Magic Attunement R2: 100% on any spell cast

-- ============================================================
-- Tidal Waves: SpellScript on LB/CL/Lava Burst (replaces PlayerScript)
-- Registered on the trigger spells, not the talent auras.
-- ============================================================
DELETE FROM `spell_script_names` WHERE `ScriptName` = 'spell_sha_tidal_waves_proc';
DELETE FROM `spell_script_names` WHERE `ScriptName` = 'spell_sha_tidal_waves_trigger';
DELETE FROM `spell_proc` WHERE `SpellId` IN (51562, 51563, 51564, 51565, 51566);
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
    (-403,   'spell_sha_tidal_waves_trigger'),  -- Lightning Bolt (all ranks)
    (-421,   'spell_sha_tidal_waves_trigger'),  -- Chain Lightning (all ranks)
    (-51505, 'spell_sha_tidal_waves_trigger');  -- Lava Burst (all ranks)
