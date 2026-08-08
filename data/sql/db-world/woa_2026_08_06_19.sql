-- ==========================================================================
-- Variant naming: stop the doubled adjective
-- ==========================================================================
--
-- woa_2026_08_06_18.sql prefixes each variant with its tool's adjective, which
-- reads badly when the item already starts with that word: "Brilliant Gold
-- Ring" became "Brilliant Brilliant Gold Ring".
--
-- 16 rows are affected, across four of the sixteen adjectives -- Gleaming,
-- Brilliant, Woven and Eternal.  Each level now has a fallback adjective
-- (TOOL_ALT_PREFIX in tools/gen_upgrade_tools.py) used only on collision, so
-- those become Burnished / Lustrous / Threaded / Undying.
--
-- The fallbacks are distinct from all sixteen primaries, not merely from their
-- own.  Reusing one would make the adjective ambiguous about the level, and
-- being able to read a level off the name is the whole point of the scheme.
--
-- Only exact repeats are touched.  315 rows have two DIFFERENT adjectives in a
-- row -- "Eternal Radiant Silver Bracers" -- which reads like ordinary fantasy
-- naming and is deliberately left alone.
--
-- Idempotent: after the update the first word no longer equals the second, so
-- the WHERE matches nothing on a re-apply.  gen_item_variants.py now applies
-- the same rule at generation time, so a regenerated set never needs this file.

UPDATE `item_template`
   SET `name` = CONCAT(
           CASE `RequiredLevel`
               WHEN  5 THEN 'Guttering'
               WHEN 10 THEN 'Faded'
               WHEN 15 THEN 'Twinkling'
               WHEN 20 THEN 'Beaming'
               WHEN 25 THEN 'Riven'
               WHEN 30 THEN 'Limpid'
               WHEN 35 THEN 'Burnished'
               WHEN 40 THEN 'Lustrous'
               WHEN 45 THEN 'Wavering'
               WHEN 50 THEN 'Humming'
               WHEN 55 THEN 'Fused'
               WHEN 60 THEN 'Prismatic'
               WHEN 65 THEN 'Threaded'
               WHEN 70 THEN 'Rising'
               WHEN 75 THEN 'Exalted'
               WHEN 80 THEN 'Undying'
           END,
           ' ',
           -- everything after the duplicated first word
           SUBSTRING(`name`, CHAR_LENGTH(SUBSTRING_INDEX(`name`, ' ', 1)) + 2))
 WHERE `entry` >= 1000000
   AND SUBSTRING_INDEX(`name`, ' ', 1)
     = SUBSTRING_INDEX(SUBSTRING_INDEX(`name`, ' ', 2), ' ', -1);
