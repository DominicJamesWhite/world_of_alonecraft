-- ============================================================
-- Warrior (Protection): rage income trim
-- ============================================================
-- Protection's rage income was audited in docs/prot_warrior_rage_audit.md and
-- came out at roughly 24 rage/sec single-target against a spend of about 23 --
-- and the spend only reaches 23 if Heroic Strike is on cooldown permanently and
-- Barricade dumps its full 80 every 10 seconds.  Anything less and you cap.
--
-- The three numbers trimmed here, and why these three:
--
--   Shield Slam   20 -> 15 rage   (~-3.4 rage/sec)
--   Revenge       15 -> 10 rage   (~-1.0 rage/sec)
--   Shield Spec    5 ->  3 rage   (~-0.6 single target, ~-1.8 on three)
--
-- Shield Slam is over half of all income once Sword and Board is factored in,
-- because SnB both refreshes its cooldown and doubles the energize
-- (SPELLMOD_EFFECT3 +100% on 50227), so every point cut here is cut roughly
-- twice.  Revenge is the second generator and moves with it.  Shield
-- Specialization is the one term that scales with the number of things hitting
-- you, so it is the multi-target correction.
--
-- Not touched deliberately: Rage.Normalized.Multiplier.  It is the only global
-- dial, but it feeds Unit::RewardRage, which these three generators never go
-- through (they are SPELL_EFFECT_ENERGIZE).  Turning it down would tax Arms,
-- Fury and bears -- all of which are tuned -- and leave Protection's actual
-- income sources untouched.
--
-- No tooltip edits anywhere, and none are needed.  All three numbers are read
-- from the base points changed below: Shield Slam's description says
-- `$/10;s3`, Revenge's `$/10;s2`, and all five Shield Specialization ranks say
-- `$/10;23602s1` -- a cross-spell reference to the trigger, which is why one
-- row retunes the whole talent.  None of the three is an aura, so SpellToolTip0
-- is empty on all of them and stays that way.
--
-- Shield Specialization gets a full-row INSERT because 23602 has never had an
-- alonecraft_spell_dbc row (it was retail-stock until now).  Shield Slam and
-- Revenge get UPDATEs, not re-INSERTs: their rows already exist from
-- woa_2026_08_08_03.sql and a 234-column re-INSERT would silently revert every
-- other column to whatever the generator believed.
--
-- Originally written as woa_2026_08_09_32.sql; that filename was concurrently
-- claimed by the SkillLineAbility work, so this is the same change re-landed.
-- ============================================================

-- ------------------------------------------------------------
-- Shield Specialization: 5 rage -> 3 rage per block/dodge/parry
--   23602 is the trigger spell shared by all five ranks (12298, 12724-12727),
--   so one row covers the talent.
--   Overrides: EffectBasePoints1: 49 -> 29
-- ------------------------------------------------------------

DELETE FROM `alonecraft_spell_dbc` WHERE `ID` = 23602;
INSERT INTO `alonecraft_spell_dbc` (`ID`, `Category`, `Dispel`, `Mechanic`, `Attributes`, `AttributesEx`, `AttributesEx2`, `AttributesEx3`, `AttributesEx4`, `AttributesEx5`, `AttributesEx6`, `AttributesEx7`, `Stances`, `Unknown1`, `StancesNot`, `Unknown2`, `Targets`, `TargetCreatureType`, `RequiresSpellFocus`, `FacingCasterFlags`, `CasterAuraState`, `TargetAuraState`, `CasterAuraStateNot`, `TargetAuraStateNot`, `CasterAuraSpell`, `TargetAuraSpell`, `ExcludeCasterAuraSpell`, `ExcludeTargetAuraSpell`, `CastingTimeIndex`, `RecoveryTime`, `CategoryRecoveryTime`, `InterruptFlags`, `AuraInterruptFlags`, `ChannelInterruptFlags`, `ProcFlags`, `ProcChance`, `ProcCharges`, `MaximumLevel`, `BaseLevel`, `SpellLevel`, `DurationIndex`, `PowerType`, `ManaCost`, `ManaCostPerLevel`, `ManaPerSecond`, `ManaPerSecondPerLevel`, `RangeIndex`, `Speed`, `ModalNextSpell`, `StackAmount`, `Totem1`, `Totem2`, `Reagent1`, `Reagent2`, `Reagent3`, `Reagent4`, `Reagent5`, `Reagent6`, `Reagent7`, `Reagent8`, `ReagentCount1`, `ReagentCount2`, `ReagentCount3`, `ReagentCount4`, `ReagentCount5`, `ReagentCount6`, `ReagentCount7`, `ReagentCount8`, `EquippedItemClass`, `EquippedItemSubClassMask`, `EquippedItemInventoryTypeMask`, `Effect1`, `Effect2`, `Effect3`, `EffectDieSides1`, `EffectDieSides2`, `EffectDieSides3`, `EffectRealPointsPerLevel1`, `EffectRealPointsPerLevel2`, `EffectRealPointsPerLevel3`, `EffectBasePoints1`, `EffectBasePoints2`, `EffectBasePoints3`, `EffectMechanic1`, `EffectMechanic2`, `EffectMechanic3`, `EffectImplicitTargetA1`, `EffectImplicitTargetA2`, `EffectImplicitTargetA3`, `EffectImplicitTargetB1`, `EffectImplicitTargetB2`, `EffectImplicitTargetB3`, `EffectRadiusIndex1`, `EffectRadiusIndex2`, `EffectRadiusIndex3`, `EffectApplyAuraName1`, `EffectApplyAuraName2`, `EffectApplyAuraName3`, `EffectAmplitude1`, `EffectAmplitude2`, `EffectAmplitude3`, `EffectMultipleValue1`, `EffectMultipleValue2`, `EffectMultipleValue3`, `EffectChainTarget1`, `EffectChainTarget2`, `EffectChainTarget3`, `EffectItemType1`, `EffectItemType2`, `EffectItemType3`, `EffectMiscValue1`, `EffectMiscValue2`, `EffectMiscValue3`, `EffectMiscValueB1`, `EffectMiscValueB2`, `EffectMiscValueB3`, `EffectTriggerSpell1`, `EffectTriggerSpell2`, `EffectTriggerSpell3`, `EffectPointsPerComboPoint1`, `EffectPointsPerComboPoint2`, `EffectPointsPerComboPoint3`, `EffectSpellClassMaskA1`, `EffectSpellClassMaskA2`, `EffectSpellClassMaskA3`, `EffectSpellClassMaskB1`, `EffectSpellClassMaskB2`, `EffectSpellClassMaskB3`, `EffectSpellClassMaskC1`, `EffectSpellClassMaskC2`, `EffectSpellClassMaskC3`, `SpellVisual1`, `SpellVisual2`, `SpellIconID`, `ActiveIconID`, `SpellPriority`, `SpellName0`, `SpellName1`, `SpellName2`, `SpellName3`, `SpellName4`, `SpellName5`, `SpellName6`, `SpellName7`, `SpellName8`, `SpellNameFlag0`, `SpellNameFlag1`, `SpellNameFlag2`, `SpellNameFlag3`, `SpellNameFlag4`, `SpellNameFlag5`, `SpellNameFlag6`, `SpellNameFlag7`, `SpellRank0`, `SpellRank1`, `SpellRank2`, `SpellRank3`, `SpellRank4`, `SpellRank5`, `SpellRank6`, `SpellRank7`, `SpellRank8`, `SpellRankFlags0`, `SpellRankFlags1`, `SpellRankFlags2`, `SpellRankFlags3`, `SpellRankFlags4`, `SpellRankFlags5`, `SpellRankFlags6`, `SpellRankFlags7`, `SpellDescription0`, `SpellDescription1`, `SpellDescription2`, `SpellDescription3`, `SpellDescription4`, `SpellDescription5`, `SpellDescription6`, `SpellDescription7`, `SpellDescription8`, `SpellDescriptionFlags0`, `SpellDescriptionFlags1`, `SpellDescriptionFlags2`, `SpellDescriptionFlags3`, `SpellDescriptionFlags4`, `SpellDescriptionFlags5`, `SpellDescriptionFlags6`, `SpellDescriptionFlags7`, `SpellToolTip0`, `SpellToolTip1`, `SpellToolTip2`, `SpellToolTip3`, `SpellToolTip4`, `SpellToolTip5`, `SpellToolTip6`, `SpellToolTip7`, `SpellToolTip8`, `SpellToolTipFlags0`, `SpellToolTipFlags1`, `SpellToolTipFlags2`, `SpellToolTipFlags3`, `SpellToolTipFlags4`, `SpellToolTipFlags5`, `SpellToolTipFlags6`, `SpellToolTipFlags7`, `ManaCostPercentage`, `StartRecoveryCategory`, `StartRecoveryTime`, `MaximumTargetLevel`, `SpellFamilyName`, `SpellFamilyFlags`, `SpellFamilyFlags1`, `SpellFamilyFlags2`, `MaximumAffectedTargets`, `DamageClass`, `PreventionType`, `StanceBarOrder`, `EffectDamageMultiplier1`, `EffectDamageMultiplier2`, `EffectDamageMultiplier3`, `MinimumFactionId`, `MinimumReputation`, `RequiredAuraVision`, `TotemCategory1`, `TotemCategory2`, `AreaGroupID`, `SchoolMask`, `RuneCostID`, `SpellMissileID`, `PowerDisplayId`, `EffectBonusMultiplier1`, `EffectBonusMultiplier2`, `EffectBonusMultiplier3`, `SpellDescriptionVariableID`, `SpellDifficultyID`) VALUES
(23602, 0, 0, 0, 262160, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 20, 100, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 30, 0, 0, 1, 0, 0, 0, 0, 0, 29, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 689, 0, 50, 'Shield Specialization', '', '', '', '', '', '', '', '', '', '', '', '', '', '', '', 16712190, 'Rank 1', '', '', '', '', '', '', '', '', '', '', '', '', '', '', '', 16712190, 'Increases your chance to block attacks with a shield and has a chance to generate $/10;s1 rage when a block occurs.', '', '', '', '', '', '', '', '', 0, 0, 0, 0, 0, 0, 0, 16712190, '', '', '', '', '', '', '', '', '', 0, 0, 0, 0, 0, 0, 0, 16712188, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0);

-- ------------------------------------------------------------
-- Shield Slam: generates 20 rage -> 15 rage (all 8 ranks)
--   Effect3 is the energize (woa_2026_08_08_03.sql).  Rage is stored at ten
--   times its displayed value and $/10; divides it back, so 149 base points
--   plus 1 die side = 150 = 15 rage.
-- ------------------------------------------------------------
UPDATE `alonecraft_spell_dbc`
   SET `EffectBasePoints3` = 149
 WHERE `ID` IN (23922, 23923, 23924, 23925, 25258, 30356, 47487, 47488)
   AND `Effect3` = 30;

-- ------------------------------------------------------------
-- Revenge: generates 15 rage -> 10 rage (all 9 ranks)
--   Effect2 here, not Effect3 -- which is exactly why no single SPELLMOD can
--   reach both abilities' rage, and why this is two statements.
-- ------------------------------------------------------------
UPDATE `alonecraft_spell_dbc`
   SET `EffectBasePoints2` = 99
 WHERE `ID` IN (6572, 6574, 7379, 11600, 11601, 25288, 25269, 30357, 57823)
   AND `Effect2` = 30;
