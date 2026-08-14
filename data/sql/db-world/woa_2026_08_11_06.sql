-- ============================================================
-- Beast Cleave: no initial threat on the hunter
-- ============================================================
--
-- Casting 13161 put ~30 threat on the HUNTER, on every creature already
-- fighting the pet.  Spell::HandleThreatSpells (Spell.cpp:5551) falls back to
-- `threat += m_spellInfo->SpellLevel` for any spell with no spell_threat row,
-- and 13161's SpellLevel is 30.  The spell is positive and lands on the pet,
-- so the amount is routed through pet->GetThreatMgr().ForwardThreatForAssistingMe
-- (ThreatManager.cpp:755) and credited to the hunter -- the opposite of what a
-- "commit to this pull" pet button wants.
--
-- This is a regression the redesign introduced rather than retail behaviour.
-- Aspect of the Beast was Category 47, and SpellMgr.cpp:3493 exempts every
-- category-47 hunter spell with SPELL_ATTR0_CU_NO_INITIAL_THREAT.  Beast
-- Cleave's row is Category 0 (it is no longer an aspect, and must not share
-- the aspect cooldown category), so the exemption stopped matching.
--
-- A flatMod of 0 makes HandleThreatSpells return at its `threat == 0.0f` check
-- before any target loop, which is the whole fix.  Core does the same for
-- Divine Protection (498,0,0,0) among others.
--
-- Chosen over the two alternatives:
--   * SPELL_ATTR1_NO_THREAT (0x400) on the DBC row would also work, but it is
--     a 234-column re-INSERT of a spell three other files already tune, and
--     the attribute additionally means "does not cause target to engage" --
--     more semantics than asked for.
--   * Category 47 would restore the exemption and the 1s aspect category
--     cooldown with it.
--
-- pctMod 1 / apPctMod 0 are the table defaults and are inert here: pctMod
-- scales damage and healing threat, and 13161 does neither.  The pet's splash
-- (200739) is untouched, so its damage still generates normal threat for the
-- PET, which is where it belongs.

DELETE FROM `spell_threat` WHERE `entry` = 13161;
INSERT INTO `spell_threat` (`entry`, `flatMod`, `pctMod`, `apPctMod`) VALUES
(13161, 0, 1, 0);
