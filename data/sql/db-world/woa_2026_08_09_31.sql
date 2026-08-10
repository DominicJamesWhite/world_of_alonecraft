-- ============================================================
-- Warrior (Protection): Barricade redesigned as a rage spender
-- ============================================================
-- Supersedes the toggle shipped in woa_2026_08_09_27.sql.  That version was a
-- permanent stance with a 5 rage per second upkeep that suppressed Shield Slam
-- and Revenge rage; it worked, but a drip cost never reads as *spending*
-- anything, so the surplus rage it existed to solve never felt consumed.
--
-- TODO.md: "Become a living barricade, increasing your shield block chance and
--           value by 20% and converting each extra point of rage into 1
--           additional percent (up to a maximum cost of 80 rage).  When you
--           block with Barricade active, you heal for your block value."
--
-- It is now a second Shield Block that scales with what you dump into it:
-- 10 second duration, 10 second cooldown, 20 rage minimum for 20%, and every
-- further point of rage buys another point of block chance and block value up
-- to 80 rage for 80%.  Because the floor is 20 rage for 20%, the conversion is
-- a flat 1% per rage across the whole range with no special case -- the
-- percentage is simply the rage spent divided by ten.
--
-- Spells:
--   200651 = Barricade, the ability (talent 152 rank 1, unchanged)
--   200652 = Barricade, the buff -- block chance, block value, heal-on-block
--   200653 = Barricade, the heal payload
--
-- The talent row is untouched: 152 already points at 200651.
--
-- Three spells rather than one because the magnitude is not known until cast
-- time.  Execute is the precedent for the whole shape -- see
-- spell_warr_execute (spell_warrior.cpp): the ability is a DUMMY effect whose
-- script reads the rage remaining *after* core has taken the base cost,
-- consumes what it needs up to the cap, and then fires the real payload with
-- CastCustomSpell.  A single self-buff cannot do that, because DBC base points
-- are static.
--
-- Consequence worth knowing: 200652's base points are 0 and the real values
-- arrive from C++ at apply time, so its tooltip is deliberately written in
-- plain prose, and 200651's description hardcodes the 20% floor.  Variables
-- only work when the DBC carries the number: `$s1` on 200652 would render 0%,
-- and on 200651's DUMMY effect it would render 1%.
-- ============================================================

-- ============================================================
-- 200651: the ability
-- ============================================================
-- Cloned from Shield Block 2565, which is exactly the archetype: instant,
-- self, rage, shield required.  Donor fixups:
--   Stances 131072 -> 0.  Shield Block is flagged Defensive Stance only.  A
--     Protection warrior lives there anyway, but a greyed-out button after
--     stance-dancing for Charge is the Victory Rush failure (TODO.md 4.66) and
--     there is no design reason to gate a rage dump by stance.
--   EquippedItemClass 4 / SubClassMask 64 is KEPT this time.  Barricade is
--     block chance and block value; without a shield it would do nothing at
--     all, so failing the cast is the honest outcome.
--   DurationIndex 1 -> 0, Effect1 APPLY_AURA -> DUMMY, Effect2 cleared.  The
--     ability itself no longer carries the auras; 200652 does.
--   RecoveryTime 60000 -> 10000 to match the duration.
--
-- ManaCost 200 is the 20 rage floor (rage is stored x10).  Core enforces it,
-- which is what stops the spell being cast for a 2% buff.
--
-- StartRecoveryCategory / StartRecoveryTime stay 0 -- off the GCD, as with the
-- previous version.  Shield Block itself is on the GCD, but a tank spender
-- that costs a Shield Slam to press is a spender nobody presses.
--
-- The description hardcodes "20%" rather than using `$s1`.  The floor is a
-- property of the spell's *cost*, not of any effect, and `$s1` here would
-- resolve against the DUMMY's base points (0) and print "1%".

DELETE FROM `alonecraft_spell_dbc` WHERE `ID` = 200651;
INSERT INTO `alonecraft_spell_dbc` (`ID`, `Category`, `Dispel`, `Mechanic`, `Attributes`, `AttributesEx`, `AttributesEx2`, `AttributesEx3`, `AttributesEx4`, `AttributesEx5`, `AttributesEx6`, `AttributesEx7`, `Stances`, `Unknown1`, `StancesNot`, `Unknown2`, `Targets`, `TargetCreatureType`, `RequiresSpellFocus`, `FacingCasterFlags`, `CasterAuraState`, `TargetAuraState`, `CasterAuraStateNot`, `TargetAuraStateNot`, `CasterAuraSpell`, `TargetAuraSpell`, `ExcludeCasterAuraSpell`, `ExcludeTargetAuraSpell`, `CastingTimeIndex`, `RecoveryTime`, `CategoryRecoveryTime`, `InterruptFlags`, `AuraInterruptFlags`, `ChannelInterruptFlags`, `ProcFlags`, `ProcChance`, `ProcCharges`, `MaximumLevel`, `BaseLevel`, `SpellLevel`, `DurationIndex`, `PowerType`, `ManaCost`, `ManaCostPerLevel`, `ManaPerSecond`, `ManaPerSecondPerLevel`, `RangeIndex`, `Speed`, `ModalNextSpell`, `StackAmount`, `Totem1`, `Totem2`, `Reagent1`, `Reagent2`, `Reagent3`, `Reagent4`, `Reagent5`, `Reagent6`, `Reagent7`, `Reagent8`, `ReagentCount1`, `ReagentCount2`, `ReagentCount3`, `ReagentCount4`, `ReagentCount5`, `ReagentCount6`, `ReagentCount7`, `ReagentCount8`, `EquippedItemClass`, `EquippedItemSubClassMask`, `EquippedItemInventoryTypeMask`, `Effect1`, `Effect2`, `Effect3`, `EffectDieSides1`, `EffectDieSides2`, `EffectDieSides3`, `EffectRealPointsPerLevel1`, `EffectRealPointsPerLevel2`, `EffectRealPointsPerLevel3`, `EffectBasePoints1`, `EffectBasePoints2`, `EffectBasePoints3`, `EffectMechanic1`, `EffectMechanic2`, `EffectMechanic3`, `EffectImplicitTargetA1`, `EffectImplicitTargetA2`, `EffectImplicitTargetA3`, `EffectImplicitTargetB1`, `EffectImplicitTargetB2`, `EffectImplicitTargetB3`, `EffectRadiusIndex1`, `EffectRadiusIndex2`, `EffectRadiusIndex3`, `EffectApplyAuraName1`, `EffectApplyAuraName2`, `EffectApplyAuraName3`, `EffectAmplitude1`, `EffectAmplitude2`, `EffectAmplitude3`, `EffectMultipleValue1`, `EffectMultipleValue2`, `EffectMultipleValue3`, `EffectChainTarget1`, `EffectChainTarget2`, `EffectChainTarget3`, `EffectItemType1`, `EffectItemType2`, `EffectItemType3`, `EffectMiscValue1`, `EffectMiscValue2`, `EffectMiscValue3`, `EffectMiscValueB1`, `EffectMiscValueB2`, `EffectMiscValueB3`, `EffectTriggerSpell1`, `EffectTriggerSpell2`, `EffectTriggerSpell3`, `EffectPointsPerComboPoint1`, `EffectPointsPerComboPoint2`, `EffectPointsPerComboPoint3`, `EffectSpellClassMaskA1`, `EffectSpellClassMaskA2`, `EffectSpellClassMaskA3`, `EffectSpellClassMaskB1`, `EffectSpellClassMaskB2`, `EffectSpellClassMaskB3`, `EffectSpellClassMaskC1`, `EffectSpellClassMaskC2`, `EffectSpellClassMaskC3`, `SpellVisual1`, `SpellVisual2`, `SpellIconID`, `ActiveIconID`, `SpellPriority`, `SpellName0`, `SpellName1`, `SpellName2`, `SpellName3`, `SpellName4`, `SpellName5`, `SpellName6`, `SpellName7`, `SpellName8`, `SpellNameFlag0`, `SpellNameFlag1`, `SpellNameFlag2`, `SpellNameFlag3`, `SpellNameFlag4`, `SpellNameFlag5`, `SpellNameFlag6`, `SpellNameFlag7`, `SpellRank0`, `SpellRank1`, `SpellRank2`, `SpellRank3`, `SpellRank4`, `SpellRank5`, `SpellRank6`, `SpellRank7`, `SpellRank8`, `SpellRankFlags0`, `SpellRankFlags1`, `SpellRankFlags2`, `SpellRankFlags3`, `SpellRankFlags4`, `SpellRankFlags5`, `SpellRankFlags6`, `SpellRankFlags7`, `SpellDescription0`, `SpellDescription1`, `SpellDescription2`, `SpellDescription3`, `SpellDescription4`, `SpellDescription5`, `SpellDescription6`, `SpellDescription7`, `SpellDescription8`, `SpellDescriptionFlags0`, `SpellDescriptionFlags1`, `SpellDescriptionFlags2`, `SpellDescriptionFlags3`, `SpellDescriptionFlags4`, `SpellDescriptionFlags5`, `SpellDescriptionFlags6`, `SpellDescriptionFlags7`, `SpellToolTip0`, `SpellToolTip1`, `SpellToolTip2`, `SpellToolTip3`, `SpellToolTip4`, `SpellToolTip5`, `SpellToolTip6`, `SpellToolTip7`, `SpellToolTip8`, `SpellToolTipFlags0`, `SpellToolTipFlags1`, `SpellToolTipFlags2`, `SpellToolTipFlags3`, `SpellToolTipFlags4`, `SpellToolTipFlags5`, `SpellToolTipFlags6`, `SpellToolTipFlags7`, `ManaCostPercentage`, `StartRecoveryCategory`, `StartRecoveryTime`, `MaximumTargetLevel`, `SpellFamilyName`, `SpellFamilyFlags`, `SpellFamilyFlags1`, `SpellFamilyFlags2`, `MaximumAffectedTargets`, `DamageClass`, `PreventionType`, `StanceBarOrder`, `EffectDamageMultiplier1`, `EffectDamageMultiplier2`, `EffectDamageMultiplier3`, `MinimumFactionId`, `MinimumReputation`, `RequiredAuraVision`, `TotemCategory1`, `TotemCategory2`, `AreaGroupID`, `SchoolMask`, `RuneCostID`, `SpellMissileID`, `PowerDisplayId`, `EffectBonusMultiplier1`, `EffectBonusMultiplier2`, `EffectBonusMultiplier3`, `SpellDescriptionVariableID`, `SpellDifficultyID`) VALUES
(200651, 0, 0, 0, 327696, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 10000, 0, 0, 0, 0, 0, 101, 0, 0, 30, 30, 0, 1, 200, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 64, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 512, 0, 3442, 0, 1941, 1941, 0, 'Barricade', '', '', '', '', '', '', '', '', '', '', '', '', '', '', '', 16712190, '', '', '', '', '', '', '', '', '', '', '', '', '', '', '', '', 16712188, 'Become a living barricade, increasing your shield block chance and value by 20% and converting each extra point of rage into 1 additional percent (up to a maximum cost of 80 rage).  When you block with Barricade active, you heal for your block value.', '', '', '', '', '', '', '', '', 0, 0, 0, 0, 0, 0, 0, 16712190, 'Block chance and block value increased by $s1%.', '', '', '', '', '', '', '', '', 0, 0, 0, 0, 0, 0, 0, 16712190, 0, 0, 0, 0, 4, 4096, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0);

-- ============================================================
-- 200652: the buff
-- ============================================================
-- Also cloned from Shield Block, which already carries the exact pair of auras
-- this needs -- MOD_BLOCK_PERCENT (51) and MOD_SHIELD_BLOCKVALUE_PCT (150) --
-- so only the amounts and a third effect had to change.
--
-- All three effects ship with EffectBasePoints 0 and EffectDieSides 0.  The
-- zero DieSides is load-bearing: SpellEffectInfo::CalcValue adds nothing when
-- DieSides is 0, so the value handed in by CastCustomSpell arrives intact.
-- With DieSides 1 every amount would silently be one point too high.
--
-- Effect3 is a bare SPELL_AURA_DUMMY carrying nothing.  It exists only to give
-- the heal-on-block proc something to hang off, which is the standard shape --
-- an aura needs an effect for OnEffectProc to bind to.
--
-- EquippedItemClass is cleared here even though the ability requires a shield.
-- The gate belongs on the cast, not on the buff; leaving the restriction on an
-- aura risks it behaving oddly if the shield is swapped mid-duration.
--
-- The block value bonus feeds the heal for free: Player::GetShieldBlockValue
-- (Player.cpp:5115) multiplies by m_auraBasePctMod[SHIELD_BLOCK_VALUE], which
-- is exactly what aura 150 modifies.  So an 80 rage Barricade heals 80% more
-- per block than a 20 rage one on both counts -- more blocks, bigger blocks.

DELETE FROM `alonecraft_spell_dbc` WHERE `ID` = 200652;
INSERT INTO `alonecraft_spell_dbc` (`ID`, `Category`, `Dispel`, `Mechanic`, `Attributes`, `AttributesEx`, `AttributesEx2`, `AttributesEx3`, `AttributesEx4`, `AttributesEx5`, `AttributesEx6`, `AttributesEx7`, `Stances`, `Unknown1`, `StancesNot`, `Unknown2`, `Targets`, `TargetCreatureType`, `RequiresSpellFocus`, `FacingCasterFlags`, `CasterAuraState`, `TargetAuraState`, `CasterAuraStateNot`, `TargetAuraStateNot`, `CasterAuraSpell`, `TargetAuraSpell`, `ExcludeCasterAuraSpell`, `ExcludeTargetAuraSpell`, `CastingTimeIndex`, `RecoveryTime`, `CategoryRecoveryTime`, `InterruptFlags`, `AuraInterruptFlags`, `ChannelInterruptFlags`, `ProcFlags`, `ProcChance`, `ProcCharges`, `MaximumLevel`, `BaseLevel`, `SpellLevel`, `DurationIndex`, `PowerType`, `ManaCost`, `ManaCostPerLevel`, `ManaPerSecond`, `ManaPerSecondPerLevel`, `RangeIndex`, `Speed`, `ModalNextSpell`, `StackAmount`, `Totem1`, `Totem2`, `Reagent1`, `Reagent2`, `Reagent3`, `Reagent4`, `Reagent5`, `Reagent6`, `Reagent7`, `Reagent8`, `ReagentCount1`, `ReagentCount2`, `ReagentCount3`, `ReagentCount4`, `ReagentCount5`, `ReagentCount6`, `ReagentCount7`, `ReagentCount8`, `EquippedItemClass`, `EquippedItemSubClassMask`, `EquippedItemInventoryTypeMask`, `Effect1`, `Effect2`, `Effect3`, `EffectDieSides1`, `EffectDieSides2`, `EffectDieSides3`, `EffectRealPointsPerLevel1`, `EffectRealPointsPerLevel2`, `EffectRealPointsPerLevel3`, `EffectBasePoints1`, `EffectBasePoints2`, `EffectBasePoints3`, `EffectMechanic1`, `EffectMechanic2`, `EffectMechanic3`, `EffectImplicitTargetA1`, `EffectImplicitTargetA2`, `EffectImplicitTargetA3`, `EffectImplicitTargetB1`, `EffectImplicitTargetB2`, `EffectImplicitTargetB3`, `EffectRadiusIndex1`, `EffectRadiusIndex2`, `EffectRadiusIndex3`, `EffectApplyAuraName1`, `EffectApplyAuraName2`, `EffectApplyAuraName3`, `EffectAmplitude1`, `EffectAmplitude2`, `EffectAmplitude3`, `EffectMultipleValue1`, `EffectMultipleValue2`, `EffectMultipleValue3`, `EffectChainTarget1`, `EffectChainTarget2`, `EffectChainTarget3`, `EffectItemType1`, `EffectItemType2`, `EffectItemType3`, `EffectMiscValue1`, `EffectMiscValue2`, `EffectMiscValue3`, `EffectMiscValueB1`, `EffectMiscValueB2`, `EffectMiscValueB3`, `EffectTriggerSpell1`, `EffectTriggerSpell2`, `EffectTriggerSpell3`, `EffectPointsPerComboPoint1`, `EffectPointsPerComboPoint2`, `EffectPointsPerComboPoint3`, `EffectSpellClassMaskA1`, `EffectSpellClassMaskA2`, `EffectSpellClassMaskA3`, `EffectSpellClassMaskB1`, `EffectSpellClassMaskB2`, `EffectSpellClassMaskB3`, `EffectSpellClassMaskC1`, `EffectSpellClassMaskC2`, `EffectSpellClassMaskC3`, `SpellVisual1`, `SpellVisual2`, `SpellIconID`, `ActiveIconID`, `SpellPriority`, `SpellName0`, `SpellName1`, `SpellName2`, `SpellName3`, `SpellName4`, `SpellName5`, `SpellName6`, `SpellName7`, `SpellName8`, `SpellNameFlag0`, `SpellNameFlag1`, `SpellNameFlag2`, `SpellNameFlag3`, `SpellNameFlag4`, `SpellNameFlag5`, `SpellNameFlag6`, `SpellNameFlag7`, `SpellRank0`, `SpellRank1`, `SpellRank2`, `SpellRank3`, `SpellRank4`, `SpellRank5`, `SpellRank6`, `SpellRank7`, `SpellRank8`, `SpellRankFlags0`, `SpellRankFlags1`, `SpellRankFlags2`, `SpellRankFlags3`, `SpellRankFlags4`, `SpellRankFlags5`, `SpellRankFlags6`, `SpellRankFlags7`, `SpellDescription0`, `SpellDescription1`, `SpellDescription2`, `SpellDescription3`, `SpellDescription4`, `SpellDescription5`, `SpellDescription6`, `SpellDescription7`, `SpellDescription8`, `SpellDescriptionFlags0`, `SpellDescriptionFlags1`, `SpellDescriptionFlags2`, `SpellDescriptionFlags3`, `SpellDescriptionFlags4`, `SpellDescriptionFlags5`, `SpellDescriptionFlags6`, `SpellDescriptionFlags7`, `SpellToolTip0`, `SpellToolTip1`, `SpellToolTip2`, `SpellToolTip3`, `SpellToolTip4`, `SpellToolTip5`, `SpellToolTip6`, `SpellToolTip7`, `SpellToolTip8`, `SpellToolTipFlags0`, `SpellToolTipFlags1`, `SpellToolTipFlags2`, `SpellToolTipFlags3`, `SpellToolTipFlags4`, `SpellToolTipFlags5`, `SpellToolTipFlags6`, `SpellToolTipFlags7`, `ManaCostPercentage`, `StartRecoveryCategory`, `StartRecoveryTime`, `MaximumTargetLevel`, `SpellFamilyName`, `SpellFamilyFlags`, `SpellFamilyFlags1`, `SpellFamilyFlags2`, `MaximumAffectedTargets`, `DamageClass`, `PreventionType`, `StanceBarOrder`, `EffectDamageMultiplier1`, `EffectDamageMultiplier2`, `EffectDamageMultiplier3`, `MinimumFactionId`, `MinimumReputation`, `RequiredAuraVision`, `TotemCategory1`, `TotemCategory2`, `AreaGroupID`, `SchoolMask`, `RuneCostID`, `SpellMissileID`, `PowerDisplayId`, `EffectBonusMultiplier1`, `EffectBonusMultiplier2`, `EffectBonusMultiplier3`, `SpellDescriptionVariableID`, `SpellDifficultyID`) VALUES
(200652, 0, 0, 0, 327696, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 40, 101, 0, 0, 30, 30, 1, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 6, 6, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 51, 150, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 512, 0, 3442, 0, 1941, 1941, 0, 'Barricade', '', '', '', '', '', '', '', '', '', '', '', '', '', '', '', 16712190, '', '', '', '', '', '', '', '', '', '', '', '', '', '', '', '', 16712188, 'Shield block chance and block value increased.  Blocking an attack heals you for your block value.', '', '', '', '', '', '', '', '', 0, 0, 0, 0, 0, 0, 0, 16712190, 'Shield block chance and block value increased.  Blocking heals you for your block value.', '', '', '', '', '', '', '', '', 0, 0, 0, 0, 0, 0, 0, 16712190, 0, 0, 0, 0, 4, 4096, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0);

-- ============================================================
-- 200653: the heal payload
-- ============================================================
-- A bare SPELL_EFFECT_HEAL on the caster whose amount is supplied per block by
-- CastCustomSpell.  Attributes 16 is SPELL_ATTR0_IS_ABILITY only -- explicitly
-- NOT 0x80 DO_NOT_DISPLAY or 0x100 DO_NOT_LOG, both of which suppress the
-- combat log entry.  The whole point of the talent is that the player watches
-- themselves heal off blocks, so the heal has to be visible.  It stays out of
-- the spellbook by never being taught.
--
-- DamageClass and PreventionType are cleared from the donor so a heal is not
-- treated as a melee ability.

DELETE FROM `alonecraft_spell_dbc` WHERE `ID` = 200653;
INSERT INTO `alonecraft_spell_dbc` (`ID`, `Category`, `Dispel`, `Mechanic`, `Attributes`, `AttributesEx`, `AttributesEx2`, `AttributesEx3`, `AttributesEx4`, `AttributesEx5`, `AttributesEx6`, `AttributesEx7`, `Stances`, `Unknown1`, `StancesNot`, `Unknown2`, `Targets`, `TargetCreatureType`, `RequiresSpellFocus`, `FacingCasterFlags`, `CasterAuraState`, `TargetAuraState`, `CasterAuraStateNot`, `TargetAuraStateNot`, `CasterAuraSpell`, `TargetAuraSpell`, `ExcludeCasterAuraSpell`, `ExcludeTargetAuraSpell`, `CastingTimeIndex`, `RecoveryTime`, `CategoryRecoveryTime`, `InterruptFlags`, `AuraInterruptFlags`, `ChannelInterruptFlags`, `ProcFlags`, `ProcChance`, `ProcCharges`, `MaximumLevel`, `BaseLevel`, `SpellLevel`, `DurationIndex`, `PowerType`, `ManaCost`, `ManaCostPerLevel`, `ManaPerSecond`, `ManaPerSecondPerLevel`, `RangeIndex`, `Speed`, `ModalNextSpell`, `StackAmount`, `Totem1`, `Totem2`, `Reagent1`, `Reagent2`, `Reagent3`, `Reagent4`, `Reagent5`, `Reagent6`, `Reagent7`, `Reagent8`, `ReagentCount1`, `ReagentCount2`, `ReagentCount3`, `ReagentCount4`, `ReagentCount5`, `ReagentCount6`, `ReagentCount7`, `ReagentCount8`, `EquippedItemClass`, `EquippedItemSubClassMask`, `EquippedItemInventoryTypeMask`, `Effect1`, `Effect2`, `Effect3`, `EffectDieSides1`, `EffectDieSides2`, `EffectDieSides3`, `EffectRealPointsPerLevel1`, `EffectRealPointsPerLevel2`, `EffectRealPointsPerLevel3`, `EffectBasePoints1`, `EffectBasePoints2`, `EffectBasePoints3`, `EffectMechanic1`, `EffectMechanic2`, `EffectMechanic3`, `EffectImplicitTargetA1`, `EffectImplicitTargetA2`, `EffectImplicitTargetA3`, `EffectImplicitTargetB1`, `EffectImplicitTargetB2`, `EffectImplicitTargetB3`, `EffectRadiusIndex1`, `EffectRadiusIndex2`, `EffectRadiusIndex3`, `EffectApplyAuraName1`, `EffectApplyAuraName2`, `EffectApplyAuraName3`, `EffectAmplitude1`, `EffectAmplitude2`, `EffectAmplitude3`, `EffectMultipleValue1`, `EffectMultipleValue2`, `EffectMultipleValue3`, `EffectChainTarget1`, `EffectChainTarget2`, `EffectChainTarget3`, `EffectItemType1`, `EffectItemType2`, `EffectItemType3`, `EffectMiscValue1`, `EffectMiscValue2`, `EffectMiscValue3`, `EffectMiscValueB1`, `EffectMiscValueB2`, `EffectMiscValueB3`, `EffectTriggerSpell1`, `EffectTriggerSpell2`, `EffectTriggerSpell3`, `EffectPointsPerComboPoint1`, `EffectPointsPerComboPoint2`, `EffectPointsPerComboPoint3`, `EffectSpellClassMaskA1`, `EffectSpellClassMaskA2`, `EffectSpellClassMaskA3`, `EffectSpellClassMaskB1`, `EffectSpellClassMaskB2`, `EffectSpellClassMaskB3`, `EffectSpellClassMaskC1`, `EffectSpellClassMaskC2`, `EffectSpellClassMaskC3`, `SpellVisual1`, `SpellVisual2`, `SpellIconID`, `ActiveIconID`, `SpellPriority`, `SpellName0`, `SpellName1`, `SpellName2`, `SpellName3`, `SpellName4`, `SpellName5`, `SpellName6`, `SpellName7`, `SpellName8`, `SpellNameFlag0`, `SpellNameFlag1`, `SpellNameFlag2`, `SpellNameFlag3`, `SpellNameFlag4`, `SpellNameFlag5`, `SpellNameFlag6`, `SpellNameFlag7`, `SpellRank0`, `SpellRank1`, `SpellRank2`, `SpellRank3`, `SpellRank4`, `SpellRank5`, `SpellRank6`, `SpellRank7`, `SpellRank8`, `SpellRankFlags0`, `SpellRankFlags1`, `SpellRankFlags2`, `SpellRankFlags3`, `SpellRankFlags4`, `SpellRankFlags5`, `SpellRankFlags6`, `SpellRankFlags7`, `SpellDescription0`, `SpellDescription1`, `SpellDescription2`, `SpellDescription3`, `SpellDescription4`, `SpellDescription5`, `SpellDescription6`, `SpellDescription7`, `SpellDescription8`, `SpellDescriptionFlags0`, `SpellDescriptionFlags1`, `SpellDescriptionFlags2`, `SpellDescriptionFlags3`, `SpellDescriptionFlags4`, `SpellDescriptionFlags5`, `SpellDescriptionFlags6`, `SpellDescriptionFlags7`, `SpellToolTip0`, `SpellToolTip1`, `SpellToolTip2`, `SpellToolTip3`, `SpellToolTip4`, `SpellToolTip5`, `SpellToolTip6`, `SpellToolTip7`, `SpellToolTip8`, `SpellToolTipFlags0`, `SpellToolTipFlags1`, `SpellToolTipFlags2`, `SpellToolTipFlags3`, `SpellToolTipFlags4`, `SpellToolTipFlags5`, `SpellToolTipFlags6`, `SpellToolTipFlags7`, `ManaCostPercentage`, `StartRecoveryCategory`, `StartRecoveryTime`, `MaximumTargetLevel`, `SpellFamilyName`, `SpellFamilyFlags`, `SpellFamilyFlags1`, `SpellFamilyFlags2`, `MaximumAffectedTargets`, `DamageClass`, `PreventionType`, `StanceBarOrder`, `EffectDamageMultiplier1`, `EffectDamageMultiplier2`, `EffectDamageMultiplier3`, `MinimumFactionId`, `MinimumReputation`, `RequiredAuraVision`, `TotemCategory1`, `TotemCategory2`, `AreaGroupID`, `SchoolMask`, `RuneCostID`, `SpellMissileID`, `PowerDisplayId`, `EffectBonusMultiplier1`, `EffectBonusMultiplier2`, `EffectBonusMultiplier3`, `SpellDescriptionVariableID`, `SpellDifficultyID`) VALUES
(200653, 0, 0, 0, 16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 101, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 512, 0, 3442, 0, 1941, 1941, 0, 'Barricade', '', '', '', '', '', '', '', '', '', '', '', '', '', '', '', 16712190, '', '', '', '', '', '', '', '', '', '', '', '', '', '', '', '', 16712188, 'Heals you for your block value.', '', '', '', '', '', '', '', '', 0, 0, 0, 0, 0, 0, 0, 16712190, 'Block chance and block value increased by $s1%.', '', '', '', '', '', '', '', '', 0, 0, 0, 0, 0, 0, 0, 16712190, 0, 0, 0, 0, 4, 4096, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0);

-- ============================================================
-- Heal-on-block proc
-- ============================================================
-- ProcFlags 40 = TAKEN_MELEE_AUTO_ATTACK | TAKEN_SPELL_MELEE_DMG_CLASS, and
-- HitMask 8256 = PROC_HIT_BLOCK (0x40) | PROC_HIT_FULL_BLOCK (0x2000), so both
-- partial and full blocks pay out.  Identical to the Shield Mastery block proc
-- in woa_2026_08_09_10.sql.
--
-- SpellPhaseMask 2 (PROC_SPELL_PHASE_HIT) is mandatory -- a zero phase mask
-- makes the proc silently never fire.
--
-- No Cooldown and no Chance limit: a block is already gated by block chance,
-- and the healing is a fraction of block value, so throttling it would just
-- make the talent feel unreliable.

DELETE FROM `spell_proc` WHERE `SpellId` = 200652;
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(200652, 0, 0, 0, 0, 0, 40, 0, 2, 8256, 0, 0, 0, 0, 0, 0);

-- ============================================================
-- Script registrations
-- ============================================================
-- spell_warr_barricade_rage_suppress is retired along with the toggle: nothing
-- suppresses Shield Slam or Revenge rage any more, so its rows on -23922 and
-- -6572 must go or those spells keep running a script with no purpose.

DELETE FROM `spell_script_names` WHERE `ScriptName` IN ('spell_warr_barricade', 'spell_warr_barricade_rage_suppress', 'spell_warr_barricade_block_heal');
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(200651, 'spell_warr_barricade'),
(200652, 'spell_warr_barricade_block_heal');
