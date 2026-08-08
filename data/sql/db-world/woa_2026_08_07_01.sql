-- ==========================================================================
-- Mount item prices: 10x cut to match the 10x XP rate
-- ==========================================================================
--
-- Companion to woa_2026_08_07_00.sql (riding skill).  Same reasoning: gold
-- income is 1x while XP is 10x, so mount prices are cut by the same factor to
-- restore the retail ratio of gold-earned to gold-owed.
--
-- SELLPRICE IS SCALED TOO, AND THAT IS NOT OPTIONAL.  131 of these 236 mounts
-- have a SellPrice above a tenth of their BuyPrice -- Horn of the Timber Wolf
-- buys at 1g and vendors back at 25s.  Cutting BuyPrice alone would let it sell
-- for 2.5x its cost: an infinite gold loop with no cooldown.  Scaling both
-- preserves the original buy/sell ratio exactly.
--
-- Scope: item_template class 15 / subclass 5, BuyPrice > 0.  Rows priced at 0
-- are skipped -- they cannot be bought, so there is no cost to cut and no
-- exploit to create.  The 19 rows here that also sit on an ExtendedCost vendor
-- are no-ops in practice (CreatureData.h::IsGoldRequired ignores BuyPrice when
-- ExtendedCost is set); honor/arena/badge costs live in ItemExtendedCost.dbc
-- and are out of scope, since changing them server-side without a client DBC
-- patch would make the tooltip disagree with what the player is charged.
--
-- Absolute values, not `BuyPrice / 10`: module SQL is re-applied at startup on
-- top of any manual pre-application, and relative arithmetic would divide twice.
-- Regenerate with tools/gen_mount_prices.py, which carries the same divisor.


UPDATE `item_template` SET `BuyPrice` = 1000, `SellPrice` = 250 WHERE `entry` = 1132;   -- Horn of the Timber Wolf: 1g -> 10s
UPDATE `item_template` SET `BuyPrice` = 1000, `SellPrice` = 250 WHERE `entry` = 2411;   -- Black Stallion Bridle: 1g -> 10s
UPDATE `item_template` SET `BuyPrice` = 1000, `SellPrice` = 250 WHERE `entry` = 2414;   -- Pinto Bridle: 1g -> 10s
UPDATE `item_template` SET `BuyPrice` = 1000, `SellPrice` = 250 WHERE `entry` = 5655;   -- Chestnut Mare Bridle: 1g -> 10s
UPDATE `item_template` SET `BuyPrice` = 1000, `SellPrice` = 250 WHERE `entry` = 5656;   -- Brown Horse Bridle: 1g -> 10s
UPDATE `item_template` SET `BuyPrice` = 1000, `SellPrice` = 250 WHERE `entry` = 5665;   -- Horn of the Dire Wolf: 1g -> 10s
UPDATE `item_template` SET `BuyPrice` = 1000, `SellPrice` = 250 WHERE `entry` = 5668;   -- Horn of the Brown Wolf: 1g -> 10s
UPDATE `item_template` SET `BuyPrice` = 1000, `SellPrice` = 250 WHERE `entry` = 5864;   -- Gray Ram: 1g -> 10s
UPDATE `item_template` SET `BuyPrice` = 1000, `SellPrice` = 250 WHERE `entry` = 5872;   -- Brown Ram: 1g -> 10s
UPDATE `item_template` SET `BuyPrice` = 1000, `SellPrice` = 250 WHERE `entry` = 5873;   -- White Ram: 1g -> 10s
UPDATE `item_template` SET `BuyPrice` = 1000, `SellPrice` = 250 WHERE `entry` = 8563;   -- Red Mechanostrider: 1g -> 10s
UPDATE `item_template` SET `BuyPrice` = 10000, `SellPrice` = 2500 WHERE `entry` = 8586;   -- Whistle of the Mottled Red Raptor: 10g -> 1g
UPDATE `item_template` SET `BuyPrice` = 1000, `SellPrice` = 250 WHERE `entry` = 8588;   -- Whistle of the Emerald Raptor: 1g -> 10s
UPDATE `item_template` SET `BuyPrice` = 1000, `SellPrice` = 250 WHERE `entry` = 8591;   -- Whistle of the Turquoise Raptor: 1g -> 10s
UPDATE `item_template` SET `BuyPrice` = 1000, `SellPrice` = 250 WHERE `entry` = 8592;   -- Whistle of the Violet Raptor: 1g -> 10s
UPDATE `item_template` SET `BuyPrice` = 1000, `SellPrice` = 250 WHERE `entry` = 8595;   -- Blue Mechanostrider: 1g -> 10s
UPDATE `item_template` SET `BuyPrice` = 1000, `SellPrice` = 250 WHERE `entry` = 8629;   -- Reins of the Striped Nightsaber: 1g -> 10s
UPDATE `item_template` SET `BuyPrice` = 1000, `SellPrice` = 250 WHERE `entry` = 8631;   -- Reins of the Striped Frostsaber: 1g -> 10s
UPDATE `item_template` SET `BuyPrice` = 1000, `SellPrice` = 250 WHERE `entry` = 8632;   -- Reins of the Spotted Frostsaber: 1g -> 10s
UPDATE `item_template` SET `BuyPrice` = 10000, `SellPrice` = 2500 WHERE `entry` = 12302;   -- Reins of the Ancient Frostsaber: 10g -> 1g
UPDATE `item_template` SET `BuyPrice` = 10000, `SellPrice` = 2500 WHERE `entry` = 12303;   -- Reins of the Nightsaber: 10g -> 1g
UPDATE `item_template` SET `BuyPrice` = 1000, `SellPrice` = 250 WHERE `entry` = 12325;   -- Reins of the Primal Leopard: 1g -> 10s
UPDATE `item_template` SET `BuyPrice` = 1000, `SellPrice` = 250 WHERE `entry` = 12326;   -- Reins of the Tawny Sabercat: 1g -> 10s
UPDATE `item_template` SET `BuyPrice` = 1000, `SellPrice` = 250 WHERE `entry` = 12327;   -- Reins of the Golden Sabercat: 1g -> 10s
UPDATE `item_template` SET `BuyPrice` = 10000, `SellPrice` = 2500 WHERE `entry` = 12330;   -- Horn of the Red Wolf: 10g -> 1g
UPDATE `item_template` SET `BuyPrice` = 10000, `SellPrice` = 2500 WHERE `entry` = 12351;   -- Horn of the Arctic Wolf: 10g -> 1g
UPDATE `item_template` SET `BuyPrice` = 10000, `SellPrice` = 2500 WHERE `entry` = 12353;   -- White Stallion Bridle: 10g -> 1g
UPDATE `item_template` SET `BuyPrice` = 10000, `SellPrice` = 2500 WHERE `entry` = 12354;   -- Palomino Bridle: 10g -> 1g
UPDATE `item_template` SET `BuyPrice` = 100000, `SellPrice` = 25000 WHERE `entry` = 13086;   -- Reins of the Winterspring Frostsaber: 100g -> 10g
UPDATE `item_template` SET `BuyPrice` = 10000, `SellPrice` = 2500 WHERE `entry` = 13317;   -- Whistle of the Ivory Raptor: 10g -> 1g
UPDATE `item_template` SET `BuyPrice` = 1000, `SellPrice` = 250 WHERE `entry` = 13321;   -- Green Mechanostrider: 1g -> 10s
UPDATE `item_template` SET `BuyPrice` = 1000, `SellPrice` = 250 WHERE `entry` = 13322;   -- Unpainted Mechanostrider: 1g -> 10s
UPDATE `item_template` SET `BuyPrice` = 1000, `SellPrice` = 250 WHERE `entry` = 13323;   -- Purple Mechanostrider: 1g -> 10s
UPDATE `item_template` SET `BuyPrice` = 1000, `SellPrice` = 250 WHERE `entry` = 13324;   -- Red and Blue Mechanostrider: 1g -> 10s
UPDATE `item_template` SET `BuyPrice` = 10000, `SellPrice` = 2500 WHERE `entry` = 13326;   -- White Mechanostrider Mod B: 10g -> 1g
UPDATE `item_template` SET `BuyPrice` = 10000, `SellPrice` = 2500 WHERE `entry` = 13327;   -- Icy Blue Mechanostrider Mod A: 10g -> 1g
UPDATE `item_template` SET `BuyPrice` = 10000, `SellPrice` = 2500 WHERE `entry` = 13328;   -- Black Ram: 10g -> 1g
UPDATE `item_template` SET `BuyPrice` = 10000, `SellPrice` = 2500 WHERE `entry` = 13329;   -- Frost Ram: 10g -> 1g
UPDATE `item_template` SET `BuyPrice` = 1000, `SellPrice` = 250 WHERE `entry` = 13331;   -- Red Skeletal Horse: 1g -> 10s
UPDATE `item_template` SET `BuyPrice` = 1000, `SellPrice` = 250 WHERE `entry` = 13332;   -- Blue Skeletal Horse: 1g -> 10s
UPDATE `item_template` SET `BuyPrice` = 1000, `SellPrice` = 250 WHERE `entry` = 13333;   -- Brown Skeletal Horse: 1g -> 10s
UPDATE `item_template` SET `BuyPrice` = 10000, `SellPrice` = 2500 WHERE `entry` = 13334;   -- Green Skeletal Warhorse: 10g -> 1g
UPDATE `item_template` SET `BuyPrice` = 1000, `SellPrice` = 250 WHERE `entry` = 15277;   -- Gray Kodo: 1g -> 10s
UPDATE `item_template` SET `BuyPrice` = 1000, `SellPrice` = 250 WHERE `entry` = 15290;   -- Brown Kodo: 1g -> 10s
UPDATE `item_template` SET `BuyPrice` = 10000, `SellPrice` = 2500 WHERE `entry` = 15292;   -- Green Kodo: 10g -> 1g
UPDATE `item_template` SET `BuyPrice` = 10000, `SellPrice` = 2500 WHERE `entry` = 15293;   -- Teal Kodo: 10g -> 1g
UPDATE `item_template` SET `BuyPrice` = 10000, `SellPrice` = 2500 WHERE `entry` = 18766;   -- Reins of the Swift Frostsaber: 10g -> 1g
UPDATE `item_template` SET `BuyPrice` = 10000, `SellPrice` = 2500 WHERE `entry` = 18767;   -- Reins of the Swift Mistsaber: 10g -> 1g
UPDATE `item_template` SET `BuyPrice` = 10000, `SellPrice` = 2500 WHERE `entry` = 18768;   -- Reins of the Swift Dawnsaber: 10g -> 1g
UPDATE `item_template` SET `BuyPrice` = 10000, `SellPrice` = 2500 WHERE `entry` = 18772;   -- Swift Green Mechanostrider: 10g -> 1g
UPDATE `item_template` SET `BuyPrice` = 10000, `SellPrice` = 2500 WHERE `entry` = 18773;   -- Swift White Mechanostrider: 10g -> 1g
UPDATE `item_template` SET `BuyPrice` = 10000, `SellPrice` = 2500 WHERE `entry` = 18774;   -- Swift Yellow Mechanostrider: 10g -> 1g
UPDATE `item_template` SET `BuyPrice` = 10000, `SellPrice` = 2500 WHERE `entry` = 18776;   -- Swift Palomino: 10g -> 1g
UPDATE `item_template` SET `BuyPrice` = 10000, `SellPrice` = 2500 WHERE `entry` = 18777;   -- Swift Brown Steed: 10g -> 1g
UPDATE `item_template` SET `BuyPrice` = 10000, `SellPrice` = 2500 WHERE `entry` = 18778;   -- Swift White Steed: 10g -> 1g
UPDATE `item_template` SET `BuyPrice` = 10000, `SellPrice` = 2500 WHERE `entry` = 18785;   -- Swift White Ram: 10g -> 1g
UPDATE `item_template` SET `BuyPrice` = 10000, `SellPrice` = 2500 WHERE `entry` = 18786;   -- Swift Brown Ram: 10g -> 1g
UPDATE `item_template` SET `BuyPrice` = 10000, `SellPrice` = 2500 WHERE `entry` = 18787;   -- Swift Gray Ram: 10g -> 1g
UPDATE `item_template` SET `BuyPrice` = 10000, `SellPrice` = 2500 WHERE `entry` = 18788;   -- Swift Blue Raptor: 10g -> 1g
UPDATE `item_template` SET `BuyPrice` = 10000, `SellPrice` = 2500 WHERE `entry` = 18789;   -- Swift Olive Raptor: 10g -> 1g
UPDATE `item_template` SET `BuyPrice` = 10000, `SellPrice` = 2500 WHERE `entry` = 18790;   -- Swift Orange Raptor: 10g -> 1g
UPDATE `item_template` SET `BuyPrice` = 10000, `SellPrice` = 2500 WHERE `entry` = 18791;   -- Purple Skeletal Warhorse: 10g -> 1g
UPDATE `item_template` SET `BuyPrice` = 10000, `SellPrice` = 2500 WHERE `entry` = 18793;   -- Great White Kodo: 10g -> 1g
UPDATE `item_template` SET `BuyPrice` = 10000, `SellPrice` = 2500 WHERE `entry` = 18794;   -- Great Brown Kodo: 10g -> 1g
UPDATE `item_template` SET `BuyPrice` = 10000, `SellPrice` = 2500 WHERE `entry` = 18795;   -- Great Gray Kodo: 10g -> 1g
UPDATE `item_template` SET `BuyPrice` = 10000, `SellPrice` = 2500 WHERE `entry` = 18796;   -- Horn of the Swift Brown Wolf: 10g -> 1g
UPDATE `item_template` SET `BuyPrice` = 10000, `SellPrice` = 2500 WHERE `entry` = 18797;   -- Horn of the Swift Timber Wolf: 10g -> 1g
UPDATE `item_template` SET `BuyPrice` = 10000, `SellPrice` = 2500 WHERE `entry` = 18798;   -- Horn of the Swift Gray Wolf: 10g -> 1g
UPDATE `item_template` SET `BuyPrice` = 10000, `SellPrice` = 2500 WHERE `entry` = 18902;   -- Reins of the Swift Stormsaber: 10g -> 1g
UPDATE `item_template` SET `BuyPrice` = 100000, `SellPrice` = 0 WHERE `entry` = 21176;   -- Black Qiraji Resonating Crystal: 100g -> 10g
UPDATE `item_template` SET `BuyPrice` = 100000, `SellPrice` = 0 WHERE `entry` = 21218;   -- Blue Qiraji Resonating Crystal: 100g -> 10g
UPDATE `item_template` SET `BuyPrice` = 100000, `SellPrice` = 0 WHERE `entry` = 21321;   -- Red Qiraji Resonating Crystal: 100g -> 10g
UPDATE `item_template` SET `BuyPrice` = 100000, `SellPrice` = 0 WHERE `entry` = 21323;   -- Green Qiraji Resonating Crystal: 100g -> 10g
UPDATE `item_template` SET `BuyPrice` = 100000, `SellPrice` = 0 WHERE `entry` = 21324;   -- Yellow Qiraji Resonating Crystal: 100g -> 10g
UPDATE `item_template` SET `BuyPrice` = 50000, `SellPrice` = 12500 WHERE `entry` = 25470;   -- Golden Gryphon: 50g -> 5g
UPDATE `item_template` SET `BuyPrice` = 50000, `SellPrice` = 12500 WHERE `entry` = 25471;   -- Ebon Gryphon: 50g -> 5g
UPDATE `item_template` SET `BuyPrice` = 50000, `SellPrice` = 12500 WHERE `entry` = 25472;   -- Snowy Gryphon: 50g -> 5g
UPDATE `item_template` SET `BuyPrice` = 100000, `SellPrice` = 25000 WHERE `entry` = 25473;   -- Swift Blue Gryphon: 100g -> 10g
UPDATE `item_template` SET `BuyPrice` = 50000, `SellPrice` = 12500 WHERE `entry` = 25474;   -- Tawny Wind Rider: 50g -> 5g
UPDATE `item_template` SET `BuyPrice` = 50000, `SellPrice` = 12500 WHERE `entry` = 25475;   -- Blue Wind Rider: 50g -> 5g
UPDATE `item_template` SET `BuyPrice` = 50000, `SellPrice` = 12500 WHERE `entry` = 25476;   -- Green Wind Rider: 50g -> 5g
UPDATE `item_template` SET `BuyPrice` = 100000, `SellPrice` = 25000 WHERE `entry` = 25477;   -- Swift Red Wind Rider: 100g -> 10g
UPDATE `item_template` SET `BuyPrice` = 100000, `SellPrice` = 25000 WHERE `entry` = 25527;   -- Swift Red Gryphon: 100g -> 10g
UPDATE `item_template` SET `BuyPrice` = 100000, `SellPrice` = 25000 WHERE `entry` = 25528;   -- Swift Green Gryphon: 100g -> 10g
UPDATE `item_template` SET `BuyPrice` = 100000, `SellPrice` = 25000 WHERE `entry` = 25529;   -- Swift Purple Gryphon: 100g -> 10g
UPDATE `item_template` SET `BuyPrice` = 100000, `SellPrice` = 25000 WHERE `entry` = 25531;   -- Swift Green Wind Rider: 100g -> 10g
UPDATE `item_template` SET `BuyPrice` = 100000, `SellPrice` = 25000 WHERE `entry` = 25532;   -- Swift Yellow Wind Rider: 100g -> 10g
UPDATE `item_template` SET `BuyPrice` = 100000, `SellPrice` = 25000 WHERE `entry` = 25533;   -- Swift Purple Wind Rider: 100g -> 10g
UPDATE `item_template` SET `BuyPrice` = 1000, `SellPrice` = 250 WHERE `entry` = 28481;   -- Brown Elekk: 1g -> 10s
UPDATE `item_template` SET `BuyPrice` = 10000, `SellPrice` = 2500 WHERE `entry` = 28482;   -- Great Elite Elekk: 10g -> 1g
UPDATE `item_template` SET `BuyPrice` = 1000, `SellPrice` = 250 WHERE `entry` = 28927;   -- Red Hawkstrider: 1g -> 10s
UPDATE `item_template` SET `BuyPrice` = 10000, `SellPrice` = 2500 WHERE `entry` = 28936;   -- Swift Pink Hawkstrider: 10g -> 1g
UPDATE `item_template` SET `BuyPrice` = 100000, `SellPrice` = 0 WHERE `entry` = 29102;   -- Reins of the Cobalt War Talbuk: 100g -> 10g
UPDATE `item_template` SET `BuyPrice` = 100000, `SellPrice` = 0 WHERE `entry` = 29103;   -- Reins of the White War Talbuk: 100g -> 10g
UPDATE `item_template` SET `BuyPrice` = 100000, `SellPrice` = 0 WHERE `entry` = 29104;   -- Reins of the Silver War Talbuk: 100g -> 10g
UPDATE `item_template` SET `BuyPrice` = 100000, `SellPrice` = 0 WHERE `entry` = 29105;   -- Reins of the Tan War Talbuk: 100g -> 10g
UPDATE `item_template` SET `BuyPrice` = 1000, `SellPrice` = 250 WHERE `entry` = 29220;   -- Blue Hawkstrider: 1g -> 10s
UPDATE `item_template` SET `BuyPrice` = 1000, `SellPrice` = 250 WHERE `entry` = 29221;   -- Black Hawkstrider: 1g -> 10s
UPDATE `item_template` SET `BuyPrice` = 1000, `SellPrice` = 250 WHERE `entry` = 29222;   -- Purple Hawkstrider: 1g -> 10s
UPDATE `item_template` SET `BuyPrice` = 10000, `SellPrice` = 2500 WHERE `entry` = 29223;   -- Swift Green Hawkstrider: 10g -> 1g
UPDATE `item_template` SET `BuyPrice` = 10000, `SellPrice` = 2500 WHERE `entry` = 29224;   -- Swift Purple Hawkstrider: 10g -> 1g
UPDATE `item_template` SET `BuyPrice` = 100000, `SellPrice` = 0 WHERE `entry` = 29227;   -- Reins of the Cobalt War Talbuk: 100g -> 10g
UPDATE `item_template` SET `BuyPrice` = 100000, `SellPrice` = 0 WHERE `entry` = 29229;   -- Reins of the Silver War Talbuk: 100g -> 10g
UPDATE `item_template` SET `BuyPrice` = 100000, `SellPrice` = 0 WHERE `entry` = 29230;   -- Reins of the Tan War Talbuk: 100g -> 10g
UPDATE `item_template` SET `BuyPrice` = 100000, `SellPrice` = 0 WHERE `entry` = 29231;   -- Reins of the White War Talbuk: 100g -> 10g
UPDATE `item_template` SET `BuyPrice` = 1000, `SellPrice` = 250 WHERE `entry` = 29743;   -- Purple Elekk: 1g -> 10s
UPDATE `item_template` SET `BuyPrice` = 1000, `SellPrice` = 250 WHERE `entry` = 29744;   -- Gray Elekk: 1g -> 10s
UPDATE `item_template` SET `BuyPrice` = 10000, `SellPrice` = 2500 WHERE `entry` = 29745;   -- Great Blue Elekk: 10g -> 1g
UPDATE `item_template` SET `BuyPrice` = 10000, `SellPrice` = 2500 WHERE `entry` = 29746;   -- Great Green Elekk: 10g -> 1g
UPDATE `item_template` SET `BuyPrice` = 10000, `SellPrice` = 2500 WHERE `entry` = 29747;   -- Great Purple Elekk: 10g -> 1g
UPDATE `item_template` SET `BuyPrice` = 200000, `SellPrice` = 0 WHERE `entry` = 30609;   -- Swift Nether Drake: 200g -> 20g
UPDATE `item_template` SET `BuyPrice` = 70000, `SellPrice` = 0 WHERE `entry` = 31829;   -- Reins of the Cobalt Riding Talbuk: 70g -> 7g
UPDATE `item_template` SET `BuyPrice` = 70000, `SellPrice` = 0 WHERE `entry` = 31830;   -- Reins of the Cobalt Riding Talbuk: 70g -> 7g
UPDATE `item_template` SET `BuyPrice` = 70000, `SellPrice` = 0 WHERE `entry` = 31831;   -- Reins of the Silver Riding Talbuk: 70g -> 7g
UPDATE `item_template` SET `BuyPrice` = 70000, `SellPrice` = 0 WHERE `entry` = 31832;   -- Reins of the Silver Riding Talbuk: 70g -> 7g
UPDATE `item_template` SET `BuyPrice` = 70000, `SellPrice` = 0 WHERE `entry` = 31833;   -- Reins of the Tan Riding Talbuk: 70g -> 7g
UPDATE `item_template` SET `BuyPrice` = 70000, `SellPrice` = 0 WHERE `entry` = 31834;   -- Reins of the Tan Riding Talbuk: 70g -> 7g
UPDATE `item_template` SET `BuyPrice` = 70000, `SellPrice` = 0 WHERE `entry` = 31835;   -- Reins of the White Riding Talbuk: 70g -> 7g
UPDATE `item_template` SET `BuyPrice` = 70000, `SellPrice` = 0 WHERE `entry` = 31836;   -- Reins of the White Riding Talbuk: 70g -> 7g
UPDATE `item_template` SET `BuyPrice` = 200000, `SellPrice` = 0 WHERE `entry` = 32314;   -- Green Riding Nether Ray: 200g -> 20g
UPDATE `item_template` SET `BuyPrice` = 200000, `SellPrice` = 0 WHERE `entry` = 32316;   -- Purple Riding Nether Ray: 200g -> 20g
UPDATE `item_template` SET `BuyPrice` = 200000, `SellPrice` = 0 WHERE `entry` = 32317;   -- Red Riding Nether Ray: 200g -> 20g
UPDATE `item_template` SET `BuyPrice` = 200000, `SellPrice` = 0 WHERE `entry` = 32318;   -- Silver Riding Nether Ray: 200g -> 20g
UPDATE `item_template` SET `BuyPrice` = 200000, `SellPrice` = 0 WHERE `entry` = 32319;   -- Blue Riding Nether Ray: 200g -> 20g
UPDATE `item_template` SET `BuyPrice` = 100000, `SellPrice` = 0 WHERE `entry` = 32458;   -- Ashes of Al'ar: 100g -> 10g
UPDATE `item_template` SET `BuyPrice` = 200000, `SellPrice` = 0 WHERE `entry` = 32857;   -- Reins of the Onyx Netherwing Drake: 200g -> 20g
UPDATE `item_template` SET `BuyPrice` = 200000, `SellPrice` = 0 WHERE `entry` = 32858;   -- Reins of the Azure Netherwing Drake: 200g -> 20g
UPDATE `item_template` SET `BuyPrice` = 200000, `SellPrice` = 0 WHERE `entry` = 32859;   -- Reins of the Cobalt Netherwing Drake: 200g -> 20g
UPDATE `item_template` SET `BuyPrice` = 200000, `SellPrice` = 0 WHERE `entry` = 32860;   -- Reins of the Purple Netherwing Drake: 200g -> 20g
UPDATE `item_template` SET `BuyPrice` = 200000, `SellPrice` = 0 WHERE `entry` = 32861;   -- Reins of the Veridian Netherwing Drake: 200g -> 20g
UPDATE `item_template` SET `BuyPrice` = 200000, `SellPrice` = 0 WHERE `entry` = 32862;   -- Reins of the Violet Netherwing Drake: 200g -> 20g
UPDATE `item_template` SET `BuyPrice` = 1000, `SellPrice` = 0 WHERE `entry` = 33224;   -- Reins of the Spectral Tiger: 1g -> 10s
UPDATE `item_template` SET `BuyPrice` = 10000, `SellPrice` = 0 WHERE `entry` = 33225;   -- Reins of the Swift Spectral Tiger: 10g -> 1g
UPDATE `item_template` SET `BuyPrice` = 10000, `SellPrice` = 0 WHERE `entry` = 33976;   -- Brewfest Ram: 10g -> 1g
UPDATE `item_template` SET `BuyPrice` = 100000, `SellPrice` = 0 WHERE `entry` = 33977;   -- Swift Brewfest Ram: 100g -> 10g
UPDATE `item_template` SET `BuyPrice` = 2000000, `SellPrice` = 0 WHERE `entry` = 33999;   -- Cenarion War Hippogryph: 2000g -> 200g
UPDATE `item_template` SET `BuyPrice` = 200000, `SellPrice` = 0 WHERE `entry` = 34092;   -- Merciless Nether Drake: 200g -> 20g
UPDATE `item_template` SET `BuyPrice` = 100000, `SellPrice` = 0 WHERE `entry` = 35225;   -- X-51 Nether-Rocket: 100g -> 10g
UPDATE `item_template` SET `BuyPrice` = 200000, `SellPrice` = 0 WHERE `entry` = 35226;   -- X-51 Nether-Rocket X-TREME: 200g -> 20g
UPDATE `item_template` SET `BuyPrice` = 100000, `SellPrice` = 0 WHERE `entry` = 35513;   -- Swift White Hawkstrider: 100g -> 10g
UPDATE `item_template` SET `BuyPrice` = 10000, `SellPrice` = 0 WHERE `entry` = 37598;   -- Swift Zhevra OLD: 10g -> 1g
UPDATE `item_template` SET `BuyPrice` = 200000, `SellPrice` = 0 WHERE `entry` = 37676;   -- Vengeful Nether Drake: 200g -> 20g
UPDATE `item_template` SET `BuyPrice` = 10000, `SellPrice` = 0 WHERE `entry` = 37719;   -- Swift Zhevra: 10g -> 1g
UPDATE `item_template` SET `BuyPrice` = 10000, `SellPrice` = 0 WHERE `entry` = 37827;   -- Brewfest Kodo: 10g -> 1g
UPDATE `item_template` SET `BuyPrice` = 100000, `SellPrice` = 0 WHERE `entry` = 37828;   -- Great Brewfest Kodo: 100g -> 10g
UPDATE `item_template` SET `BuyPrice` = 10000, `SellPrice` = 0 WHERE `entry` = 38576;   -- Big Battle Bear: 10g -> 1g
UPDATE `item_template` SET `BuyPrice` = 1000000, `SellPrice` = 0 WHERE `entry` = 40775;   -- Winged Steed of the Ebon Blade: 1000g -> 100g
UPDATE `item_template` SET `BuyPrice` = 100000, `SellPrice` = 0 WHERE `entry` = 40777;   -- Polar Bear Harness: 100g -> 10g
UPDATE `item_template` SET `BuyPrice` = 5000, `SellPrice` = 1250 WHERE `entry` = 41508;   -- Mechano-hog: 5g -> 50s
UPDATE `item_template` SET `BuyPrice` = 200000, `SellPrice` = 0 WHERE `entry` = 43516;   -- Brutal Nether Drake: 200g -> 20g
UPDATE `item_template` SET `BuyPrice` = 200000, `SellPrice` = 0 WHERE `entry` = 43951;   -- Reins of the Bronze Drake: 200g -> 20g
UPDATE `item_template` SET `BuyPrice` = 200000, `SellPrice` = 0 WHERE `entry` = 43952;   -- Reins of the Azure Drake: 200g -> 20g
UPDATE `item_template` SET `BuyPrice` = 200000, `SellPrice` = 0 WHERE `entry` = 43953;   -- Reins of the Blue Drake: 200g -> 20g
UPDATE `item_template` SET `BuyPrice` = 200000, `SellPrice` = 0 WHERE `entry` = 43954;   -- Reins of the Twilight Drake: 200g -> 20g
UPDATE `item_template` SET `BuyPrice` = 2000000, `SellPrice` = 0 WHERE `entry` = 43955;   -- Reins of the Red Drake: 2000g -> 200g
UPDATE `item_template` SET `BuyPrice` = 1000000, `SellPrice` = 250000 WHERE `entry` = 43958;   -- Reins of the Ice Mammoth: 1000g -> 100g
UPDATE `item_template` SET `BuyPrice` = 100000, `SellPrice` = 0 WHERE `entry` = 43959;   -- Reins of the Grand Black War Mammoth: 100g -> 10g
UPDATE `item_template` SET `BuyPrice` = 10000000, `SellPrice` = 2500000 WHERE `entry` = 43961;   -- Reins of the Grand Ice Mammoth: 10000g -> 1000g
UPDATE `item_template` SET `BuyPrice` = 10000, `SellPrice` = 0 WHERE `entry` = 43962;   -- Reins of the White Polar Bear: 10g -> 1g
UPDATE `item_template` SET `BuyPrice` = 100000, `SellPrice` = 0 WHERE `entry` = 43963;   -- Reins of the Brown Polar Bear: 100g -> 10g
UPDATE `item_template` SET `BuyPrice` = 100000, `SellPrice` = 0 WHERE `entry` = 43964;   -- Reins of the Black Polar Bear: 100g -> 10g
UPDATE `item_template` SET `BuyPrice` = 200000, `SellPrice` = 0 WHERE `entry` = 43986;   -- Reins of the Black Drake: 200g -> 20g
UPDATE `item_template` SET `BuyPrice` = 1000000, `SellPrice` = 250000 WHERE `entry` = 44080;   -- Reins of the Ice Mammoth: 1000g -> 100g
UPDATE `item_template` SET `BuyPrice` = 100000, `SellPrice` = 0 WHERE `entry` = 44083;   -- Reins of the Grand Black War Mammoth: 100g -> 10g
UPDATE `item_template` SET `BuyPrice` = 10000000, `SellPrice` = 2500000 WHERE `entry` = 44086;   -- Reins of the Grand Ice Mammoth: 10000g -> 1000g
UPDATE `item_template` SET `BuyPrice` = 200000, `SellPrice` = 0 WHERE `entry` = 44151;   -- Reins of the Blue Proto-Drake: 200g -> 20g
UPDATE `item_template` SET `BuyPrice` = 200000, `SellPrice` = 0 WHERE `entry` = 44160;   -- Reins of the Red Proto-Drake: 200g -> 20g
UPDATE `item_template` SET `BuyPrice` = 200000, `SellPrice` = 0 WHERE `entry` = 44164;   -- Reins of the Black Proto-Drake: 200g -> 20g
UPDATE `item_template` SET `BuyPrice` = 200000, `SellPrice` = 0 WHERE `entry` = 44168;   -- Reins of the Time-Lost Proto-Drake: 200g -> 20g
UPDATE `item_template` SET `BuyPrice` = 200000, `SellPrice` = 0 WHERE `entry` = 44175;   -- Reins of the Plagued Proto-Drake: 200g -> 20g
UPDATE `item_template` SET `BuyPrice` = 200000, `SellPrice` = 0 WHERE `entry` = 44177;   -- Reins of the Violet Proto-Drake: 200g -> 20g
UPDATE `item_template` SET `BuyPrice` = 200000, `SellPrice` = 0 WHERE `entry` = 44178;   -- Reins of the Albino Drake: 200g -> 20g
UPDATE `item_template` SET `BuyPrice` = 100000, `SellPrice` = 0 WHERE `entry` = 44221;   -- Loaned Gryphon Reins: 100g -> 10g
UPDATE `item_template` SET `BuyPrice` = 10000, `SellPrice` = 2500 WHERE `entry` = 44223;   -- Reins of the Black War Bear: 10g -> 1g
UPDATE `item_template` SET `BuyPrice` = 10000, `SellPrice` = 0 WHERE `entry` = 44224;   -- Reins of the Black War Bear: 10g -> 1g
UPDATE `item_template` SET `BuyPrice` = 750000, `SellPrice` = 0 WHERE `entry` = 44225;   -- Reins of the Armored Brown Bear: 750g -> 75g
UPDATE `item_template` SET `BuyPrice` = 750000, `SellPrice` = 0 WHERE `entry` = 44226;   -- Reins of the Armored Brown Bear: 750g -> 75g
UPDATE `item_template` SET `BuyPrice` = 100000, `SellPrice` = 0 WHERE `entry` = 44229;   -- Loaned Wind Rider Reins: 100g -> 10g
UPDATE `item_template` SET `BuyPrice` = 20000000, `SellPrice` = 0 WHERE `entry` = 44234;   -- Reins of the Traveler's Tundra Mammoth: 20000g -> 2000g
UPDATE `item_template` SET `BuyPrice` = 20000000, `SellPrice` = 0 WHERE `entry` = 44235;   -- Reins of the Traveler's Tundra Mammoth: 20000g -> 2000g
UPDATE `item_template` SET `BuyPrice` = 5000, `SellPrice` = 0 WHERE `entry` = 44413;   -- Mekgineer's Chopper: 5g -> 50s
UPDATE `item_template` SET `BuyPrice` = 200000, `SellPrice` = 0 WHERE `entry` = 44558;   -- Magnificent Flying Carpet: 200g -> 20g
UPDATE `item_template` SET `BuyPrice` = 2000000, `SellPrice` = 0 WHERE `entry` = 44689;   -- Armored Snowy Gryphon: 2000g -> 200g
UPDATE `item_template` SET `BuyPrice` = 2000000, `SellPrice` = 0 WHERE `entry` = 44690;   -- Armored Blue Wind Rider: 2000g -> 200g
UPDATE `item_template` SET `BuyPrice` = 200000, `SellPrice` = 0 WHERE `entry` = 44707;   -- Reins of the Green Proto-Drake: 200g -> 20g
UPDATE `item_template` SET `BuyPrice` = 2000000, `SellPrice` = 0 WHERE `entry` = 44842;   -- Red Dragonhawk Mount: 2000g -> 200g
UPDATE `item_template` SET `BuyPrice` = 2000000, `SellPrice` = 0 WHERE `entry` = 44843;   -- Blue Dragonhawk Mount: 2000g -> 200g
UPDATE `item_template` SET `BuyPrice` = 200000, `SellPrice` = 0 WHERE `entry` = 45693;   -- Mimiron's Head: 200g -> 20g
UPDATE `item_template` SET `BuyPrice` = 200000, `SellPrice` = 0 WHERE `entry` = 45801;   -- Reins of the Ironbound Proto-Drake: 200g -> 20g
UPDATE `item_template` SET `BuyPrice` = 200000, `SellPrice` = 0 WHERE `entry` = 45802;   -- Reins of the Rusted Proto-Drake: 200g -> 20g
UPDATE `item_template` SET `BuyPrice` = 1000, `SellPrice` = 250 WHERE `entry` = 46099;   -- Horn of the Black Wolf: 1g -> 10s
UPDATE `item_template` SET `BuyPrice` = 1000, `SellPrice` = 250 WHERE `entry` = 46100;   -- White Kodo: 1g -> 10s
UPDATE `item_template` SET `BuyPrice` = 10000, `SellPrice` = 2500 WHERE `entry` = 46102;   -- Whistle of the Venomhide Ravasaur: 10g -> 1g
UPDATE `item_template` SET `BuyPrice` = 200000, `SellPrice` = 0 WHERE `entry` = 46171;   -- Furious Gladiator's Frost Wyrm: 200g -> 20g
UPDATE `item_template` SET `BuyPrice` = 1000, `SellPrice` = 250 WHERE `entry` = 46308;   -- Black Skeletal Horse: 1g -> 10s
UPDATE `item_template` SET `BuyPrice` = 200000, `SellPrice` = 0 WHERE `entry` = 46708;   -- Deadly Gladiator's Frost Wyrm: 200g -> 20g
UPDATE `item_template` SET `BuyPrice` = 500000, `SellPrice` = 125000 WHERE `entry` = 46743;   -- Swift Purple Raptor: 500g -> 50g
UPDATE `item_template` SET `BuyPrice` = 500000, `SellPrice` = 125000 WHERE `entry` = 46744;   -- Swift Moonsaber: 500g -> 50g
UPDATE `item_template` SET `BuyPrice` = 500000, `SellPrice` = 125000 WHERE `entry` = 46745;   -- Great Red Elekk: 500g -> 50g
UPDATE `item_template` SET `BuyPrice` = 500000, `SellPrice` = 125000 WHERE `entry` = 46746;   -- White Skeletal Warhorse: 500g -> 50g
UPDATE `item_template` SET `BuyPrice` = 500000, `SellPrice` = 125000 WHERE `entry` = 46747;   -- Turbostrider: 500g -> 50g
UPDATE `item_template` SET `BuyPrice` = 500000, `SellPrice` = 125000 WHERE `entry` = 46748;   -- Swift Violet Ram: 500g -> 50g
UPDATE `item_template` SET `BuyPrice` = 500000, `SellPrice` = 125000 WHERE `entry` = 46749;   -- Swift Burgundy Wolf: 500g -> 50g
UPDATE `item_template` SET `BuyPrice` = 500000, `SellPrice` = 125000 WHERE `entry` = 46750;   -- Great Golden Kodo: 500g -> 50g
UPDATE `item_template` SET `BuyPrice` = 500000, `SellPrice` = 125000 WHERE `entry` = 46751;   -- Swift Red Hawkstrider: 500g -> 50g
UPDATE `item_template` SET `BuyPrice` = 500000, `SellPrice` = 125000 WHERE `entry` = 46752;   -- Swift Gray Steed: 500g -> 50g
UPDATE `item_template` SET `BuyPrice` = 500000, `SellPrice` = 125000 WHERE `entry` = 46755;   -- Great Golden Kodo: 500g -> 50g
UPDATE `item_template` SET `BuyPrice` = 500000, `SellPrice` = 125000 WHERE `entry` = 46756;   -- Great Red Elekk: 500g -> 50g
UPDATE `item_template` SET `BuyPrice` = 500000, `SellPrice` = 125000 WHERE `entry` = 46757;   -- Swift Burgundy Wolf: 500g -> 50g
UPDATE `item_template` SET `BuyPrice` = 500000, `SellPrice` = 125000 WHERE `entry` = 46758;   -- Swift Gray Steed: 500g -> 50g
UPDATE `item_template` SET `BuyPrice` = 500000, `SellPrice` = 125000 WHERE `entry` = 46759;   -- Swift Moonsaber: 500g -> 50g
UPDATE `item_template` SET `BuyPrice` = 500000, `SellPrice` = 125000 WHERE `entry` = 46760;   -- Swift Purple Raptor: 500g -> 50g
UPDATE `item_template` SET `BuyPrice` = 500000, `SellPrice` = 125000 WHERE `entry` = 46761;   -- Swift Red Hawkstrider: 500g -> 50g
UPDATE `item_template` SET `BuyPrice` = 500000, `SellPrice` = 125000 WHERE `entry` = 46762;   -- Swift Violet Ram: 500g -> 50g
UPDATE `item_template` SET `BuyPrice` = 500000, `SellPrice` = 125000 WHERE `entry` = 46763;   -- Turbostrider: 500g -> 50g
UPDATE `item_template` SET `BuyPrice` = 500000, `SellPrice` = 125000 WHERE `entry` = 46764;   -- White Skeletal Warhorse: 500g -> 50g
UPDATE `item_template` SET `BuyPrice` = 100000, `SellPrice` = 0 WHERE `entry` = 46778;   -- Magic Rooster Egg: 100g -> 10g
UPDATE `item_template` SET `BuyPrice` = 1000, `SellPrice` = 250 WHERE `entry` = 47100;   -- Reins of the Striped Dawnsaber: 1g -> 10s
UPDATE `item_template` SET `BuyPrice` = 10000, `SellPrice` = 2500 WHERE `entry` = 47101;   -- Ochre Skeletal Warhorse: 10g -> 1g
UPDATE `item_template` SET `BuyPrice` = 200000, `SellPrice` = 0 WHERE `entry` = 47840;   -- Relentless Gladiator's Frost Wyrm: 200g -> 20g
UPDATE `item_template` SET `BuyPrice` = 10000, `SellPrice` = 2500 WHERE `entry` = 49044;   -- Swift Alliance Steed: 10g -> 1g
UPDATE `item_template` SET `BuyPrice` = 10000, `SellPrice` = 2500 WHERE `entry` = 49046;   -- Swift Horde Wolf: 10g -> 1g
UPDATE `item_template` SET `BuyPrice` = 10000, `SellPrice` = 0 WHERE `entry` = 49282;   -- Big Battle Bear: 10g -> 1g
UPDATE `item_template` SET `BuyPrice` = 1000, `SellPrice` = 0 WHERE `entry` = 49283;   -- Reins of the Spectral Tiger: 1g -> 10s
UPDATE `item_template` SET `BuyPrice` = 10000, `SellPrice` = 0 WHERE `entry` = 49284;   -- Reins of the Swift Spectral Tiger: 10g -> 1g
UPDATE `item_template` SET `BuyPrice` = 100000, `SellPrice` = 0 WHERE `entry` = 49285;   -- X-51 Nether-Rocket: 100g -> 10g
UPDATE `item_template` SET `BuyPrice` = 200000, `SellPrice` = 0 WHERE `entry` = 49286;   -- X-51 Nether-Rocket X-TREME: 200g -> 20g
UPDATE `item_template` SET `BuyPrice` = 100000, `SellPrice` = 0 WHERE `entry` = 49290;   -- Magic Rooster Egg: 100g -> 10g
UPDATE `item_template` SET `BuyPrice` = 200000, `SellPrice` = 0 WHERE `entry` = 49636;   -- Reins of the Onyxian Drake: 200g -> 20g
UPDATE `item_template` SET `BuyPrice` = 200000, `SellPrice` = 0 WHERE `entry` = 50435;   -- Wrathful Gladiator's Frost Wyrm: 200g -> 20g
UPDATE `item_template` SET `BuyPrice` = 200000, `SellPrice` = 0 WHERE `entry` = 51954;   -- Reins of the Bloodbathed Frostbrood Vanquisher: 200g -> 20g
UPDATE `item_template` SET `BuyPrice` = 200000, `SellPrice` = 0 WHERE `entry` = 51955;   -- Reins of the Icebound Frostbrood Vanquisher: 200g -> 20g
UPDATE `item_template` SET `BuyPrice` = 10000, `SellPrice` = 0 WHERE `entry` = 54068;   -- Wooly White Rhino: 10g -> 1g
UPDATE `item_template` SET `BuyPrice` = 10000, `SellPrice` = 0 WHERE `entry` = 54069;   -- Blazing Hippogryph: 10g -> 1g
UPDATE `item_template` SET `BuyPrice` = 200000, `SellPrice` = 0 WHERE `entry` = 54797;   -- Frosty Flying Carpet: 200g -> 20g
UPDATE `item_template` SET `BuyPrice` = 10000, `SellPrice` = 0 WHERE `entry` = 54860;   -- X-53 Touring Rocket: 10g -> 1g
