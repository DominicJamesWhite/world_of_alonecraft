-- Fix swapped PetTalentMask on the Ferocity and Tenacity pet talent tabs.
--
-- `talenttab_dbc` is layered over the binary TalentTab.dbc at startup by
-- LoadDBC (DBCStores.cpp:387), and the table wins. Its rows for the two pet
-- tabs have their masks exchanged relative to the DBC:
--
--   tab 409 Tenacity : table 1, DBC 2
--   tab 410 Ferocity : table 2, DBC 1
--   tab 411 Cunning  : table 4, DBC 4   (correct)
--
-- The mask must be `1 << CreatureFamily.petTalentType`, and petTalentType is
-- 0 Ferocity / 1 Tenacity / 2 Cunning. Player::LearnPetTalent tests
-- `(1 << petTalentType) & petTalentMask` (Player.cpp:14275) and returns
-- silently when it fails, so with the masks swapped a Ferocity pet computes
-- `1 & 2` and a Tenacity pet computes `2 & 1` -- both zero. Every talent for
-- those two pet types was rejected with no error; only Cunning pets worked.
--
-- The client reads its own unpatched TalentTab.dbc out of the MPQ, so it drew
-- the correct tree and let points be previewed. Only the server disagreed,
-- which is why "Learn" appeared to do nothing: the batch was rejected entry by
-- entry and the reply reset the preview.
--
-- BuildPetTalentsInfoData (Player.cpp:14643) filters on the same mask, so this
-- also fixes the learned-talent list the server sends back for those pets.

UPDATE `talenttab_dbc` SET `PetTalentMask` = 2 WHERE `ID` = 409; -- Tenacity
UPDATE `talenttab_dbc` SET `PetTalentMask` = 1 WHERE `ID` = 410; -- Ferocity
UPDATE `talenttab_dbc` SET `PetTalentMask` = 4 WHERE `ID` = 411; -- Cunning
