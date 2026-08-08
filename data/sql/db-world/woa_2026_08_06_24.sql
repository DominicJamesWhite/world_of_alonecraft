-- ==========================================================================
-- Upgrade tool prices: match a trainer, not an endgame gold sink
-- ==========================================================================
--
-- The tools were priced at 21 * L^2.6, which is 186g for a single level-80
-- reforge.  Upgrading a full set of gear across a levelling run cost more than
-- anything else a character buys, so the system read as something to save up
-- for rather than something to use.
--
-- Refitted to what a class trainer charges for a spell at the same level, taken
-- from the median MoneyCost in trainer_spell.  Same 2.6 exponent, coefficient
-- 1.75 -- a twelvefold reduction:
--
--     level      10      20      30      40      50      60       70       80
--     trainer   500   4,000   8,000  18,000  30,000  46,000  100,000  180,000
--     tool      696   4,223  12,121  25,609  45,746  73,490  109,721  155,264
--
-- tools/gen_upgrade_tools.py carries the same coefficient, so regenerating
-- produces these prices rather than reverting them.
--
-- SellPrice stays at a fifth of BuyPrice, matching the generator.

UPDATE `item_template` SET `BuyPrice` = 100, `SellPrice` = 20 WHERE `entry` = 200100;   -- level 5
UPDATE `item_template` SET `BuyPrice` = 700, `SellPrice` = 140 WHERE `entry` = 200101;   -- level 10
UPDATE `item_template` SET `BuyPrice` = 2000, `SellPrice` = 400 WHERE `entry` = 200102;   -- level 15
UPDATE `item_template` SET `BuyPrice` = 4200, `SellPrice` = 840 WHERE `entry` = 200103;   -- level 20
UPDATE `item_template` SET `BuyPrice` = 7500, `SellPrice` = 1500 WHERE `entry` = 200104;   -- level 25
UPDATE `item_template` SET `BuyPrice` = 12100, `SellPrice` = 2420 WHERE `entry` = 200105;   -- level 30
UPDATE `item_template` SET `BuyPrice` = 18100, `SellPrice` = 3620 WHERE `entry` = 200106;   -- level 35
UPDATE `item_template` SET `BuyPrice` = 25600, `SellPrice` = 5120 WHERE `entry` = 200107;   -- level 40
UPDATE `item_template` SET `BuyPrice` = 34800, `SellPrice` = 6960 WHERE `entry` = 200108;   -- level 45
UPDATE `item_template` SET `BuyPrice` = 45700, `SellPrice` = 9140 WHERE `entry` = 200109;   -- level 50
UPDATE `item_template` SET `BuyPrice` = 58600, `SellPrice` = 11720 WHERE `entry` = 200110;   -- level 55
UPDATE `item_template` SET `BuyPrice` = 73500, `SellPrice` = 14700 WHERE `entry` = 200111;   -- level 60
UPDATE `item_template` SET `BuyPrice` = 90500, `SellPrice` = 18100 WHERE `entry` = 200112;   -- level 65
UPDATE `item_template` SET `BuyPrice` = 109700, `SellPrice` = 21940 WHERE `entry` = 200113;   -- level 70
UPDATE `item_template` SET `BuyPrice` = 131300, `SellPrice` = 26260 WHERE `entry` = 200114;   -- level 75
UPDATE `item_template` SET `BuyPrice` = 155300, `SellPrice` = 31060 WHERE `entry` = 200115;   -- level 80
