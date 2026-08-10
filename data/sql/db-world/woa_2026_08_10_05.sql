-- ============================================================
-- Warrior (Arms): Tactical Mastery requires a two-handed weapon
-- ============================================================
-- The redesigned Tactical Mastery (woa_2026_08_07_11.sql) is parry <-> crit:
-- parrying grants 3/6/10% crit, critting grants 3/6/10% parry.  Both halves
-- are worth far more to a sword-and-board Protection warrior than to the Arms
-- warrior whose tree it sits in -- a shield user parries constantly, so the
-- crit buff is close to permanent uptime and the parry buff compounds with a
-- parry rate that is already high.  Gating it on a two-handed weapon puts the
-- talent back where the tree intends it: solo Arms.
--
-- Done entirely in the DBC, with no C++.  EquippedItemClass = 2
-- (ITEM_CLASS_WEAPON) plus EquippedItemSubClassMask gates the talent in two
-- independent places, both in core:
--
--   * Player::ApplyItemDependentAuras (Player.cpp:7132) removes the passive
--     aura outright when the equipped set stops satisfying the requirement and
--     re-casts it when it starts again, because Attributes 464 carries
--     SPELL_ATTR0_PASSIVE (0x40) and EquippedItemClass is now >= 0.  So the
--     talent visibly is not active while a shield is out, rather than sitting
--     there doing nothing.
--   * Aura::IsProcTriggeredOnEvent (SpellAuras.cpp:2220) re-checks the weapon
--     in the slot the event came from before any effect procs.  AttributesEx3
--     is 0, so SPELL_ATTR3_NO_PROC_EQUIP_REQUIREMENT is not set and this check
--     is live.  Both directions of the proc carry a melee DamageInfo with
--     BASE_ATTACK, which resolves to the main hand -- the two-hander.
--
-- Mask 354 = (1 << 1) axe2h | (1 << 5) mace2h | (1 << 6) polearm |
-- (1 << 8) sword2h.  This is copied verbatim from Two-Handed Weapon
-- Specialization (12712), the sibling talent one tier down, so the two agree
-- on what "two-handed" means.  Note it excludes staves (subclass 10) exactly
-- as retail's does; if staff-wielding warriors should qualify, both spells
-- want 1378, not just this one.
--
-- spell_warr_tactical_mastery is untouched.  It only routes effect 0 to the
-- parry check and effect 1 to the crit check; the weapon gate happens before
-- CheckEffectProc is ever reached.
UPDATE `alonecraft_spell_dbc`
    SET `EquippedItemClass` = 2,
        `EquippedItemSubClassMask` = 354
    WHERE `ID` IN (12295, 12676, 12677);

-- The client renders no automatic "Requires a two-handed weapon" line for a
-- passive talent, so the requirement has to be stated in the prose or it is
-- invisible until a player notices the buffs have stopped.  Only
-- SpellDescription0 is set: the talent pane reads it, and these rows are
-- SPELL_ATTR0_DO_NOT_DISPLAY (0x80) passives that never reach the buff bar,
-- so SpellToolTip0 stays empty as before.
UPDATE `alonecraft_spell_dbc` SET `SpellDescription0` =
    'While wielding a two-handed weapon, parrying an attack grants you $200610s1% critical strike chance for $200610d, and landing a critical strike grants you $200613s1% parry chance for $200613d.'
    WHERE `ID` = 12295;
UPDATE `alonecraft_spell_dbc` SET `SpellDescription0` =
    'While wielding a two-handed weapon, parrying an attack grants you $200611s1% critical strike chance for $200611d, and landing a critical strike grants you $200614s1% parry chance for $200614d.'
    WHERE `ID` = 12676;
UPDATE `alonecraft_spell_dbc` SET `SpellDescription0` =
    'While wielding a two-handed weapon, parrying an attack grants you $200612s1% critical strike chance for $200612d, and landing a critical strike grants you $200615s1% parry chance for $200615d.'
    WHERE `ID` = 12677;
