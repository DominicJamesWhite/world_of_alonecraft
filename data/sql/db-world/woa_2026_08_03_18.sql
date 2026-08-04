-- Alonecraft 4.61 -- buff bar tooltips for the remaining custom auras.
--
-- Companion to woa_2026_08_03_16.sql. Same root cause: the 3.3.5a buff bar
-- renders `SpellToolTip`, not `SpellDescription`, so a custom aura with a
-- populated description and an empty tooltip shows no hover text at all.
--
-- This file covers every remaining ID >= 200000 that applies a visible aura
-- (Effect = 6, not PASSIVE 0x40, not DO_NOT_DISPLAY 0x80) and had an empty
-- SpellToolTip0.
--
-- Where the amount is set by C++ at aura-apply time rather than by the DBC,
-- the tooltip deliberately uses plain text instead of $s1/$s2 -- the base
-- points are 0 in those rows, so a variable would render "0%".

-- --------------------------------------------------------------- Warlock ---

-- Sacrifice of Blood (WM-05). Both amounts are written by the Health Funnel
-- script (-15/-30% taken, +25/+50% done by rank), so no $s variables.
UPDATE `alonecraft_spell_dbc`
SET `SpellToolTip0` = 'Damage taken reduced and damage dealt increased while your summoner channels Health Funnel.'
WHERE `ID` = 200411;

-- Nether Scar (WM-07). A debuff on the victim, so it is worded from the
-- target's side rather than the caster's.
UPDATE `alonecraft_spell_dbc`
SET `SpellToolTip0` = 'Shadow damage taken from the warlock who applied this scar is increased.'
WHERE `ID` = 200413;

-- Fel Domination (WM-08). Amount is recomputed per DoT count, cap 8.
UPDATE `alonecraft_spell_dbc`
SET `SpellToolTip0` = 'Damage increased for each of your summoner''s damage-over-time effects on the target.'
WHERE `ID` = 200416;

-- Demonic Aegis (WM-09). Armor and crit-avoidance are script-applied.
UPDATE `alonecraft_spell_dbc`
SET `SpellToolTip0` = 'Armor greatly increased.  Chance to be critically hit reduced.'
WHERE `ID` = 200417;

-- ---------------------------------------------------------------- Shaman ---

-- Healing Way. Effect 1 is aura 79 with BasePoints 9 + DieSides 1 = 10, so
-- $s1 renders correctly here.
UPDATE `alonecraft_spell_dbc`
SET `SpellToolTip0` = 'Damage increased by $s1%.  Chain Lightning''s cooldown is reduced.'
WHERE `ID` = 200234;

-- ------------------------------------------------------- Encounter debuffs --
--
-- These three sit on NPCs (Iron Council / Overwhelmed Growth), so they are
-- hovered on the target frame rather than the player's buff bar.
--
-- NOTE: their DieSides are 0, so the applied amount is BasePoints + max(1,0),
-- i.e. -65 / -32 / -74, one short of the -66 / -33 / -75 the descriptions
-- claim. Left alone deliberately -- these are encounter tuning numbers and a
-- 1% drift is cosmetic, but it is why the tooltips below use plain text
-- rather than $s1.

UPDATE `alonecraft_spell_dbc`
SET `SpellToolTip0` = 'Melee damage reduced by 66%.'
WHERE `ID` = 200308;

UPDATE `alonecraft_spell_dbc`
SET `SpellToolTip0` = 'Melee damage reduced by 33%.'
WHERE `ID` = 200309;

UPDATE `alonecraft_spell_dbc`
SET `SpellToolTip0` = 'Damage done reduced by 75%.'
WHERE `ID` = 200310;
