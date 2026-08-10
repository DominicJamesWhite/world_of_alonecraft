-- ============================================================
-- Warrior (Protection): Incite's cooldown reduction moves to C++
-- ============================================================
-- Second bug fix for woa_2026_08_09_09.sql.  The -15/30/45 sec on Mocking Blow
-- still did not happen in play after woa_2026_08_09_20.sql.
--
-- FIRST, CORRECT THE RECORD.  woa_2026_08_09_20.sql moved Mocking Blow's
-- cooldown from CategoryRecoveryTime to RecoveryTime on the theory that the
-- client will not apply spell modifiers to a CATEGORY cooldown.  That theory is
-- wrong, and retail disproves it.  Surveying dbc/base/retail/Spell.dbc for
-- aura 107 + EffectMiscValue 11 turns up 133 flat cooldown modifiers, and
-- several of the best known working ones target spells whose ENTIRE cooldown is
-- a category cooldown:
--
--   Improved Fire Blast -> Fire Blast   (2136, cat 19, rec 0, catrec 8000)
--   Improved Mind Blast -> Mind Blast   (8092, cat 19, rec 0, catrec 8000)
--   Veiled Shadows      -> Fade          (586, cat 82, rec 0, catrec 30000)
--   Endurance           -> Sprint/Evasion (cat 44/66, rec 0, catrec 180000)
--
-- So the category was never the problem.  The RecoveryTime layout is KEPT
-- anyway -- category 40 is a shared bucket (Raptor Strike, Cleave, Lash of Pain,
-- Sinister Strike), so keeping Mocking Blow's cooldown out of it is mildly
-- preferable -- but nobody should chase that theory a third time.
--
-- WHAT THE DATA LOOKS LIKE.  Everything in the data layer is correct and was
-- verified end to end: the alonecraft_spell_dbc rows, the built server DBC, and
-- the Spell.dbc extracted back out of the client's patch-4.MPQ all agree, and
-- the running worldserver was started after that DBC was written.  Incite's
-- EFFECT_1 is SpellFamilyName 4 / EffectSpellClassMaskA2 0x08000000, which is
-- exactly Mocking Blow's SpellFamilyFlags, with EffectMiscValue2 = 11
-- (SPELLMOD_COOLDOWN) and an amount of -15000/-30000/-45000.
--
-- The telling detail is that the DAMAGE half works in game.  EFFECT_0 is a spell
-- modifier on the SAME aura with the SAME class mask and family, and it reaches
-- Mocking Blow.  The aura is applied and its modifiers are registered on the
-- player; the cooldown modifier is simply not showing up where it is watched.
--
-- THE FIX.  Stop asking the DBC modifier to carry this and use the one call that
-- edits the server's cooldown record AND tells the client about it:
-- Player::ModifySpellCooldown (Player.cpp:11109), which sends
-- SMSG_MODIFY_COOLDOWN.  Core precedent is Elemental Mastery
-- (spell_shaman.cpp:1024).  This is a deliberate step away from the module's
-- DBC-first rule: the DBC route has had two attempts, Incite already has a C++
-- script, and this route is unambiguous on both sides of the wire.
--
-- EFFECT_1 therefore has to stop being a live modifier, or a server-side mod
-- that IS working plus the new script would reduce the cooldown twice.
--
-- It becomes SPELL_AURA_DUMMY and stays a data carrier:  EffectBasePoints2 is
-- deliberately LEFT at -15001/-30001/-45001 with EffectDieSides2 = 1, so the
-- description's $/1000;s2 still renders "15/30/45 sec" and the script reads the
-- same number off the aura effect.  One source of truth for the amount.
--
-- DO NOT try to disable the modifier by zeroing only the class mask.
-- SpellInfo::IsAffected (SpellInfo.cpp:1333) reads an empty familyFlags as
-- "match everything", so a masked-off ADD_FLAT_MODIFIER would apply -45 sec to
-- every warrior spell.  The aura type is what has to change.
--
-- Spells:
--   50685 / 50686 / 50687 = Incite ranks 1-3
--   694 (all ranks)       = Mocking Blow
-- ============================================================

UPDATE `alonecraft_spell_dbc`
   SET `EffectApplyAuraName2`   = 4,   -- SPELL_AURA_DUMMY, was 107 ADD_FLAT_MODIFIER
       `EffectMiscValue2`       = 0,   -- was 11 SPELLMOD_COOLDOWN
       `EffectSpellClassMaskA2` = 0
 WHERE `ID` IN (50685, 50686, 50687);

-- -694 covers all seven Mocking Blow ranks via spell_ranks.
DELETE FROM `spell_script_names`
      WHERE `ScriptName` = 'spell_warr_mocking_blow_incite_cd';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`)
     VALUES (-694, 'spell_warr_mocking_blow_incite_cd');
