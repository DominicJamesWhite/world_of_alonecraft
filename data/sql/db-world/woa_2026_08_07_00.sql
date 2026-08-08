-- ==========================================================================
-- Riding skill costs: 10x cut to match the 10x XP rate
-- ==========================================================================
--
-- XP runs at 10x but gold income is still 1x, so a character reaches level 40
-- with about a tenth of the gold retail assumed.  The riding gates were paced
-- against the retail levelling curve; at 10x they are walls that never existed
-- in the original game.  Dividing by 10 restores the original ratio of
-- gold-earned to gold-owed rather than removing the gate.
--
-- Selected by trainer_spell.ReqSkillLine = 762 (Riding), which also covers the
-- druid Swift Flight Form -- that is a riding cost like any other.  Keyed on
-- SpellId, not TrainerId, because each rank sits on every riding trainer.
--
-- npc_trainer is deliberately untouched: it is legacy, and no C++ in src/
-- reads it (the live path is trainer / trainer_spell / creature_default_trainer).
--
-- Player::GetReputationPriceDiscount still takes up to 20% off these at
-- Exalted.  Unchanged and intended.
--
-- Absolute values, not `MoneyCost / 10`: module SQL is re-applied at startup on
-- top of any manual pre-application, and relative arithmetic would divide twice.
-- Regenerate with tools/gen_mount_prices.py, which carries the same divisor.


UPDATE `trainer_spell` SET `MoneyCost` = 4000 WHERE `SpellId` = 33388;   -- Apprentice Riding (lvl 20): 4g -> 40s
UPDATE `trainer_spell` SET `MoneyCost` = 50000 WHERE `SpellId` = 33391;   -- Journeyman Riding (lvl 40): 50g -> 5g
UPDATE `trainer_spell` SET `MoneyCost` = 250000 WHERE `SpellId` = 34090;   -- Expert Riding (lvl 60): 250g -> 25g
UPDATE `trainer_spell` SET `MoneyCost` = 5000000 WHERE `SpellId` = 34091;   -- Artisan Riding (lvl 70): 5000g -> 500g
UPDATE `trainer_spell` SET `MoneyCost` = 20000 WHERE `SpellId` = 40120;   -- Swift Flight Form (lvl 71): 20g -> 2g
UPDATE `trainer_spell` SET `MoneyCost` = 1000000 WHERE `SpellId` = 54197;   -- Cold Weather Flying (lvl 77): 1000g -> 100g
