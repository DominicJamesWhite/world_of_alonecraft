--
-- Correction: restore the row woa_2026_08_14_02.sql wrote.
--
-- The first version of this file "fixed" the simulator arena's raid size from
-- 40 to 25. It was wrong, and the mistake is worth keeping written down because
-- it is easy to repeat.
--
-- The reasoning was: mod-autobalance takes its baseline from the map the fight
-- happens on (getInflectionPointSettings, ABUtils.cpp:929), the arena is
-- Emerald Dream rather than Naxxramas, and reading the binary
--     C:\Build\bin\RelWithDebInfo\Data\dbc\MapDifficulty.dbc
-- shows map 169 with maxPlayers = 40 against Naxxramas's 25. All true, and the
-- conclusion did not follow: `mapdifficulty_dbc` is layered over the binary at
-- load time (LOAD_DBC, DBCStores.cpp:353), and woa_2026_08_14_02.sql had
-- already written exactly this row. The runtime value was 25 the whole time.
--
-- The probes that "tested" the change therefore both ran at 25, which is why
-- they landed 0.5% apart (366,488 and 364,639 hp) -- not because the curve was
-- saturated, but because nothing changed between them. The saturation argument
-- may still be true; it was not what those two numbers showed, and it should
-- not be cited as though it were.
--
-- READ THE OVERRIDE TABLE, NOT THE BINARY. Every *_dbc table in acore_world
-- reaches the server without a file copy and without touching the client. A
-- binary DBC read tells you what the file says, never what the server uses.
--
-- The row below is byte-identical to woa_2026_08_14_02.sql's, including
-- Difficultystring = '1', which the withdrawn version of this file had
-- overwritten with ''. Applying it returns the table to that state.
--

DELETE FROM `mapdifficulty_dbc` WHERE `ID` = 21;
INSERT INTO `mapdifficulty_dbc`
  (`ID`, `MapID`, `Difficulty`, `Message_Lang_Mask`, `RaidDuration`, `MaxPlayers`,
   `Difficultystring`)
VALUES
(21, 169, 0, 16712188, 0, 25, '1');
