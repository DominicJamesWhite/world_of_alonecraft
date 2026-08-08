-- ============================================================
-- Warrior (Arms): Second Wind -- health gate instead of stun/root
-- ============================================================
-- TODO.md: "Second Wind (0, 6): Being hit by while you are below 35% health
--           generates 20 rage and 10% of your total health over 10 sec.
--           (2 ranks)"
--
-- The payload spells need no change whatsoever.  Stock 29841 is 2 rage + 1% of
-- health per 2 sec tick over 10 sec (10 rage + 5% total) and 29842 is double
-- that (20 rage + 10%), which is exactly the agreed rank 1 = half, rank 2 =
-- full split.  Only their descriptions are realigned with the new trigger.
--
-- What changes is the gate.  Stock Second Wind fires on stun or immobilise,
-- and that condition lives in C++, not in the DBC: spell_warr_second_wind
-- (src/server/scripts/Spells/spell_warrior.cpp:996-1048) checks the incoming
-- spell's mechanic mask for MECHANIC_ROOT / MECHANIC_STUN in CheckProc, and
-- also owns the rank -> payload cast.  So the script has to go, and the DBC has
-- to take over both halves.
--
-- No C++ replaces it, because the health condition has a native DBC form:
--   CasterAuraState = 13 = AURA_STATE_HEALTHLESS_35_PERCENT.
--
-- That is a genuine gate on the proc, not just a cast-time check:
--   Unit::Update re-evaluates the state on every tick (Unit.cpp:632);
--   Unit::ModifyAuraState applies/unapplies every effect of an aura whose
--     CasterAuraState matches as it toggles (Unit.cpp:7624-7661);
--   Aura::GetProcEffectMask seeds its mask from the application's effect mask
--     and returns 0 when it is empty (SpellAuras.cpp:2201-2215).
-- So above 35% health the aura simply has no active effects and cannot proc.
-- Player.cpp:3226 covers the learn path and PlayerStorage.cpp:5708 the login
-- path, so the state is correct after a relog too.
--
-- The hit that takes you below 35% counts: Unit::AttackerStateUpdate calls
-- DealMeleeDamage before ProcSkillsAndAuras (Unit.cpp:2844 then 2846-2848).
--
-- ProcFlags 1048576 = 0x100000 = PROC_FLAG_TAKEN_DAMAGE, replacing stock's
-- 174760 / 699048 (which were broad taken-spell masks shaped around the
-- stun/root gate).  "Being hit" is now the whole trigger.
--
-- Spells:
--   29834 / 29838 = Second Wind ranks 1-2 (the talent)
--   29841 / 29842 = the payloads -- mechanically untouched
-- ============================================================

-- ============================================================
-- Displace the core script
-- ============================================================
-- Deleting core's registration rather than editing spell_warrior.cpp keeps this
-- a module-only change; the script is simply never loaded.  Idempotent, and the
-- updates table stops core base SQL from re-adding the row.  Deleted by the
-- exact spell_id/ScriptName pair so nothing else on -29834 is disturbed.

DELETE FROM `spell_script_names` WHERE `spell_id` = -29834 AND `ScriptName` = 'spell_warr_second_wind';

-- ============================================================
-- spell_proc
-- ============================================================
-- SpellFamilyName stays 0 (core's row is already 0): a warrior family mask here
-- would be matched against the *incoming enemy* spell and nothing would proc.
-- SpellTypeMask 0 -- stock's 5 was shaped for the old spell-based gate; taken
-- damage is the trigger now.
-- Cooldown 10000 matches the payload duration, so being hit repeatedly at low
-- health refreshes it once per cycle instead of on every swing.

DELETE FROM `spell_proc` WHERE `SpellId` = -29834;
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(-29834, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 10000, 0);

-- ============================================================
-- 29834 / 29838: the talent
-- ============================================================
-- Effect 1 goes from SPELL_AURA_DUMMY (4) -- which existed only as a hook for
-- the deleted script -- to SPELL_AURA_PROC_TRIGGER_SPELL (42) casting the
-- payload directly.  EffectMiscValue1 is zeroed with it: it means nothing for
-- DUMMY but is read by plenty of other aura types, and leaving stale values
-- behind an aura-type change is how effects silently misbehave.
-- EffectSpellClassMaskA1/A2 (0x2010 and 0x8) are zeroed for the same reason.

DELETE FROM `alonecraft_spell_dbc` WHERE `ID` IN (29834, 29838);
INSERT INTO `alonecraft_spell_dbc` (`ID`, `Category`, `Dispel`, `Mechanic`, `Attributes`, `AttributesEx`, `AttributesEx2`, `AttributesEx3`, `AttributesEx4`, `AttributesEx5`, `AttributesEx6`, `AttributesEx7`, `Stances`, `Unknown1`, `StancesNot`, `Unknown2`, `Targets`, `TargetCreatureType`, `RequiresSpellFocus`, `FacingCasterFlags`, `CasterAuraState`, `TargetAuraState`, `CasterAuraStateNot`, `TargetAuraStateNot`, `CasterAuraSpell`, `TargetAuraSpell`, `ExcludeCasterAuraSpell`, `ExcludeTargetAuraSpell`, `CastingTimeIndex`, `RecoveryTime`, `CategoryRecoveryTime`, `InterruptFlags`, `AuraInterruptFlags`, `ChannelInterruptFlags`, `ProcFlags`, `ProcChance`, `ProcCharges`, `MaximumLevel`, `BaseLevel`, `SpellLevel`, `DurationIndex`, `PowerType`, `ManaCost`, `ManaCostPerLevel`, `ManaPerSecond`, `ManaPerSecondPerLevel`, `RangeIndex`, `Speed`, `ModalNextSpell`, `StackAmount`, `Totem1`, `Totem2`, `Reagent1`, `Reagent2`, `Reagent3`, `Reagent4`, `Reagent5`, `Reagent6`, `Reagent7`, `Reagent8`, `ReagentCount1`, `ReagentCount2`, `ReagentCount3`, `ReagentCount4`, `ReagentCount5`, `ReagentCount6`, `ReagentCount7`, `ReagentCount8`, `EquippedItemClass`, `EquippedItemSubClassMask`, `EquippedItemInventoryTypeMask`, `Effect1`, `Effect2`, `Effect3`, `EffectDieSides1`, `EffectDieSides2`, `EffectDieSides3`, `EffectRealPointsPerLevel1`, `EffectRealPointsPerLevel2`, `EffectRealPointsPerLevel3`, `EffectBasePoints1`, `EffectBasePoints2`, `EffectBasePoints3`, `EffectMechanic1`, `EffectMechanic2`, `EffectMechanic3`, `EffectImplicitTargetA1`, `EffectImplicitTargetA2`, `EffectImplicitTargetA3`, `EffectImplicitTargetB1`, `EffectImplicitTargetB2`, `EffectImplicitTargetB3`, `EffectRadiusIndex1`, `EffectRadiusIndex2`, `EffectRadiusIndex3`, `EffectApplyAuraName1`, `EffectApplyAuraName2`, `EffectApplyAuraName3`, `EffectAmplitude1`, `EffectAmplitude2`, `EffectAmplitude3`, `EffectMultipleValue1`, `EffectMultipleValue2`, `EffectMultipleValue3`, `EffectChainTarget1`, `EffectChainTarget2`, `EffectChainTarget3`, `EffectItemType1`, `EffectItemType2`, `EffectItemType3`, `EffectMiscValue1`, `EffectMiscValue2`, `EffectMiscValue3`, `EffectMiscValueB1`, `EffectMiscValueB2`, `EffectMiscValueB3`, `EffectTriggerSpell1`, `EffectTriggerSpell2`, `EffectTriggerSpell3`, `EffectPointsPerComboPoint1`, `EffectPointsPerComboPoint2`, `EffectPointsPerComboPoint3`, `EffectSpellClassMaskA1`, `EffectSpellClassMaskA2`, `EffectSpellClassMaskA3`, `EffectSpellClassMaskB1`, `EffectSpellClassMaskB2`, `EffectSpellClassMaskB3`, `EffectSpellClassMaskC1`, `EffectSpellClassMaskC2`, `EffectSpellClassMaskC3`, `SpellVisual1`, `SpellVisual2`, `SpellIconID`, `ActiveIconID`, `SpellPriority`, `SpellName0`, `SpellName1`, `SpellName2`, `SpellName3`, `SpellName4`, `SpellName5`, `SpellName6`, `SpellName7`, `SpellName8`, `SpellNameFlag0`, `SpellNameFlag1`, `SpellNameFlag2`, `SpellNameFlag3`, `SpellNameFlag4`, `SpellNameFlag5`, `SpellNameFlag6`, `SpellNameFlag7`, `SpellRank0`, `SpellRank1`, `SpellRank2`, `SpellRank3`, `SpellRank4`, `SpellRank5`, `SpellRank6`, `SpellRank7`, `SpellRank8`, `SpellRankFlags0`, `SpellRankFlags1`, `SpellRankFlags2`, `SpellRankFlags3`, `SpellRankFlags4`, `SpellRankFlags5`, `SpellRankFlags6`, `SpellRankFlags7`, `SpellDescription0`, `SpellDescription1`, `SpellDescription2`, `SpellDescription3`, `SpellDescription4`, `SpellDescription5`, `SpellDescription6`, `SpellDescription7`, `SpellDescription8`, `SpellDescriptionFlags0`, `SpellDescriptionFlags1`, `SpellDescriptionFlags2`, `SpellDescriptionFlags3`, `SpellDescriptionFlags4`, `SpellDescriptionFlags5`, `SpellDescriptionFlags6`, `SpellDescriptionFlags7`, `SpellToolTip0`, `SpellToolTip1`, `SpellToolTip2`, `SpellToolTip3`, `SpellToolTip4`, `SpellToolTip5`, `SpellToolTip6`, `SpellToolTip7`, `SpellToolTip8`, `SpellToolTipFlags0`, `SpellToolTipFlags1`, `SpellToolTipFlags2`, `SpellToolTipFlags3`, `SpellToolTipFlags4`, `SpellToolTipFlags5`, `SpellToolTipFlags6`, `SpellToolTipFlags7`, `ManaCostPercentage`, `StartRecoveryCategory`, `StartRecoveryTime`, `MaximumTargetLevel`, `SpellFamilyName`, `SpellFamilyFlags`, `SpellFamilyFlags1`, `SpellFamilyFlags2`, `MaximumAffectedTargets`, `DamageClass`, `PreventionType`, `StanceBarOrder`, `EffectDamageMultiplier1`, `EffectDamageMultiplier2`, `EffectDamageMultiplier3`, `MinimumFactionId`, `MinimumReputation`, `RequiredAuraVision`, `TotemCategory1`, `TotemCategory2`, `AreaGroupID`, `SchoolMask`, `RuneCostID`, `SpellMissileID`, `PowerDisplayId`, `EffectBonusMultiplier1`, `EffectBonusMultiplier2`, `EffectBonusMultiplier3`, `SpellDescriptionVariableID`, `SpellDifficultyID`) VALUES
(29834, 0, 0, 0, 464, 0, 0, 67108864, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 13, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1048576, 100, 0, 0, 0, 1, 21, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 6, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 42, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 29841, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1697, 0, 50, 'Second Wind', '', '', '', '', '', '', '', '', '', '', '', '', '', '', '', 16712190, 'Rank 1', '', '', '', '', '', '', '', '', '', '', '', '', '', '', '', 16712190, 'Whenever you are struck while below 35% health you generate $/10;29841o1 rage and $29841o2% of your total health over $29841d.', '', '', '', '', '', '', '', '', 0, 0, 0, 0, 0, 0, 0, 16712190, '', '', '', '', '', '', '', '', '', 0, 0, 0, 0, 0, 0, 0, 16712188, 0, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0),
(29838, 0, 0, 0, 464, 0, 0, 67108864, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 13, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1048576, 100, 0, 0, 0, 1, 21, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 6, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 42, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 29842, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1697, 0, 50, 'Second Wind', '', '', '', '', '', '', '', '', '', '', '', '', '', '', '', 16712190, 'Rank 2', '', '', '', '', '', '', '', '', '', '', '', '', '', '', '', 16712190, 'Whenever you are struck while below 35% health you generate $/10;29842o1 rage and $29842o2% of your total health over $29842d.', '', '', '', '', '', '', '', '', 0, 0, 0, 0, 0, 0, 0, 16712190, '', '', '', '', '', '', '', '', '', 0, 0, 0, 0, 0, 0, 0, 16712188, 0, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0);

-- ============================================================
-- 29841 / 29842: payload descriptions only
-- ============================================================
-- Mechanically identical to stock -- same auras, amplitudes, base points and
-- duration.  Only the prose changes, so it stops describing a stun/immobilise
-- trigger that no longer exists.  Their SpellToolTip0 is already correct and is
-- carried through unchanged.

DELETE FROM `alonecraft_spell_dbc` WHERE `ID` IN (29841, 29842);
INSERT INTO `alonecraft_spell_dbc` (`ID`, `Category`, `Dispel`, `Mechanic`, `Attributes`, `AttributesEx`, `AttributesEx2`, `AttributesEx3`, `AttributesEx4`, `AttributesEx5`, `AttributesEx6`, `AttributesEx7`, `Stances`, `Unknown1`, `StancesNot`, `Unknown2`, `Targets`, `TargetCreatureType`, `RequiresSpellFocus`, `FacingCasterFlags`, `CasterAuraState`, `TargetAuraState`, `CasterAuraStateNot`, `TargetAuraStateNot`, `CasterAuraSpell`, `TargetAuraSpell`, `ExcludeCasterAuraSpell`, `ExcludeTargetAuraSpell`, `CastingTimeIndex`, `RecoveryTime`, `CategoryRecoveryTime`, `InterruptFlags`, `AuraInterruptFlags`, `ChannelInterruptFlags`, `ProcFlags`, `ProcChance`, `ProcCharges`, `MaximumLevel`, `BaseLevel`, `SpellLevel`, `DurationIndex`, `PowerType`, `ManaCost`, `ManaCostPerLevel`, `ManaPerSecond`, `ManaPerSecondPerLevel`, `RangeIndex`, `Speed`, `ModalNextSpell`, `StackAmount`, `Totem1`, `Totem2`, `Reagent1`, `Reagent2`, `Reagent3`, `Reagent4`, `Reagent5`, `Reagent6`, `Reagent7`, `Reagent8`, `ReagentCount1`, `ReagentCount2`, `ReagentCount3`, `ReagentCount4`, `ReagentCount5`, `ReagentCount6`, `ReagentCount7`, `ReagentCount8`, `EquippedItemClass`, `EquippedItemSubClassMask`, `EquippedItemInventoryTypeMask`, `Effect1`, `Effect2`, `Effect3`, `EffectDieSides1`, `EffectDieSides2`, `EffectDieSides3`, `EffectRealPointsPerLevel1`, `EffectRealPointsPerLevel2`, `EffectRealPointsPerLevel3`, `EffectBasePoints1`, `EffectBasePoints2`, `EffectBasePoints3`, `EffectMechanic1`, `EffectMechanic2`, `EffectMechanic3`, `EffectImplicitTargetA1`, `EffectImplicitTargetA2`, `EffectImplicitTargetA3`, `EffectImplicitTargetB1`, `EffectImplicitTargetB2`, `EffectImplicitTargetB3`, `EffectRadiusIndex1`, `EffectRadiusIndex2`, `EffectRadiusIndex3`, `EffectApplyAuraName1`, `EffectApplyAuraName2`, `EffectApplyAuraName3`, `EffectAmplitude1`, `EffectAmplitude2`, `EffectAmplitude3`, `EffectMultipleValue1`, `EffectMultipleValue2`, `EffectMultipleValue3`, `EffectChainTarget1`, `EffectChainTarget2`, `EffectChainTarget3`, `EffectItemType1`, `EffectItemType2`, `EffectItemType3`, `EffectMiscValue1`, `EffectMiscValue2`, `EffectMiscValue3`, `EffectMiscValueB1`, `EffectMiscValueB2`, `EffectMiscValueB3`, `EffectTriggerSpell1`, `EffectTriggerSpell2`, `EffectTriggerSpell3`, `EffectPointsPerComboPoint1`, `EffectPointsPerComboPoint2`, `EffectPointsPerComboPoint3`, `EffectSpellClassMaskA1`, `EffectSpellClassMaskA2`, `EffectSpellClassMaskA3`, `EffectSpellClassMaskB1`, `EffectSpellClassMaskB2`, `EffectSpellClassMaskB3`, `EffectSpellClassMaskC1`, `EffectSpellClassMaskC2`, `EffectSpellClassMaskC3`, `SpellVisual1`, `SpellVisual2`, `SpellIconID`, `ActiveIconID`, `SpellPriority`, `SpellName0`, `SpellName1`, `SpellName2`, `SpellName3`, `SpellName4`, `SpellName5`, `SpellName6`, `SpellName7`, `SpellName8`, `SpellNameFlag0`, `SpellNameFlag1`, `SpellNameFlag2`, `SpellNameFlag3`, `SpellNameFlag4`, `SpellNameFlag5`, `SpellNameFlag6`, `SpellNameFlag7`, `SpellRank0`, `SpellRank1`, `SpellRank2`, `SpellRank3`, `SpellRank4`, `SpellRank5`, `SpellRank6`, `SpellRank7`, `SpellRank8`, `SpellRankFlags0`, `SpellRankFlags1`, `SpellRankFlags2`, `SpellRankFlags3`, `SpellRankFlags4`, `SpellRankFlags5`, `SpellRankFlags6`, `SpellRankFlags7`, `SpellDescription0`, `SpellDescription1`, `SpellDescription2`, `SpellDescription3`, `SpellDescription4`, `SpellDescription5`, `SpellDescription6`, `SpellDescription7`, `SpellDescription8`, `SpellDescriptionFlags0`, `SpellDescriptionFlags1`, `SpellDescriptionFlags2`, `SpellDescriptionFlags3`, `SpellDescriptionFlags4`, `SpellDescriptionFlags5`, `SpellDescriptionFlags6`, `SpellDescriptionFlags7`, `SpellToolTip0`, `SpellToolTip1`, `SpellToolTip2`, `SpellToolTip3`, `SpellToolTip4`, `SpellToolTip5`, `SpellToolTip6`, `SpellToolTip7`, `SpellToolTip8`, `SpellToolTipFlags0`, `SpellToolTipFlags1`, `SpellToolTipFlags2`, `SpellToolTipFlags3`, `SpellToolTipFlags4`, `SpellToolTipFlags5`, `SpellToolTipFlags6`, `SpellToolTipFlags7`, `ManaCostPercentage`, `StartRecoveryCategory`, `StartRecoveryTime`, `MaximumTargetLevel`, `SpellFamilyName`, `SpellFamilyFlags`, `SpellFamilyFlags1`, `SpellFamilyFlags2`, `MaximumAffectedTargets`, `DamageClass`, `PreventionType`, `StanceBarOrder`, `EffectDamageMultiplier1`, `EffectDamageMultiplier2`, `EffectDamageMultiplier3`, `MinimumFactionId`, `MinimumReputation`, `RequiredAuraVision`, `TotemCategory1`, `TotemCategory2`, `AreaGroupID`, `SchoolMask`, `RuneCostID`, `SpellMissileID`, `PowerDisplayId`, `EffectBonusMultiplier1`, `EffectBonusMultiplier2`, `EffectBonusMultiplier3`, `SpellDescriptionVariableID`, `SpellDifficultyID`) VALUES
(29841, 0, 0, 0, 16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 101, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 6, 6, 0, 1, 1, 0, 0, 0, 0, 19, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 24, 20, 0, 2000, 2000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8208, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1697, 0, 50, 'Second Wind', '', '', '', '', '', '', '', '', '', '', '', '', '', '', '', 16712190, 'Rank 1', '', '', '', '', '', '', '', '', '', '', '', '', '', '', '', 16712190, 'Whenever you are struck while below 35% health you generate $/10;29841o1 rage and $29841o2% of your total health over $29841d.', '', '', '', '', '', '', '', '', 0, 0, 0, 0, 0, 0, 0, 16712190, 'Generates $/10;s1 rage and heals $s2% of your total health every $t1 sec.', '', '', '', '', '', '', '', '', 0, 0, 0, 0, 0, 0, 0, 16712190, 0, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0),
(29842, 0, 0, 0, 16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 101, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 6, 6, 0, 1, 1, 0, 0, 0, 0, 39, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 24, 20, 0, 2000, 2000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8208, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1697, 0, 50, 'Second Wind', '', '', '', '', '', '', '', '', '', '', '', '', '', '', '', 16712190, 'Rank 2', '', '', '', '', '', '', '', '', '', '', '', '', '', '', '', 16712190, 'Whenever you are struck while below 35% health you generate $/10;29842o1 rage and $29842o2% of your total health over $29842d.', '', '', '', '', '', '', '', '', 0, 0, 0, 0, 0, 0, 0, 16712190, 'Generates $/10;s1 rage and heals $s2% of your total health every $t1 sec.', '', '', '', '', '', '', '', '', 0, 0, 0, 0, 0, 0, 0, 16712190, 0, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0);
