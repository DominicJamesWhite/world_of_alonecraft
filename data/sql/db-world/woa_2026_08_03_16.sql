-- Alonecraft 4.61 -- buff bar tooltips for custom Destruction buffs.
--
-- The WoW 3.3.5a buff bar renders `SpellToolTip`, NOT `SpellDescription`.
-- SpellDescription is what the spellbook / talent pane shows.  Four custom
-- buffs had a correct description but an absent or inherited tooltip, so
-- hovering the buff icon in game showed either nothing or the wrong text:
--
--   200402 Wailing Soul        -- cloned from Nether Protection 54370 and
--                                 inherited its tooltip verbatim,
--                                 "Holy spell damage reduced by $s1%."
--   200507 Nathrezim Foresight -- empty tooltip, no hover text at all
--   200405 Infernal Bargain    -- empty tooltip
--   200506 Backlash (ICD)      -- empty tooltip
--
-- Single-column UPDATEs rather than the usual DELETE + 234-column INSERT:
-- only one field changes, so this cannot drift the other 233 columns, and
-- re-running it is a no-op.  The rows are created by woa_2026_08_01_06.sql,
-- woa_2026_08_01_08.sql and woa_2026_08_03_12.sql, which sort earlier.

-- Wailing Soul: mirrors its own description. Negative base points with $s1
-- render the same way they do on Nether Protection, which this is a clone of.
UPDATE `alonecraft_spell_dbc`
SET `SpellToolTip0` = 'Damage taken reduced by $s1%.'
WHERE `ID` = 200402;

-- Nathrezim Foresight: deliberately plain text rather than $s1/$s2. The aura
-- is per-stack (-1% taken / +1% crit, engine multiplies by GetStackAmount()),
-- and the client does not scale tooltip variables by stack count, so $s1 would
-- always read "-1%" no matter how many shards you carry.
UPDATE `alonecraft_spell_dbc`
SET `SpellToolTip0` = 'Damage taken reduced by 1% and spell critical strike chance increased by 1% for each Soul Shard you carry.'
WHERE `ID` = 200507;

-- Infernal Bargain: $s1 = 3 + max(1,1) = 4, $s2 = 1 + max(1,1) = 2, per stack.
UPDATE `alonecraft_spell_dbc`
SET `SpellToolTip0` = 'Damage increased by $s1%.  Spell critical strike chance increased by $s2%.'
WHERE `ID` = 200405;

-- Backlash: the 60s cheat-death ICD marker.
UPDATE `alonecraft_spell_dbc`
SET `SpellToolTip0` = 'Backlash cannot save you from death again yet.'
WHERE `ID` = 200506;
