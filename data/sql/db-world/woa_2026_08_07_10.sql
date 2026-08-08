-- ============================================================
-- Warrior (Arms): Iron Will -> Riposte
-- ============================================================
-- TODO.md: "Iron Will (Renamed to Riposte): Redesigned. Parrying an attack
--           immediately counter-attacks every enemy within 8 yards for
--           40/70/100% weapon damage. Cannot occur more than once every second.
--           Requires 5 points in Deflection. (Only affects enemies where CC
--           wouldn't be broken)"
--
-- Shipped with a 2 second internal cooldown rather than 1, by explicit
-- decision: with the parry rates this tree reaches, a 1 second gate on an
-- uncapped 8 yard counterattack is close to a free full weapon swing per second
-- per target.  The tooltip figure is unchanged; only the cadence is halved.
--
-- Prior art: this is the stock NPC Retaliation (52423 -> 52424) and NPC Riposte
-- (41393 -> 41392) shape -- ProcFlags 40, a spell_proc row with HitMask 32 and
-- a Cooldown, and a triggered damage spell.  Both are pure DB with no script.
-- The AoE half is Whirlwind's off-hand spell 44949, which is the stock
-- "weapon damage to everything in 8 yards" spell.
--
-- ProcFlags 40 = TAKEN_MELEE_AUTO_ATTACK | TAKEN_SPELL_MELEE_DMG_CLASS.  The
-- taken side is load-bearing -- HitMask describes the outcome of the event
-- ProcFlags names, so done-side flags with HitMask PARRY would mean "my attack
-- was parried", the opposite of this talent.
--
-- Spells:
--   12300 / 12959 / 12960     = Riposte ranks 1-3 (the talent)
--   200600 / 200601 / 200602  = the 40 / 70 / 100% counterattacks
-- Talent: 641, unchanged position (TabID 161, Tier 1, Column 1), gains a
--         prerequisite of 5 points in Small Victories.
-- ============================================================

-- ============================================================
-- 200600-200602: the counterattacks
-- ============================================================
-- Cloned from Whirlwind off-hand 44949 for its targeting: EffectImplicitTargetA
-- 22 (TARGET_UNIT_SRC_AREA_ENEMY) with EffectRadiusIndex 14, which is 8.0 yards
-- in SpellRadius.dbc.
--
-- Three things from the donor had to be undone, and each would have been a
-- silent bug:
--   AttributesEx3 0x01000000 = SPELL_ATTR3_REQUIRES_OFF_HAND_WEAPON.  44949 is
--     the *off-hand* half of Whirlwind.  Left in place, Riposte would have
--     required a second weapon and used its damage -- fatal for the two-handed
--     builds this tree is otherwise pushing.
--   MaximumAffectedTargets 4 -> 0.  Whirlwind caps at four; the talent says
--     every enemy within 8 yards.
--   EquippedItemClass 2 / SubClassMask 41395 -> -1 / 0.  The donor's weapon
--     restriction is Whirlwind's, not Riposte's, and parrying already implies a
--     weapon.
--
-- Effect1 becomes SPELL_EFFECT_WEAPON_PERCENT_DAMAGE (31) rather than the
-- donor's NORMALIZED_WEAPON_DMG (121), because the talent is specified as a
-- percentage of weapon damage.  Base points are the percentage directly:
-- SpellEffects.cpp:3611 and :3665 do ApplyPct(weaponDamage, <base points>).
-- 39 / 69 / 99 render 40 / 70 / 100 under the N-1 convention.
--
-- SpellFamilyName is set to 0 rather than 4 on purpose.  As a warrior-family
-- spell these counterattacks would feed every warrior family-mask proc in the
-- tree -- Deep Wounds, Trauma, Blood Frenzy -- from an effect that fires on
-- every parry.  Family 0 keeps the counterattack out of those chains.

DELETE FROM `alonecraft_spell_dbc` WHERE `ID` IN (200600, 200601, 200602);
INSERT INTO `alonecraft_spell_dbc` (`ID`, `Category`, `Dispel`, `Mechanic`, `Attributes`, `AttributesEx`, `AttributesEx2`, `AttributesEx3`, `AttributesEx4`, `AttributesEx5`, `AttributesEx6`, `AttributesEx7`, `Stances`, `Unknown1`, `StancesNot`, `Unknown2`, `Targets`, `TargetCreatureType`, `RequiresSpellFocus`, `FacingCasterFlags`, `CasterAuraState`, `TargetAuraState`, `CasterAuraStateNot`, `TargetAuraStateNot`, `CasterAuraSpell`, `TargetAuraSpell`, `ExcludeCasterAuraSpell`, `ExcludeTargetAuraSpell`, `CastingTimeIndex`, `RecoveryTime`, `CategoryRecoveryTime`, `InterruptFlags`, `AuraInterruptFlags`, `ChannelInterruptFlags`, `ProcFlags`, `ProcChance`, `ProcCharges`, `MaximumLevel`, `BaseLevel`, `SpellLevel`, `DurationIndex`, `PowerType`, `ManaCost`, `ManaCostPerLevel`, `ManaPerSecond`, `ManaPerSecondPerLevel`, `RangeIndex`, `Speed`, `ModalNextSpell`, `StackAmount`, `Totem1`, `Totem2`, `Reagent1`, `Reagent2`, `Reagent3`, `Reagent4`, `Reagent5`, `Reagent6`, `Reagent7`, `Reagent8`, `ReagentCount1`, `ReagentCount2`, `ReagentCount3`, `ReagentCount4`, `ReagentCount5`, `ReagentCount6`, `ReagentCount7`, `ReagentCount8`, `EquippedItemClass`, `EquippedItemSubClassMask`, `EquippedItemInventoryTypeMask`, `Effect1`, `Effect2`, `Effect3`, `EffectDieSides1`, `EffectDieSides2`, `EffectDieSides3`, `EffectRealPointsPerLevel1`, `EffectRealPointsPerLevel2`, `EffectRealPointsPerLevel3`, `EffectBasePoints1`, `EffectBasePoints2`, `EffectBasePoints3`, `EffectMechanic1`, `EffectMechanic2`, `EffectMechanic3`, `EffectImplicitTargetA1`, `EffectImplicitTargetA2`, `EffectImplicitTargetA3`, `EffectImplicitTargetB1`, `EffectImplicitTargetB2`, `EffectImplicitTargetB3`, `EffectRadiusIndex1`, `EffectRadiusIndex2`, `EffectRadiusIndex3`, `EffectApplyAuraName1`, `EffectApplyAuraName2`, `EffectApplyAuraName3`, `EffectAmplitude1`, `EffectAmplitude2`, `EffectAmplitude3`, `EffectMultipleValue1`, `EffectMultipleValue2`, `EffectMultipleValue3`, `EffectChainTarget1`, `EffectChainTarget2`, `EffectChainTarget3`, `EffectItemType1`, `EffectItemType2`, `EffectItemType3`, `EffectMiscValue1`, `EffectMiscValue2`, `EffectMiscValue3`, `EffectMiscValueB1`, `EffectMiscValueB2`, `EffectMiscValueB3`, `EffectTriggerSpell1`, `EffectTriggerSpell2`, `EffectTriggerSpell3`, `EffectPointsPerComboPoint1`, `EffectPointsPerComboPoint2`, `EffectPointsPerComboPoint3`, `EffectSpellClassMaskA1`, `EffectSpellClassMaskA2`, `EffectSpellClassMaskA3`, `EffectSpellClassMaskB1`, `EffectSpellClassMaskB2`, `EffectSpellClassMaskB3`, `EffectSpellClassMaskC1`, `EffectSpellClassMaskC2`, `EffectSpellClassMaskC3`, `SpellVisual1`, `SpellVisual2`, `SpellIconID`, `ActiveIconID`, `SpellPriority`, `SpellName0`, `SpellName1`, `SpellName2`, `SpellName3`, `SpellName4`, `SpellName5`, `SpellName6`, `SpellName7`, `SpellName8`, `SpellNameFlag0`, `SpellNameFlag1`, `SpellNameFlag2`, `SpellNameFlag3`, `SpellNameFlag4`, `SpellNameFlag5`, `SpellNameFlag6`, `SpellNameFlag7`, `SpellRank0`, `SpellRank1`, `SpellRank2`, `SpellRank3`, `SpellRank4`, `SpellRank5`, `SpellRank6`, `SpellRank7`, `SpellRank8`, `SpellRankFlags0`, `SpellRankFlags1`, `SpellRankFlags2`, `SpellRankFlags3`, `SpellRankFlags4`, `SpellRankFlags5`, `SpellRankFlags6`, `SpellRankFlags7`, `SpellDescription0`, `SpellDescription1`, `SpellDescription2`, `SpellDescription3`, `SpellDescription4`, `SpellDescription5`, `SpellDescription6`, `SpellDescription7`, `SpellDescription8`, `SpellDescriptionFlags0`, `SpellDescriptionFlags1`, `SpellDescriptionFlags2`, `SpellDescriptionFlags3`, `SpellDescriptionFlags4`, `SpellDescriptionFlags5`, `SpellDescriptionFlags6`, `SpellDescriptionFlags7`, `SpellToolTip0`, `SpellToolTip1`, `SpellToolTip2`, `SpellToolTip3`, `SpellToolTip4`, `SpellToolTip5`, `SpellToolTip6`, `SpellToolTip7`, `SpellToolTip8`, `SpellToolTipFlags0`, `SpellToolTipFlags1`, `SpellToolTipFlags2`, `SpellToolTipFlags3`, `SpellToolTipFlags4`, `SpellToolTipFlags5`, `SpellToolTipFlags6`, `SpellToolTipFlags7`, `ManaCostPercentage`, `StartRecoveryCategory`, `StartRecoveryTime`, `MaximumTargetLevel`, `SpellFamilyName`, `SpellFamilyFlags`, `SpellFamilyFlags1`, `SpellFamilyFlags2`, `MaximumAffectedTargets`, `DamageClass`, `PreventionType`, `StanceBarOrder`, `EffectDamageMultiplier1`, `EffectDamageMultiplier2`, `EffectDamageMultiplier3`, `MinimumFactionId`, `MinimumReputation`, `RequiredAuraVision`, `TotemCategory1`, `TotemCategory2`, `AreaGroupID`, `SchoolMask`, `RuneCostID`, `SpellMissileID`, `PowerDisplayId`, `EffectBonusMultiplier1`, `EffectBonusMultiplier2`, `EffectBonusMultiplier3`, `SpellDescriptionVariableID`, `SpellDifficultyID`) VALUES
(200600, 0, 0, 0, 327696, 16, 0, 0, 128, 32768, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 36, 36, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 31, 0, 0, 1, 1, 0, 0, 0, 0, 39, -1, 0, 0, 0, 0, 22, 0, 0, 15, 0, 0, 14, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 223, 0, 83, 0, 0, 'Riposte', '', '', '', '', '', '', '', '', '', '', '', '', '', '', '', 16712190, '', '', '', '', '', '', '', '', '', '', '', '', '', '', '', '', 16712172, 'Counterattacks all nearby enemies for $s1% weapon damage.', '', '', '', '', '', '', '', '', 0, 0, 0, 0, 0, 0, 0, 16712188, '', '', '', '', '', '', '', '', '', 0, 0, 0, 0, 0, 0, 0, 16712188, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0),
(200601, 0, 0, 0, 327696, 16, 0, 0, 128, 32768, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 36, 36, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 31, 0, 0, 1, 1, 0, 0, 0, 0, 69, -1, 0, 0, 0, 0, 22, 0, 0, 15, 0, 0, 14, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 223, 0, 83, 0, 0, 'Riposte', '', '', '', '', '', '', '', '', '', '', '', '', '', '', '', 16712190, '', '', '', '', '', '', '', '', '', '', '', '', '', '', '', '', 16712172, 'Counterattacks all nearby enemies for $s1% weapon damage.', '', '', '', '', '', '', '', '', 0, 0, 0, 0, 0, 0, 0, 16712188, '', '', '', '', '', '', '', '', '', 0, 0, 0, 0, 0, 0, 0, 16712188, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0),
(200602, 0, 0, 0, 327696, 16, 0, 0, 128, 32768, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 36, 36, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 31, 0, 0, 1, 1, 0, 0, 0, 0, 99, -1, 0, 0, 0, 0, 22, 0, 0, 15, 0, 0, 14, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 223, 0, 83, 0, 0, 'Riposte', '', '', '', '', '', '', '', '', '', '', '', '', '', '', '', 16712190, '', '', '', '', '', '', '', '', '', '', '', '', '', '', '', '', 16712172, 'Counterattacks all nearby enemies for $s1% weapon damage.', '', '', '', '', '', '', '', '', 0, 0, 0, 0, 0, 0, 0, 16712188, '', '', '', '', '', '', '', '', '', 0, 0, 0, 0, 0, 0, 0, 16712188, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0);

-- ============================================================
-- 12300 / 12959 / 12960: the talent
-- ============================================================
-- Both stock effects were SPELL_AURA_MECHANIC_DURATION_MOD (232) -- the stun and
-- charm duration reduction.  Effect 1 is repurposed and effect 2 is zeroed out
-- entirely.
--
-- EffectMiscValue1 must be cleared with the aura change.  It held 1
-- (MECHANIC_CHARM), which is load-bearing for aura 232 and meaningless for
-- SPELL_AURA_PROC_TRIGGER_SPELL -- exactly the kind of leftover that makes a
-- repurposed effect behave oddly with no error anywhere.
--
-- SpellFamilyName stays 0, matching the spell_proc row below.

DELETE FROM `alonecraft_spell_dbc` WHERE `ID` IN (12300, 12959, 12960);
INSERT INTO `alonecraft_spell_dbc` (`ID`, `Category`, `Dispel`, `Mechanic`, `Attributes`, `AttributesEx`, `AttributesEx2`, `AttributesEx3`, `AttributesEx4`, `AttributesEx5`, `AttributesEx6`, `AttributesEx7`, `Stances`, `Unknown1`, `StancesNot`, `Unknown2`, `Targets`, `TargetCreatureType`, `RequiresSpellFocus`, `FacingCasterFlags`, `CasterAuraState`, `TargetAuraState`, `CasterAuraStateNot`, `TargetAuraStateNot`, `CasterAuraSpell`, `TargetAuraSpell`, `ExcludeCasterAuraSpell`, `ExcludeTargetAuraSpell`, `CastingTimeIndex`, `RecoveryTime`, `CategoryRecoveryTime`, `InterruptFlags`, `AuraInterruptFlags`, `ChannelInterruptFlags`, `ProcFlags`, `ProcChance`, `ProcCharges`, `MaximumLevel`, `BaseLevel`, `SpellLevel`, `DurationIndex`, `PowerType`, `ManaCost`, `ManaCostPerLevel`, `ManaPerSecond`, `ManaPerSecondPerLevel`, `RangeIndex`, `Speed`, `ModalNextSpell`, `StackAmount`, `Totem1`, `Totem2`, `Reagent1`, `Reagent2`, `Reagent3`, `Reagent4`, `Reagent5`, `Reagent6`, `Reagent7`, `Reagent8`, `ReagentCount1`, `ReagentCount2`, `ReagentCount3`, `ReagentCount4`, `ReagentCount5`, `ReagentCount6`, `ReagentCount7`, `ReagentCount8`, `EquippedItemClass`, `EquippedItemSubClassMask`, `EquippedItemInventoryTypeMask`, `Effect1`, `Effect2`, `Effect3`, `EffectDieSides1`, `EffectDieSides2`, `EffectDieSides3`, `EffectRealPointsPerLevel1`, `EffectRealPointsPerLevel2`, `EffectRealPointsPerLevel3`, `EffectBasePoints1`, `EffectBasePoints2`, `EffectBasePoints3`, `EffectMechanic1`, `EffectMechanic2`, `EffectMechanic3`, `EffectImplicitTargetA1`, `EffectImplicitTargetA2`, `EffectImplicitTargetA3`, `EffectImplicitTargetB1`, `EffectImplicitTargetB2`, `EffectImplicitTargetB3`, `EffectRadiusIndex1`, `EffectRadiusIndex2`, `EffectRadiusIndex3`, `EffectApplyAuraName1`, `EffectApplyAuraName2`, `EffectApplyAuraName3`, `EffectAmplitude1`, `EffectAmplitude2`, `EffectAmplitude3`, `EffectMultipleValue1`, `EffectMultipleValue2`, `EffectMultipleValue3`, `EffectChainTarget1`, `EffectChainTarget2`, `EffectChainTarget3`, `EffectItemType1`, `EffectItemType2`, `EffectItemType3`, `EffectMiscValue1`, `EffectMiscValue2`, `EffectMiscValue3`, `EffectMiscValueB1`, `EffectMiscValueB2`, `EffectMiscValueB3`, `EffectTriggerSpell1`, `EffectTriggerSpell2`, `EffectTriggerSpell3`, `EffectPointsPerComboPoint1`, `EffectPointsPerComboPoint2`, `EffectPointsPerComboPoint3`, `EffectSpellClassMaskA1`, `EffectSpellClassMaskA2`, `EffectSpellClassMaskA3`, `EffectSpellClassMaskB1`, `EffectSpellClassMaskB2`, `EffectSpellClassMaskB3`, `EffectSpellClassMaskC1`, `EffectSpellClassMaskC2`, `EffectSpellClassMaskC3`, `SpellVisual1`, `SpellVisual2`, `SpellIconID`, `ActiveIconID`, `SpellPriority`, `SpellName0`, `SpellName1`, `SpellName2`, `SpellName3`, `SpellName4`, `SpellName5`, `SpellName6`, `SpellName7`, `SpellName8`, `SpellNameFlag0`, `SpellNameFlag1`, `SpellNameFlag2`, `SpellNameFlag3`, `SpellNameFlag4`, `SpellNameFlag5`, `SpellNameFlag6`, `SpellNameFlag7`, `SpellRank0`, `SpellRank1`, `SpellRank2`, `SpellRank3`, `SpellRank4`, `SpellRank5`, `SpellRank6`, `SpellRank7`, `SpellRank8`, `SpellRankFlags0`, `SpellRankFlags1`, `SpellRankFlags2`, `SpellRankFlags3`, `SpellRankFlags4`, `SpellRankFlags5`, `SpellRankFlags6`, `SpellRankFlags7`, `SpellDescription0`, `SpellDescription1`, `SpellDescription2`, `SpellDescription3`, `SpellDescription4`, `SpellDescription5`, `SpellDescription6`, `SpellDescription7`, `SpellDescription8`, `SpellDescriptionFlags0`, `SpellDescriptionFlags1`, `SpellDescriptionFlags2`, `SpellDescriptionFlags3`, `SpellDescriptionFlags4`, `SpellDescriptionFlags5`, `SpellDescriptionFlags6`, `SpellDescriptionFlags7`, `SpellToolTip0`, `SpellToolTip1`, `SpellToolTip2`, `SpellToolTip3`, `SpellToolTip4`, `SpellToolTip5`, `SpellToolTip6`, `SpellToolTip7`, `SpellToolTip8`, `SpellToolTipFlags0`, `SpellToolTipFlags1`, `SpellToolTipFlags2`, `SpellToolTipFlags3`, `SpellToolTipFlags4`, `SpellToolTipFlags5`, `SpellToolTipFlags6`, `SpellToolTipFlags7`, `ManaCostPercentage`, `StartRecoveryCategory`, `StartRecoveryTime`, `MaximumTargetLevel`, `SpellFamilyName`, `SpellFamilyFlags`, `SpellFamilyFlags1`, `SpellFamilyFlags2`, `MaximumAffectedTargets`, `DamageClass`, `PreventionType`, `StanceBarOrder`, `EffectDamageMultiplier1`, `EffectDamageMultiplier2`, `EffectDamageMultiplier3`, `MinimumFactionId`, `MinimumReputation`, `RequiredAuraVision`, `TotemCategory1`, `TotemCategory2`, `AreaGroupID`, `SchoolMask`, `RuneCostID`, `SpellMissileID`, `PowerDisplayId`, `EffectBonusMultiplier1`, `EffectBonusMultiplier2`, `EffectBonusMultiplier3`, `SpellDescriptionVariableID`, `SpellDifficultyID`) VALUES
(12300, 0, 0, 0, 262352, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 40, 100, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 6, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 42, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 200600, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 332, 0, 50, 'Riposte', '', '', '', '', '', '', '', '', '', '', '', '', '', '', '', 16712190, 'Rank 1', '', '', '', '', '', '', '', '', '', '', '', '', '', '', '', 16712190, 'Parrying an attack immediately counterattacks every enemy within 8 yards for $200600s1% weapon damage.  Cannot occur more than once every 2 sec.  Requires 5 points in Small Victories.', '', '', '', '', '', '', '', '', 0, 0, 0, 0, 0, 0, 0, 16712190, '', '', '', '', '', '', '', '', '', 0, 0, 0, 0, 0, 0, 0, 16712188, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0),
(12959, 0, 0, 0, 262352, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 40, 100, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 6, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 42, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 200601, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 332, 0, 50, 'Riposte', '', '', '', '', '', '', '', '', '', '', '', '', '', '', '', 16712190, 'Rank 2', '', '', '', '', '', '', '', '', '', '', '', '', '', '', '', 16712190, 'Parrying an attack immediately counterattacks every enemy within 8 yards for $200601s1% weapon damage.  Cannot occur more than once every 2 sec.  Requires 5 points in Small Victories.', '', '', '', '', '', '', '', '', 0, 0, 0, 0, 0, 0, 0, 16712190, '', '', '', '', '', '', '', '', '', 0, 0, 0, 0, 0, 0, 0, 16712188, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0),
(12960, 0, 0, 0, 262352, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 40, 100, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 6, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 42, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 200602, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 332, 0, 50, 'Riposte', '', '', '', '', '', '', '', '', '', '', '', '', '', '', '', 16712190, 'Rank 3', '', '', '', '', '', '', '', '', '', '', '', '', '', '', '', 16712190, 'Parrying an attack immediately counterattacks every enemy within 8 yards for $200602s1% weapon damage.  Cannot occur more than once every 2 sec.  Requires 5 points in Small Victories.', '', '', '', '', '', '', '', '', 0, 0, 0, 0, 0, 0, 0, 16712190, '', '', '', '', '', '', '', '', '', 0, 0, 0, 0, 0, 0, 0, 16712188, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0);

-- ============================================================
-- spell_proc
-- ============================================================
-- HitMask 32 = PROC_EX_PARRY.  Cooldown 2000 is the "cannot occur more than
-- once every 2 sec" clause, enforced by Aura::IsProcOnCooldown
-- (SpellAuras.cpp:2083-2101).
--
-- SpellFamilyName 0 matches the talent.  A warrior family mask here would be
-- tested against the incoming enemy attack and nothing would ever proc.

DELETE FROM `spell_proc` WHERE `SpellId` = -12300;
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(-12300, 0, 0, 0, 0, 0, 0, 0, 2, 32, 0, 0, 0, 0, 2000, 0);

-- ============================================================
-- Script registration: the crowd-control filter
-- ============================================================
-- Registered on the three counterattacks, not on the talent.  The script does
-- one thing: drop targets that a hit would break crowd control on.  Everything
-- else about the spell is DBC.

DELETE FROM `spell_script_names` WHERE `ScriptName` = 'spell_warr_riposte';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(200600, 'spell_warr_riposte'),
(200601, 'spell_warr_riposte'),
(200602, 'spell_warr_riposte');

-- ============================================================
-- talent_dbc: require 5 points in Small Victories
-- ============================================================
-- Position is unchanged (TabID 161 Arms, Tier 1, Column 1).  Small Victories is
-- talent 130 at Tier 0 Column 1, directly above, so the client draws the arrow.
--
-- PrereqRank_1 is ZERO-INDEXED: 4 means "5 points".  Writing 5 would demand a
-- sixth rank that does not exist and leave Riposte permanently unlearnable.
--
-- talent_dbc overrides replace the whole record, so every column is restated.
-- Record ordering is not a concern -- build_dbc.py re-sorts by
-- (TabID, TierID, ColumnIndex).

DELETE FROM `talent_dbc` WHERE `ID` = 641;
INSERT INTO `talent_dbc` (
    `ID`, `TabID`, `TierID`, `ColumnIndex`,
    `SpellRank_1`, `SpellRank_2`, `SpellRank_3`, `SpellRank_4`, `SpellRank_5`,
    `SpellRank_6`, `SpellRank_7`, `SpellRank_8`, `SpellRank_9`,
    `PrereqTalent_1`, `PrereqTalent_2`, `PrereqTalent_3`,
    `PrereqRank_1`, `PrereqRank_2`, `PrereqRank_3`,
    `Flags`, `RequiredSpellID`, `CategoryMask_1`, `CategoryMask_2`
) VALUES (
    641, 161, 1, 1,
    12300, 12959, 12960, 0, 0,
    0, 0, 0, 0,
    130, 0, 0,
    4, 0, 0,
    0, 0, 0, 0
);
