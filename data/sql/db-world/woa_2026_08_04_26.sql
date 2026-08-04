-- ===========================================================================
-- Rogue: poisons are unlimited-use
-- ===========================================================================
--
-- Applying a poison to a weapon used to destroy one unit from the stack, so a
-- rogue had to keep re-buying/re-crafting poisons purely as upkeep.  For
-- Alonecraft's solo focus that is friction with no decision attached to it, so
-- a poison is now a permanent item: buy one, keep it forever.
--
-- Spell::EffectEnchantItemTmp (SpellEffects.cpp:2985) never touches the cast
-- item -- Spell::TakeCastItem (Spell.cpp:5241-5282) is the ONLY path that
-- destroys a poison:
--
--     if (proto->Spells[i].SpellCharges)        // <-- 0 short-circuits all of it
--     {
--         if (proto->Spells[i].SpellCharges < 0)
--             expendable = true;
--         ...
--     }
--     if (expendable && withoutCharges)
--         DestroyItemCount(m_CastItem, 1, true);
--
-- Poisons ship with spellcharges_1 = -1 (negative = destroy on last use,
-- magnitude 1 = one use).  Setting it to 0 skips the block entirely: nothing
-- is decremented, `expendable` stays false, the item is never destroyed.
-- Poisonous Mushroom (5823) already uses this convention.
--
-- Because the block is skipped, charge values already stored per item in
-- `item_instance` become irrelevant -- no data migration needed.
--
-- stackable = 1 because one poison of each type is now all a rogue can use.
-- Existing 20-stacks are NOT clamped (Item::LoadFromDB has no stack check;
-- only Item::CreateItem clamps, Item.cpp:1101) -- they keep working and can be
-- vendored off.  Both columns ride in the item query response and are cached
-- client-side, so an existing character may need Cache/WDB/ cleared to see the
-- new tooltip; server behaviour is correct either way.
--
-- Crippling Poison II (3776) is class 15 (Miscellaneous), not class 0 like
-- every other poison -- easy to miss with a class-filtered query, but it is a
-- genuine usable poison (spellid_1 = 11202).
--
-- Deliberately untouched:
--     31535 Bloodboil Poison           -- NPC/encounter item, not a rogue poison
--      5823 Poisonous Mushroom         -- not a rogue poison, already 0 charges
--   6951/9186 Mind-numbing Poison II/III -- legacy rows, spellid_1 = 0, unusable
--     21302 Handbook of Deadly Poison V -- a recipe; SHOULD be consumed on use
--     22796 Apothecary's Poison        -- quest item
--     42958 Glyph of Crippling Poison  -- a glyph
-- ===========================================================================

UPDATE `item_template`
SET `spellcharges_1` = 0, `stackable` = 1
WHERE `entry` IN (
    2892, 2893, 8984, 8985, 20844, 22053, 22054, 43232, 43233,  -- Deadly Poison I-IX
    6947, 6949, 6950, 8926, 8927, 8928, 21927, 43230, 43231,    -- Instant Poison I-IX
    10918, 10920, 10921, 10922, 22055, 43234, 43235,            -- Wound Poison I-VII
    3775, 3776,                                                 -- Crippling Poison I-II
    5237,                                                       -- Mind-numbing Poison
    21835, 43237                                                -- Anesthetic Poison I-II
);
