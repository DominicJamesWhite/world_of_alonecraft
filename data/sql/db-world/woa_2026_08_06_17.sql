-- ==========================================================================
-- Upgrade tools require the level they reforge to
-- ==========================================================================
--
-- The 16 tools shipped with RequiredLevel = 1, so a level 20 character could
-- buy the Eternal Astral Sigil and only discover it was useless on clicking it
-- -- the gate lived solely in the script's CheckCast, which runs after the
-- purchase and produces a chat line rather than anything visible on the item.
--
-- Setting RequiredLevel puts the gate where players already look for it:
--   * the red "Requires Level 80" line appears in the tooltip and in the
--     vendor list, so the wrong tool is visibly wrong before any gold moves
--   * Player::CanUseItem (PlayerStorage.cpp:2414) refuses the use outright
--     with EQUIP_ERR_CANT_EQUIP_LEVEL_I, before the spell is ever cast
--
-- The CheckCast level test stays as the server-side authority; this only moves
-- the first refusal earlier and makes it visible.
--
-- Note this does NOT block the purchase itself -- Player::BuyItemFromVendorSlot
-- has no level check, and buying gear you cannot yet use is normal vendor
-- behaviour everywhere else in the game.  Blocking it would need C++.
--
-- Level = 5 for 200100 rising in fives to 80 for 200115, matching each tool's
-- spell MaximumLevel rather than a second hardcoded table.

UPDATE `item_template`
   SET `RequiredLevel` = (`entry` - 200100 + 1) * 5
 WHERE `entry` BETWEEN 200100 AND 200115;
