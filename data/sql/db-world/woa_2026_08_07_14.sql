-- ============================================================
-- Warrior (Arms): Weapon Mastery
-- ============================================================
-- TODO.md: "Weapon Mastery (0, 5): Reduces the chance for your attacks to be
--           dodged by 2/4% and while using a two-handed weapon your parry
--           chance is increased by your strength. (2 ranks) (Same effect as DK
--           strength -> parry conversion)"
--
-- Two changes, plus a normalisation.
--
-- 1. Dodge reduction 1/2% -> 2/4%.  Effect 1 is SPELL_AURA_MOD_COMBAT_RESULT_CHANCE
--    (248) with EffectMiscValue 2 (VICTIMSTATE_DODGE), NOT MOD_ENEMY_DODGE
--    (251) -- and it is added FLAT, not as a multiplier:
--    Unit.cpp:3046 and :3466 do dodge_chance += amount * 100, where
--    dodge_chance is in hundredths of a percent.  So base points -3 / -5 give
--    2% / 4%.
--
-- 2. Strength -> parry, on effect 3, which was empty.
--    Prior art: Forceful Deflection 49410, the Death Knight passive this cites.
--    SPELL_AURA_MOD_RATING_FROM_STAT (220), EffectMiscValue = 8 = 1 << CR_PARRY
--    (CR_PARRY is 3), EffectMiscValueB = 0 = STAT_STRENGTH, amount = percent of
--    that stat.  The DK converts 25%; this scales 12% / 25% per rank, so rank 1
--    is worth buying.  (12.5 is not representable -- base points are integers.)
--
-- 3. SpellFamilyName: rank 1 is 4 and rank 2 is 0 in stock, which is a retail
--    inconsistency rather than a deliberate difference.  Both are set to 4.
--
-- Why the two-handed gate is in C++ and not in the DBC, which is the only
-- awkward part of this file:
--
--   EquippedItemClass is a WHOLE-SPELL gate, and the dodge reduction must stay
--   unconditional -- so it cannot go on this spell.
--
--   Splitting the second half into its own item-gated passive does not work
--   either.  spell_linked_spell type 2 would apply it, but core only ever
--   re-adds item-dependent passives by walking the spell map and the talent map
--   (Player::ApplyItemDependentAuras, Player.cpp:7141-7164).  A linked spell is
--   in neither, so it would be removed on unequip and never come back on
--   re-equip -- silent, one-way breakage.
--
--   Talent.dbc has one spell per rank, so the talent cannot simply teach two.
--
-- So the gate lives in the amount calculation, in the same file and the same
-- idiom as the Two-Handed Weapon Specialization conversion.
--
-- Spells:
--   20504 / 20505 = Weapon Mastery ranks 1-2
-- ============================================================

DELETE FROM `alonecraft_spell_dbc` WHERE `ID` IN (20504, 20505);
INSERT INTO `alonecraft_spell_dbc` (`ID`, `Category`, `Dispel`, `Mechanic`, `Attributes`, `AttributesEx`, `AttributesEx2`, `AttributesEx3`, `AttributesEx4`, `AttributesEx5`, `AttributesEx6`, `AttributesEx7`, `Stances`, `Unknown1`, `StancesNot`, `Unknown2`, `Targets`, `TargetCreatureType`, `RequiresSpellFocus`, `FacingCasterFlags`, `CasterAuraState`, `TargetAuraState`, `CasterAuraStateNot`, `TargetAuraStateNot`, `CasterAuraSpell`, `TargetAuraSpell`, `ExcludeCasterAuraSpell`, `ExcludeTargetAuraSpell`, `CastingTimeIndex`, `RecoveryTime`, `CategoryRecoveryTime`, `InterruptFlags`, `AuraInterruptFlags`, `ChannelInterruptFlags`, `ProcFlags`, `ProcChance`, `ProcCharges`, `MaximumLevel`, `BaseLevel`, `SpellLevel`, `DurationIndex`, `PowerType`, `ManaCost`, `ManaCostPerLevel`, `ManaPerSecond`, `ManaPerSecondPerLevel`, `RangeIndex`, `Speed`, `ModalNextSpell`, `StackAmount`, `Totem1`, `Totem2`, `Reagent1`, `Reagent2`, `Reagent3`, `Reagent4`, `Reagent5`, `Reagent6`, `Reagent7`, `Reagent8`, `ReagentCount1`, `ReagentCount2`, `ReagentCount3`, `ReagentCount4`, `ReagentCount5`, `ReagentCount6`, `ReagentCount7`, `ReagentCount8`, `EquippedItemClass`, `EquippedItemSubClassMask`, `EquippedItemInventoryTypeMask`, `Effect1`, `Effect2`, `Effect3`, `EffectDieSides1`, `EffectDieSides2`, `EffectDieSides3`, `EffectRealPointsPerLevel1`, `EffectRealPointsPerLevel2`, `EffectRealPointsPerLevel3`, `EffectBasePoints1`, `EffectBasePoints2`, `EffectBasePoints3`, `EffectMechanic1`, `EffectMechanic2`, `EffectMechanic3`, `EffectImplicitTargetA1`, `EffectImplicitTargetA2`, `EffectImplicitTargetA3`, `EffectImplicitTargetB1`, `EffectImplicitTargetB2`, `EffectImplicitTargetB3`, `EffectRadiusIndex1`, `EffectRadiusIndex2`, `EffectRadiusIndex3`, `EffectApplyAuraName1`, `EffectApplyAuraName2`, `EffectApplyAuraName3`, `EffectAmplitude1`, `EffectAmplitude2`, `EffectAmplitude3`, `EffectMultipleValue1`, `EffectMultipleValue2`, `EffectMultipleValue3`, `EffectChainTarget1`, `EffectChainTarget2`, `EffectChainTarget3`, `EffectItemType1`, `EffectItemType2`, `EffectItemType3`, `EffectMiscValue1`, `EffectMiscValue2`, `EffectMiscValue3`, `EffectMiscValueB1`, `EffectMiscValueB2`, `EffectMiscValueB3`, `EffectTriggerSpell1`, `EffectTriggerSpell2`, `EffectTriggerSpell3`, `EffectPointsPerComboPoint1`, `EffectPointsPerComboPoint2`, `EffectPointsPerComboPoint3`, `EffectSpellClassMaskA1`, `EffectSpellClassMaskA2`, `EffectSpellClassMaskA3`, `EffectSpellClassMaskB1`, `EffectSpellClassMaskB2`, `EffectSpellClassMaskB3`, `EffectSpellClassMaskC1`, `EffectSpellClassMaskC2`, `EffectSpellClassMaskC3`, `SpellVisual1`, `SpellVisual2`, `SpellIconID`, `ActiveIconID`, `SpellPriority`, `SpellName0`, `SpellName1`, `SpellName2`, `SpellName3`, `SpellName4`, `SpellName5`, `SpellName6`, `SpellName7`, `SpellName8`, `SpellNameFlag0`, `SpellNameFlag1`, `SpellNameFlag2`, `SpellNameFlag3`, `SpellNameFlag4`, `SpellNameFlag5`, `SpellNameFlag6`, `SpellNameFlag7`, `SpellRank0`, `SpellRank1`, `SpellRank2`, `SpellRank3`, `SpellRank4`, `SpellRank5`, `SpellRank6`, `SpellRank7`, `SpellRank8`, `SpellRankFlags0`, `SpellRankFlags1`, `SpellRankFlags2`, `SpellRankFlags3`, `SpellRankFlags4`, `SpellRankFlags5`, `SpellRankFlags6`, `SpellRankFlags7`, `SpellDescription0`, `SpellDescription1`, `SpellDescription2`, `SpellDescription3`, `SpellDescription4`, `SpellDescription5`, `SpellDescription6`, `SpellDescription7`, `SpellDescription8`, `SpellDescriptionFlags0`, `SpellDescriptionFlags1`, `SpellDescriptionFlags2`, `SpellDescriptionFlags3`, `SpellDescriptionFlags4`, `SpellDescriptionFlags5`, `SpellDescriptionFlags6`, `SpellDescriptionFlags7`, `SpellToolTip0`, `SpellToolTip1`, `SpellToolTip2`, `SpellToolTip3`, `SpellToolTip4`, `SpellToolTip5`, `SpellToolTip6`, `SpellToolTip7`, `SpellToolTip8`, `SpellToolTipFlags0`, `SpellToolTipFlags1`, `SpellToolTipFlags2`, `SpellToolTipFlags3`, `SpellToolTipFlags4`, `SpellToolTipFlags5`, `SpellToolTipFlags6`, `SpellToolTipFlags7`, `ManaCostPercentage`, `StartRecoveryCategory`, `StartRecoveryTime`, `MaximumTargetLevel`, `SpellFamilyName`, `SpellFamilyFlags`, `SpellFamilyFlags1`, `SpellFamilyFlags2`, `MaximumAffectedTargets`, `DamageClass`, `PreventionType`, `StanceBarOrder`, `EffectDamageMultiplier1`, `EffectDamageMultiplier2`, `EffectDamageMultiplier3`, `MinimumFactionId`, `MinimumReputation`, `RequiredAuraVision`, `TotemCategory1`, `TotemCategory2`, `AreaGroupID`, `SchoolMask`, `RuneCostID`, `SpellMissileID`, `PowerDisplayId`, `EffectBonusMultiplier1`, `EffectBonusMultiplier2`, `EffectBonusMultiplier3`, `SpellDescriptionVariableID`, `SpellDifficultyID`) VALUES
(20504, 0, 0, 0, 464, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 101, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 6, 6, 6, 1, 1, 1, 0, 0, 0, -3, -26, 11, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 248, 234, 220, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 3, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1976, 0, 0, 'Weapon Mastery', '', '', '', '', '', '', '', '', '', '', '', '', '', '', '', 16712190, 'Rank 1', '', '', '', '', '', '', '', '', '', '', '', '', '', '', '', 16712190, 'Reduces the chance for your attacks to be dodged by $s1% and reduces the duration of all Disarm effects by $s2%.  While a two-handed weapon is equipped, your parry rating is increased by $s3% of your Strength.', '', '', '', '', '', '', '', '', 0, 0, 0, 0, 0, 0, 0, 16712190, '', '', '', '', '', '', '', '', '', 0, 0, 0, 0, 0, 0, 0, 16712188, 0, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 0, 0),
(20505, 0, 0, 0, 464, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 101, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 6, 6, 6, 1, 1, 1, 0, 0, 0, -5, -51, 24, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 248, 234, 220, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 3, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1976, 0, 50, 'Weapon Mastery', '', '', '', '', '', '', '', '', '', '', '', '', '', '', '', 16712190, 'Rank 2', '', '', '', '', '', '', '', '', '', '', '', '', '', '', '', 16712190, 'Reduces the chance for your attacks to be dodged by $s1% and reduces the duration of all Disarm effects by $s2%.  While a two-handed weapon is equipped, your parry rating is increased by $s3% of your Strength.', '', '', '', '', '', '', '', '', 0, 0, 0, 0, 0, 0, 0, 16712190, '', '', '', '', '', '', '', '', '', 0, 0, 0, 0, 0, 0, 0, 16712188, 0, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0);

-- ============================================================
-- Script registration
-- ============================================================

DELETE FROM `spell_script_names` WHERE `ScriptName` = 'spell_warr_weapon_mastery_parry';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(-20504, 'spell_warr_weapon_mastery_parry');
