-- ============================================================
-- SkillLineAbility overrides: put custom spells in the right spellbook tab
-- ============================================================
-- Barricade (200651) was appearing under "General" instead of "Protection".
--
-- The spellbook tab is not decided by Talent.dbc.  Talent.dbc grants the
-- spell; the client then groups every known spell by looking it up in
-- SkillLineAbility.dbc and filing it under the tab for its SkillLine.  A spell
-- with no row there has no skill line, so it falls through to General.  This
-- is invisible for a talent *redesign*, because the original spell keeps its
-- existing row -- Concussion Blow 12809 had SkillLineAbility 6973 pointing at
-- SkillLine 257.  It only bites when a redesign introduces a brand new spell
-- id, as the Barricade rework did.
--
-- Warrior skill lines: 26 Arms, 256 Fury, 257 Protection.
--
-- Only 200651 needs a row.  200652 (the buff) and 200653 (the heal) are never
-- taught, so they never appear in a spellbook to be filed anywhere.
--
-- IDs use the 60000+ range, comfortably clear of retail's max of 31441.
--
-- build_dbc.py reads this table and patches SkillLineAbility.dbc into
-- patch-4.mpq; build_and_run.bat also copies it to the server's Data/dbc so
-- both sides agree on the skill association.
-- ============================================================

CREATE TABLE IF NOT EXISTS `skilllineability_dbc` (
    `ID` INT UNSIGNED NOT NULL,
    `SkillLine` INT UNSIGNED NOT NULL DEFAULT 0,
    `Spell` INT UNSIGNED NOT NULL DEFAULT 0,
    `RaceMask` INT UNSIGNED NOT NULL DEFAULT 0,
    `ClassMask` INT UNSIGNED NOT NULL DEFAULT 0,
    `ExcludeRace` INT UNSIGNED NOT NULL DEFAULT 0,
    `ExcludeClass` INT UNSIGNED NOT NULL DEFAULT 0,
    `MinSkillLineRank` INT UNSIGNED NOT NULL DEFAULT 0,
    `SupercededBySpell` INT UNSIGNED NOT NULL DEFAULT 0,
    `AcquireMethod` INT UNSIGNED NOT NULL DEFAULT 0,
    `TrivialSkillLineRankHigh` INT UNSIGNED NOT NULL DEFAULT 0,
    `TrivialSkillLineRankLow` INT UNSIGNED NOT NULL DEFAULT 0,
    `CharacterPoints_1` INT UNSIGNED NOT NULL DEFAULT 0,
    `CharacterPoints_2` INT UNSIGNED NOT NULL DEFAULT 0,
    PRIMARY KEY (`ID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4
  COMMENT='Client SkillLineAbility.dbc overrides -- spellbook tab placement';

-- ClassMask 1 = Warrior, matching Shield Block (4278) and Shield Wall (3427).
-- Concussion Blow's own row used ClassMask 0, which also works, but being
-- explicit costs nothing and matches the neighbouring Protection abilities.
DELETE FROM `skilllineability_dbc` WHERE `ID` = 60000;
INSERT INTO `skilllineability_dbc` (
    `ID`, `SkillLine`, `Spell`, `RaceMask`, `ClassMask`,
    `ExcludeRace`, `ExcludeClass`, `MinSkillLineRank`, `SupercededBySpell`,
    `AcquireMethod`, `TrivialSkillLineRankHigh`, `TrivialSkillLineRankLow`,
    `CharacterPoints_1`, `CharacterPoints_2`
) VALUES (
    60000, 257, 200651, 0, 1,
    0, 0, 0, 0,
    0, 0, 0,
    0, 0
);
