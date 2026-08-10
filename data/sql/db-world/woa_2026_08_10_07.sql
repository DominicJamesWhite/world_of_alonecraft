-- ============================================================
-- SkillLineAbility rows for the Trap Launcher spells
-- ============================================================
-- Companion to woa_2026_08_10_06.sql, which creates the 19 castable
-- "Trap Launcher: <Trap>" spells (200700-200718) and their triggered
-- children (200720-200738).
--
-- Without a SkillLineAbility row a known spell has no skill line, and the
-- client files it under "General" rather than the tab it belongs to.  Every
-- retail hunter trap sits on SkillLine 51 (Survival) -- verified against
-- dbc/base/SkillLineAbility.dbc rows 7534 (Freezing Trap 1499), 7589
-- (Explosive Trap 13813), 15243 (Snake Trap 34600) and 20264 (Freezing
-- Arrow 60192).  The launched variants go in the same place.
--
-- Row shape is copied from those trap rows exactly: RaceMask 0,
-- ClassMask 4 (Hunter), MinSkillLineRank 1, everything else 0.
--
-- SupercededBySpell is deliberately 0, because it is 0 on every retail trap
-- row too.  Trap rank collapsing in the spellbook is not a client-side DBC
-- behaviour -- Player::addSpell walks spell_ranks and unlearns the
-- superseded rank server-side.  Those spell_ranks rows are in
-- woa_2026_08_10_06.sql.
--
-- Only the castable parents need rows.  The triggered children are never
-- taught, so they never appear in a spellbook to be filed anywhere.
--
-- IDs continue the 60000+ range (60000 is Barricade, woa_2026_08_09_32.sql).
-- ============================================================

DELETE FROM `skilllineability_dbc` WHERE `ID` BETWEEN 60001 AND 60019;
INSERT INTO `skilllineability_dbc` (
    `ID`, `SkillLine`, `Spell`, `RaceMask`, `ClassMask`,
    `ExcludeRace`, `ExcludeClass`, `MinSkillLineRank`, `SupercededBySpell`,
    `AcquireMethod`, `TrivialSkillLineRankHigh`, `TrivialSkillLineRankLow`,
    `CharacterPoints_1`, `CharacterPoints_2`
) VALUES
-- Freezing Trap ranks 1-3
(60001, 51, 200700, 0, 4, 0, 0, 1, 0, 0, 0, 0, 0, 0),
(60002, 51, 200701, 0, 4, 0, 0, 1, 0, 0, 0, 0, 0, 0),
(60003, 51, 200702, 0, 4, 0, 0, 1, 0, 0, 0, 0, 0, 0),
-- Frost Trap
(60004, 51, 200703, 0, 4, 0, 0, 1, 0, 0, 0, 0, 0, 0),
-- Immolation Trap ranks 1-8
(60005, 51, 200704, 0, 4, 0, 0, 1, 0, 0, 0, 0, 0, 0),
(60006, 51, 200705, 0, 4, 0, 0, 1, 0, 0, 0, 0, 0, 0),
(60007, 51, 200706, 0, 4, 0, 0, 1, 0, 0, 0, 0, 0, 0),
(60008, 51, 200707, 0, 4, 0, 0, 1, 0, 0, 0, 0, 0, 0),
(60009, 51, 200708, 0, 4, 0, 0, 1, 0, 0, 0, 0, 0, 0),
(60010, 51, 200709, 0, 4, 0, 0, 1, 0, 0, 0, 0, 0, 0),
(60011, 51, 200710, 0, 4, 0, 0, 1, 0, 0, 0, 0, 0, 0),
(60012, 51, 200711, 0, 4, 0, 0, 1, 0, 0, 0, 0, 0, 0),
-- Explosive Trap ranks 1-6
(60013, 51, 200712, 0, 4, 0, 0, 1, 0, 0, 0, 0, 0, 0),
(60014, 51, 200713, 0, 4, 0, 0, 1, 0, 0, 0, 0, 0, 0),
(60015, 51, 200714, 0, 4, 0, 0, 1, 0, 0, 0, 0, 0, 0),
(60016, 51, 200715, 0, 4, 0, 0, 1, 0, 0, 0, 0, 0, 0),
(60017, 51, 200716, 0, 4, 0, 0, 1, 0, 0, 0, 0, 0, 0),
(60018, 51, 200717, 0, 4, 0, 0, 1, 0, 0, 0, 0, 0, 0),
-- Snake Trap
(60019, 51, 200718, 0, 4, 0, 0, 1, 0, 0, 0, 0, 0, 0);
