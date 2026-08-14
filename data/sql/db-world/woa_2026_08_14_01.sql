--
-- An LFG level band for the simulator's empty arena, so mod-autobalance will
-- scale creatures there.
--
-- The simulator can fight in an empty raid instance (Emerald Dream, map 169) so
-- that a solo-content measurement is not contaminated by an encounter script.
-- That matters concretely: fighting Patchwerk in his own chamber summons his
-- adds the moment he is attacked, and every other real boss room has its own
-- version of that problem. An empty map has none of them.
--
-- But an empty map could not be autobalanced, and the reason is not the one it
-- looks like. AutoBalance does NOT derive a map's level range from the creatures
-- living in it. It reads `lfgMinLevel`/`lfgMaxLevel` from the map's LFGDungeons
-- entry (ABUtils.cpp InitializeMap -> GetLFGDungeon), and then refuses to touch
-- any creature below 85% of the minimum or above 115% of the maximum
-- (ABAllCreatureScript.cpp:476-486).
--
-- Emerald Dream has no LFGDungeons row at all, so its range was (0 to 0) and
-- every creature was outside it:
--
--   Creature Patchwerk (83) | is a creature outside of the expected NPC level
--   range for the map (0 to 0), not modified.
--
-- So the fix is one row of LFG data rather than any creature at all. The map
-- stays completely empty, which was the whole point of choosing it.
--
-- Values copied from Naxxramas-25 (LFGDungeons id 159): MinLevel 80, MaxLevel
-- 83, Target 80/80/83. That band covers every level-80 actor and every level-83
-- raid boss the sim points at, which is the entire range the fork cares about.
--
-- Difficulty 0: the sim's instance of map 169 reports as "40-player Normal",
-- and GetLFGDungeon matches on (mapId, difficulty). A row at any other
-- difficulty is never found.
--
-- ID 900 is reserved for Alonecraft. Retail's highest LFGDungeons id is 294 and
-- 3.3.5a is a closed set, so there is no upstream growth to collide with.
--
-- `lfgdungeons_dbc` is a MySQL override layered over the client DBC at load
-- time, so this reaches the server without touching a .dbc or the client. It is
-- also invisible to players: nothing queues for Emerald Dream, and the row is
-- consulted only by AutoBalance asking a map about its level band.
--

DELETE FROM `lfgdungeons_dbc` WHERE `ID` = 900;
INSERT INTO `lfgdungeons_dbc`
  (`ID`, `Name_Lang_enUS`, `Name_Lang_Mask`,
   `MinLevel`, `MaxLevel`, `Target_Level`, `Target_Level_Min`, `Target_Level_Max`,
   `MapID`, `Difficulty`, `Flags`, `TypeID`, `Faction`, `TextureFilename`,
   `ExpansionLevel`, `Order_Index`, `Group_Id`)
VALUES
(900, 'Alonecraft Simulator Arena', 16712190,
 80, 83, 80, 80, 83,
 169, 0, 0, 2, -1, '',
 2, 2218, 2);
