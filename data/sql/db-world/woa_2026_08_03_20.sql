-- Alonecraft 4.61 -- buff bar tooltips, part 3: overridden Blizzard spells.
--
-- woa_2026_08_03_16/18.sql covered custom 200000+ auras. This file covers the
-- other half of the same bug: real Blizzard spells whose Alonecraft override
-- ADDED a player-visible aura to a spell that never had one, and therefore
-- never had a SpellToolTip either. The buff lands, scales and works -- it just
-- has no hover text, because the buff bar renders SpellToolTip and not
-- SpellDescription.
--
-- Found by diffing every alonecraft_spell_dbc row against base Spell.dbc for
-- rows that apply an aura (Effect = 6), are not PASSIVE (0x40) or
-- DO_NOT_DISPLAY (0x80), and have an empty SpellToolTip0.
--
-- Ordering note: this file must sort AFTER woa_2026_08_03_19.sql, which does a
-- full-row DELETE + INSERT on 63349 with an empty SpellToolTip0. Single-column
-- UPDATEs here so the custom SpellVisual1 = 200001 set by that file survives.

-- ----------------------------------------------------------------- Rogue ---
--
-- Feint (RS-01). Base Feint was an instant threat drop with no buff at all, so
-- it had no tooltip. The Alonecraft version applies aura 47
-- (MOD_PARRY_PERCENT) for 15s, which does show in the buff bar.
-- $s1 resolves per rank: BasePoints 19 + DieSides 1 = 20% at rank 1,
-- 44 + 1 = 45% at rank 7.

UPDATE `alonecraft_spell_dbc`
SET `SpellToolTip0` = 'Chance to parry increased by $s1%.'
WHERE `ID` IN (1966, 6768, 8637, 11303, 25302, 27448, 48658);

-- --------------------------------------------------------------- Warlock ---
--
-- Infernal Bargain channel (WD-11). Effect 2 is aura 87 with BasePoints -26 +
-- DieSides 1 = -25: the damage reduction that applies only while channeling.
-- The 4-stack buff it grants (200405) is a separate spell, already covered by
-- woa_2026_08_03_16.sql.

UPDATE `alonecraft_spell_dbc`
SET `SpellToolTip0` = 'Damage taken reduced by $s2%.'
WHERE `ID` = 63349;

-- ---------------------------------------------------------------- Shaman ---
--
-- Riptide. 61295 was rebuilt as a dummy that picks a friendly or hostile
-- outcome (see ShamanRiptide.cpp), triggering these per-rank children. The
-- rewrite in 2026_04_02_05.sql blanked the parent's original tooltip
-- ('Heals $s2 every $t2 seconds. ...') and the children were created without
-- one, so neither the HoT nor the DoT has hover text.
--
-- 200222-225: Effect2 = aura 8 (PERIODIC_HEAL), amplitude 3000.
-- 200226-229: Effect2 = aura 3 (PERIODIC_DAMAGE), amplitude 3000.
-- $s2 / $t2 resolve against effect 2 in both cases.

UPDATE `alonecraft_spell_dbc`
SET `SpellToolTip0` = 'Heals $s2 every $t2 seconds.'
WHERE `ID` IN (200222, 200223, 200224, 200225);

UPDATE `alonecraft_spell_dbc`
SET `SpellToolTip0` = 'Deals $s2 Nature damage every $t2 seconds.'
WHERE `ID` IN (200226, 200227, 200228, 200229);

-- ---------------------------------------------------------------------------
-- Deliberately NOT changed: Torment (3716, 7809-7811, 11774, 11775, 27270,
-- 47984). The scan flags these because WM-17 gave them a taunt aura, but
-- Torment is a pet action-bar ability -- that bar renders SpellDescription,
-- not SpellToolTip, and the taunt places no visible buff on the player. A
-- tooltip there would be dead data.
-- ---------------------------------------------------------------------------
