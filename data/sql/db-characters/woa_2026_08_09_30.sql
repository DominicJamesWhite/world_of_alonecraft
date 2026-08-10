-- ============================================================
-- Warrior: force a talent reset after the Concussion Blow -> Barricade swap
-- ============================================================
-- Follows woa_2026_08_09_29.sql, which deleted the orphaned character_talent
-- rows for 12809 to stop Player::_LoadTalents asserting.
--
-- That stopped the crash but left the tree inconsistent.  Talent 148
-- (Vigilance, Protection tier 6 column 1) lists talent 152 as PrereqTalent_1,
-- so every character who had spent a point on Concussion Blow and then taken
-- Vigilance now holds a talent whose prerequisite is unmet, plus a refunded
-- point floating loose.  A prereq is only enforced at spend time, so nothing
-- errors -- the tree is just quietly in a state the client would never have
-- let you build.
--
-- Why the at_login flag rather than more DELETEs.  A talent reset is not just
-- "remove the rows": Player::resetTalents (Player.cpp:3729) walks every
-- talent rank and unlearns the spells it granted.  Deleting character_talent
-- by hand would leave all those abilities sitting in character_spell, paid
-- for by nothing.  Setting AT_LOGIN_RESET_TALENTS (0x04) hands the job to the
-- engine at next login (CharacterHandler.cpp:965), which calls
-- resetTalents(true) -- noResetCost, so it is free -- and clears the flag
-- itself.  This is exactly what `.reset talents <name>` does to an offline
-- character.
--
-- Why this is deliberately over-broad.  The precise set was the 15 characters
-- holding spell 12809, and _29 deleted those rows without recording their
-- GUIDs first, so that list no longer exists.  The recoverable upper bound is
-- "warriors with any talents at all" -- 64 characters, of which nearly all are
-- playerbots that re-spec on their own.  A warrior who never took Concussion
-- Blow loses their build here for no reason; that is the cost of the missing
-- GUID list, and it is cheaper than leaving broken trees in place.
--
-- Non-warriors are untouched: talent 152 is Protection, so no other class
-- could have held it.
--
-- Idempotent: OR-ing a bit that is already set is a no-op, and the engine
-- clears the flag once the reset runs.
-- ============================================================

UPDATE `characters` c
SET `at_login` = `at_login` | 4
WHERE c.`class` = 1
  AND EXISTS (SELECT 1 FROM `character_talent` t WHERE t.`guid` = c.`guid`);
