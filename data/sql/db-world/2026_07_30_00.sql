-- ============================================================
-- Drop the stale Bone Shield script registration.
--
-- 2026_03_31_15.sql restored the core registration
-- 49222 -> 'spell_dk_bone_shield'. Upstream has since deleted that
-- SpellScript outright (double charge consumption, PR #25439) along
-- with its own registration, so ours now points at code that no
-- longer exists and the server logs "assigned in the database, but
-- has no code!" on every startup.
--
-- The Bone Harvest half of the rework (200117 / 200119) is unaffected.
-- ============================================================

DELETE FROM `spell_script_names`
    WHERE `spell_id` = 49222 AND `ScriptName` = 'spell_dk_bone_shield';
