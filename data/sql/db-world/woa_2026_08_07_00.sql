-- ==========================================================================
-- Riding skill costs: restored to retail
-- ==========================================================================
--
-- This file used to divide every riding cost by 10, on the reasoning that XP
-- ran at 10x while gold income ran at 1x.  That reasoning was sound at the
-- time and is not any more: the income side was fixed too, by
-- `Rate.RewardQuestMoney = 8` in worldserver.overrides.conf.  Two corrections
-- for one problem stack, leaving gold per level near the retail ratio while
-- mounts still cost a tenth of retail.  The config key is the half worth
-- keeping -- it scales with the whole levelling curve instead of pinning 242
-- rows that have to be regenerated whenever the world DB moves.
--
-- Values come from data/sql/base/db_world/trainer_spell.sql, the dump this
-- fork is built on.  When the cut was reverted every one of them was checked
-- against the price this file used to carry: floor(retail / 10) matched in all
-- 6 cases, so the dump is provably the thing that was divided.
--
-- Kept as explicit UPDATEs rather than deleting the file: the cut is already
-- applied to live databases, and a deleted update never runs.  On a fresh DB
-- these are no-ops, which is the correct outcome.
--
-- Selected by trainer_spell.ReqSkillLine = 762 (Riding), which also covers the
-- druid Swift Flight Form -- that is a riding cost like any other.  Keyed on
-- SpellId, not TrainerId, because each rank sits on every riding trainer.
--
-- npc_trainer is deliberately untouched: it is legacy, and no C++ in src/
-- reads it (the live path is trainer / trainer_spell / creature_default_trainer).
--
-- Player::GetReputationPriceDiscount still takes up to 20% off these at
-- Exalted.  Unchanged and intended, exactly as at retail.
--
-- Regenerate with tools/gen_mount_prices.py.


UPDATE `trainer_spell` SET `MoneyCost` = 40000 WHERE `SpellId` = 33388;   -- Apprentice Riding (lvl 20): 4g
UPDATE `trainer_spell` SET `MoneyCost` = 500000 WHERE `SpellId` = 33391;   -- Journeyman Riding (lvl 40): 50g
UPDATE `trainer_spell` SET `MoneyCost` = 2500000 WHERE `SpellId` = 34090;   -- Expert Riding (lvl 60): 250g
UPDATE `trainer_spell` SET `MoneyCost` = 50000000 WHERE `SpellId` = 34091;   -- Artisan Riding (lvl 70): 5000g
UPDATE `trainer_spell` SET `MoneyCost` = 200000 WHERE `SpellId` = 40120;   -- Swift Flight Form (lvl 71): 20g
UPDATE `trainer_spell` SET `MoneyCost` = 10000000 WHERE `SpellId` = 54197;   -- Cold Weather Flying (lvl 77): 1000g
