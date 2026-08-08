-- ==========================================================================
-- Variant naming: prefix with the tool's adjective, drop the "(NN)" suffix
-- ==========================================================================
--
-- "Arcanite Reaper (65)" becomes "Woven Arcanite Reaper".
--
-- The adjective is the first word of the tool that produces that level --
-- Woven Astral Sigil reforges to 65 -- and each of the 16 is unique, so the
-- mapping is still reversible: "Woven" means level 65 and nothing else.  A
-- prefix reads as part of the item's name the way Blizzard's own affixes do,
-- where a trailing "(65)" always looked like debug output.
--
-- The cost is alphabetical.  Variants no longer sort beside their base item,
-- which is a real loss when scanning a full bank, and is why the suffix was
-- chosen first.
--
-- tools/gen_item_variants.py now emits the prefix directly (build_name_prefix_case,
-- importing TOOL_NAMES from gen_upgrade_tools so the two cannot drift), so this
-- file only has to carry the 77,530 rows already in the database.  On a fresh
-- install woa_2026_08_06_13.sql still writes the old suffixed names and this
-- file renames them a moment later; after the next regeneration the names
-- arrive already prefixed and the WHERE below simply matches nothing.
--
-- Idempotent by construction rather than by DELETE/INSERT: the WHERE clause
-- requires the old " (NN)" suffix to still be present, so a second apply is a
-- no-op.  Without that guard a re-run would produce "Woven Woven Arcanite
-- Reaper" -- and module files DO get re-applied whenever their hash changes.

UPDATE `item_template`
   SET `name` = CONCAT(
           CASE `RequiredLevel`
               WHEN  5 THEN 'Flickering'
               WHEN 10 THEN 'Dim'
               WHEN 15 THEN 'Glimmering'
               WHEN 20 THEN 'Radiant'
               WHEN 25 THEN 'Fractured'
               WHEN 30 THEN 'Lucid'
               WHEN 35 THEN 'Gleaming'
               WHEN 40 THEN 'Brilliant'
               WHEN 45 THEN 'Shifting'
               WHEN 50 THEN 'Resonant'
               WHEN 55 THEN 'Coalesced'
               WHEN 60 THEN 'Refracted'
               WHEN 65 THEN 'Woven'
               WHEN 70 THEN 'Ascendant'
               WHEN 75 THEN 'Transcendent'
               WHEN 80 THEN 'Eternal'
           END,
           ' ',
           -- strip the trailing " (NN)"
           LEFT(`name`, CHAR_LENGTH(`name`)
                        - CHAR_LENGTH(CONCAT(' (', `RequiredLevel`, ')'))))
 WHERE `entry` >= 1000000
   AND `name` LIKE CONCAT('% (', `RequiredLevel`, ')');
