--
-- Pin the simulator's arena to 25 players, so an autobalanced boss there is
-- scaled the way a player actually meets one.
--
-- mod-autobalance scales a creature by the ratio of players present to the
-- map's nominal size. The arena is Emerald Dream (map 169), which retail
-- declares as a **40-player** raid -- so an autobalanced fight there scaled
-- 40 -> 1, which is a good deal harsher than anything the fork's content
-- actually presents.
--
-- Measured on Patchwerk, same boss, same actor, autobalance on:
--
--   arena as 40-player   340,959 HP
--   Naxxramas-25          467,293 HP     <- what a player meets
--
-- 27% adrift, in the direction that quietly makes every solo-viability number
-- look worse than the game does. Naxxramas-25 is the tier the fork is balanced
-- against, so the arena is pinned to match it.
--
-- Overrides MapDifficulty.dbc row **21** by id, so it replaces that row rather
-- than adding a second one for the same (map, difficulty) pair -- GetMapDifficulty
-- takes the first match, and two rows would make which one wins an accident of
-- load order. Every field is retail's except MaxPlayers.
--
-- Safe because nothing else uses Emerald Dream: it has no creatures, no quests,
-- no LFG queue (the row in woa_2026_08_14_01.sql is consulted only by
-- AutoBalance asking the map about its level band) and no player-facing entrance.
--
-- To model a different tier, change MaxPlayers here -- 10 for Naxxramas-10, 40
-- for a vanilla raid -- and say so in the commit, because it moves every
-- autobalanced number in the matrix.
--

DELETE FROM `mapdifficulty_dbc` WHERE `ID` = 21;
INSERT INTO `mapdifficulty_dbc`
  (`ID`, `MapID`, `Difficulty`, `Message_Lang_Mask`, `RaidDuration`, `MaxPlayers`,
   `Difficultystring`)
VALUES
(21, 169, 0, 16712188, 0, 25, '1');
