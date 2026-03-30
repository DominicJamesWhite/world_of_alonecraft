-- Alonecraft: Fix spell_script_names and spell_proc startup errors
-- Removes incorrect script registrations and fixes missing ProcFlags.

-- Fix: Remove 200037 from spell_script_names.
-- 200037 is an aura spell (Aura 108/108/4), not a damage spell.
-- The -30455 family registration already covers Ice Lance (SPELL_EFFECT_SCHOOL_DAMAGE).
DELETE FROM `spell_script_names` WHERE `spell_id` = 200037;

-- Fix: Remove 42463/53739 (Seal of Vengeance/Corruption damage effects).
-- These are SPELL_EFFECT_WEAPON_DAMAGE_NOSCHOOL spells, not seal auras.
-- The core script spell_pal_seal_of_vengeance expects SPELL_AURA_DUMMY which they lack.
DELETE FROM `spell_script_names` WHERE `spell_id` IN (42463, 53739) AND `ScriptName` = 'spell_pal_seal_of_vengeance';

-- Fix: Remove -724 (Lightwell family) from spell_pri_lightwell.
-- Spell 724 is SPELL_EFFECT_SUMMON, but spell_pri_lightwell expects SPELL_EFFECT_SCRIPT_EFFECT.
-- The module's Lightform is handled by (200060, 'spell_priest_aura_interaction').
DELETE FROM `spell_script_names` WHERE `spell_id` = -724 AND `ScriptName` = 'spell_pri_lightwell';

-- Fix: Remove -33880 from spell_proc (not first rank; -33879 covers all ranks).
DELETE FROM `spell_proc` WHERE `SpellId` = -33880;

-- Fix: Practiced Silence (Improved Counterspell) ProcFlags=0 in both DBC and base spell_proc.
-- Counterspell is DamageClass=0 (none) negative spell, so use PROC_FLAG_DONE_SPELL_NONE_DMG_CLASS_NEG (4096).
UPDATE `spell_proc` SET `ProcFlags` = 4096 WHERE `SpellId` = -11255 AND `ProcFlags` = 0;

-- Fix: Set ProcFlags for entries that had ProcFlags=0 (procs never triggered).
-- Nature's Grace: procs on Wrath/Starfire damage hits.
UPDATE `spell_proc` SET `ProcFlags` = 65536 WHERE `SpellId` = -33879 AND `ProcFlags` = 0;

-- Swiftmend trigger: procs when Swiftmend is cast (AoE damage explosion).
UPDATE `spell_proc` SET `ProcFlags` = 16384 WHERE `SpellId` = 200016 AND `ProcFlags` = 0;

-- Gift of Prophecy R1-R3: procs on melee auto-attack hits.
UPDATE `spell_proc` SET `ProcFlags` = 4 WHERE `SpellId` IN (200056, 200058, 200059) AND `ProcFlags` = 0;
