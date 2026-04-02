-- ============================================================
-- Focused Mind: refactor from PlayerScript to AuraScript
-- Register AuraScript on talent ranks, add spell_proc entries
-- ============================================================

-- Script registration for the talent AuraScript
DELETE FROM `spell_script_names` WHERE `ScriptName` = 'spell_sha_focused_mind';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(30864, 'spell_sha_focused_mind'),
(30865, 'spell_sha_focused_mind'),
(30866, 'spell_sha_focused_mind');

-- Proc config: fire on positive magic spell hit, filtered to heal family flags
-- ProcFlags 0x4000 = PROC_FLAG_DONE_SPELL_MAGIC_DMG_CLASS_POS (matches Serendipity pattern)
-- SpellPhaseMask 2 = PROC_SPELL_PHASE_HIT (required — 0 means no phase matches)
-- SpellFamilyName 11 = Shaman
-- SpellFamilyMask0 0x1C0 = HW (0x40) + LHW (0x80) + CH (0x100)
DELETE FROM `spell_proc` WHERE `SpellId` IN (30864, 30865, 30866);
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(30864, 0, 11, 0x1C0, 0, 0, 0x4000, 0, 2, 0, 0, 0, 0, 100, 0, 0),
(30865, 0, 11, 0x1C0, 0, 0, 0x4000, 0, 2, 0, 0, 0, 0, 100, 0, 0),
(30866, 0, 11, 0x1C0, 0, 0, 0x4000, 0, 2, 0, 0, 0, 0, 100, 0, 0);
