-- ===========================================================================
-- Warlock / Demonology: Nemesis -> proc-per-minute
-- ===========================================================================
--
-- Nemesis generated far too few Soul Shards, for two reasons.
--
--   1. The pet half never fired at all.  spell_warl_demon_nemesis read its
--      chance from the talent's third effect (OwnerTalentAmount(..., EFFECT_2)),
--      but EffectBasePoints3 and EffectDieSides3 were both 0 on all three
--      ranks, so GetAmount() returned 0 and every demon attack logged
--      "roll=fail".  Only the warlock's own crits ever paid out.
--
--   2. The owner half was a 5/10/15% roll gated behind HitMask = 2
--      (critical strikes only), so the real rate was that chance times the
--      warlock's crit rate -- about one shard a minute at rank 3 in quest
--      greens, and worse the lower the gear.
--
-- Both halves now use the engine's proc-per-minute path, which is what
-- Killing Machine (51123-51130) does: the spell_proc row carries
-- ProcsPerMinute per rank and nothing else, and Aura::CalcProcChance
-- (SpellAuras.cpp:2273) converts it to a per-event chance from the attack
-- speed -- the caster's weapon speed for melee, the spell's cast time with a
-- 1.5s floor for everything else.  The result is a flat rate that does not
-- move with crit rating, which is the whole point for a levelling warlock.
--
-- Consequences of the switch, all deliberate:
--
--   * The critical-strike gate is gone (HitMask 0).  PPM on top of a crit
--     roll is two random gates multiplying out to an unpredictable rate, and
--     re-introduces exactly the gear dependence PPM exists to remove.  The
--     descriptions below drop the word "critical" to match.
--
--   * PROC_FLAG_DONE_PERIODIC (0x40000) is removed from ProcFlags.  DoT ticks
--     carry no cast time, so the PPM path floors them at 1.5s and rolls the
--     full per-cast chance on every tick -- four DoTs ticking every 3s would
--     have paid several times the intended rate, and the Felguard's
--     Immolation Aura ticks on every nearby enemy.  331796 -> 69652
--     (DONE_MELEE_AUTO_ATTACK | DONE_SPELL_MELEE_DMG_CLASS |
--      DONE_SPELL_NONE_DMG_CLASS_NEG | DONE_SPELL_MAGIC_DMG_CLASS_NEG).
--
--   * The pet half needs one aura per rank.  ProcsPerMinute is a spell_proc
--     column and spell_proc is keyed by spell id, so a single shared pet aura
--     cannot carry a per-rank rate.  200422 is joined by 200425 and 200426,
--     and the script's manual roll goes away with them.
--
-- Rate at rank 3: 6 PPM from the warlock plus 6 PPM from the demon.  Halve
-- the pet rows if that reads as too generous once tested.
--
-- ===========================================================================

-- ---------------------------------------------------------------------------
-- Pet-side carriers, one per talent rank
-- ---------------------------------------------------------------------------
-- 200425 and 200426 are copies of 200422 (a hidden dummy aura on the demon).
-- They exist only so each rank has its own spell id for spell_proc to hang a
-- ProcsPerMinute value on, exactly as Killing Machine's five ranks each have
-- their own.

DELETE FROM `alonecraft_spell_dbc` WHERE `ID` IN (200425, 200426);
INSERT INTO `alonecraft_spell_dbc` (`ID`, `Category`, `Dispel`, `Mechanic`, `Attributes`, `AttributesEx`, `AttributesEx2`, `AttributesEx3`, `AttributesEx4`, `AttributesEx5`, `AttributesEx6`, `AttributesEx7`, `Stances`, `Unknown1`, `StancesNot`, `Unknown2`, `Targets`, `TargetCreatureType`, `RequiresSpellFocus`, `FacingCasterFlags`, `CasterAuraState`, `TargetAuraState`, `CasterAuraStateNot`, `TargetAuraStateNot`, `CasterAuraSpell`, `TargetAuraSpell`, `ExcludeCasterAuraSpell`, `ExcludeTargetAuraSpell`, `CastingTimeIndex`, `RecoveryTime`, `CategoryRecoveryTime`, `InterruptFlags`, `AuraInterruptFlags`, `ChannelInterruptFlags`, `ProcFlags`, `ProcChance`, `ProcCharges`, `MaximumLevel`, `BaseLevel`, `SpellLevel`, `DurationIndex`, `PowerType`, `ManaCost`, `ManaCostPerLevel`, `ManaPerSecond`, `ManaPerSecondPerLevel`, `RangeIndex`, `Speed`, `ModalNextSpell`, `StackAmount`, `Totem1`, `Totem2`, `Reagent1`, `Reagent2`, `Reagent3`, `Reagent4`, `Reagent5`, `Reagent6`, `Reagent7`, `Reagent8`, `ReagentCount1`, `ReagentCount2`, `ReagentCount3`, `ReagentCount4`, `ReagentCount5`, `ReagentCount6`, `ReagentCount7`, `ReagentCount8`, `EquippedItemClass`, `EquippedItemSubClassMask`, `EquippedItemInventoryTypeMask`, `Effect1`, `Effect2`, `Effect3`, `EffectDieSides1`, `EffectDieSides2`, `EffectDieSides3`, `EffectRealPointsPerLevel1`, `EffectRealPointsPerLevel2`, `EffectRealPointsPerLevel3`, `EffectBasePoints1`, `EffectBasePoints2`, `EffectBasePoints3`, `EffectMechanic1`, `EffectMechanic2`, `EffectMechanic3`, `EffectImplicitTargetA1`, `EffectImplicitTargetA2`, `EffectImplicitTargetA3`, `EffectImplicitTargetB1`, `EffectImplicitTargetB2`, `EffectImplicitTargetB3`, `EffectRadiusIndex1`, `EffectRadiusIndex2`, `EffectRadiusIndex3`, `EffectApplyAuraName1`, `EffectApplyAuraName2`, `EffectApplyAuraName3`, `EffectAmplitude1`, `EffectAmplitude2`, `EffectAmplitude3`, `EffectMultipleValue1`, `EffectMultipleValue2`, `EffectMultipleValue3`, `EffectChainTarget1`, `EffectChainTarget2`, `EffectChainTarget3`, `EffectItemType1`, `EffectItemType2`, `EffectItemType3`, `EffectMiscValue1`, `EffectMiscValue2`, `EffectMiscValue3`, `EffectMiscValueB1`, `EffectMiscValueB2`, `EffectMiscValueB3`, `EffectTriggerSpell1`, `EffectTriggerSpell2`, `EffectTriggerSpell3`, `EffectPointsPerComboPoint1`, `EffectPointsPerComboPoint2`, `EffectPointsPerComboPoint3`, `EffectSpellClassMaskA1`, `EffectSpellClassMaskA2`, `EffectSpellClassMaskA3`, `EffectSpellClassMaskB1`, `EffectSpellClassMaskB2`, `EffectSpellClassMaskB3`, `EffectSpellClassMaskC1`, `EffectSpellClassMaskC2`, `EffectSpellClassMaskC3`, `SpellVisual1`, `SpellVisual2`, `SpellIconID`, `ActiveIconID`, `SpellPriority`, `SpellName0`, `SpellName1`, `SpellName2`, `SpellName3`, `SpellName4`, `SpellName5`, `SpellName6`, `SpellName7`, `SpellName8`, `SpellNameFlag0`, `SpellNameFlag1`, `SpellNameFlag2`, `SpellNameFlag3`, `SpellNameFlag4`, `SpellNameFlag5`, `SpellNameFlag6`, `SpellNameFlag7`, `SpellRank0`, `SpellRank1`, `SpellRank2`, `SpellRank3`, `SpellRank4`, `SpellRank5`, `SpellRank6`, `SpellRank7`, `SpellRank8`, `SpellRankFlags0`, `SpellRankFlags1`, `SpellRankFlags2`, `SpellRankFlags3`, `SpellRankFlags4`, `SpellRankFlags5`, `SpellRankFlags6`, `SpellRankFlags7`, `SpellDescription0`, `SpellDescription1`, `SpellDescription2`, `SpellDescription3`, `SpellDescription4`, `SpellDescription5`, `SpellDescription6`, `SpellDescription7`, `SpellDescription8`, `SpellDescriptionFlags0`, `SpellDescriptionFlags1`, `SpellDescriptionFlags2`, `SpellDescriptionFlags3`, `SpellDescriptionFlags4`, `SpellDescriptionFlags5`, `SpellDescriptionFlags6`, `SpellDescriptionFlags7`, `SpellToolTip0`, `SpellToolTip1`, `SpellToolTip2`, `SpellToolTip3`, `SpellToolTip4`, `SpellToolTip5`, `SpellToolTip6`, `SpellToolTip7`, `SpellToolTip8`, `SpellToolTipFlags0`, `SpellToolTipFlags1`, `SpellToolTipFlags2`, `SpellToolTipFlags3`, `SpellToolTipFlags4`, `SpellToolTipFlags5`, `SpellToolTipFlags6`, `SpellToolTipFlags7`, `ManaCostPercentage`, `StartRecoveryCategory`, `StartRecoveryTime`, `MaximumTargetLevel`, `SpellFamilyName`, `SpellFamilyFlags`, `SpellFamilyFlags1`, `SpellFamilyFlags2`, `MaximumAffectedTargets`, `DamageClass`, `PreventionType`, `StanceBarOrder`, `EffectDamageMultiplier1`, `EffectDamageMultiplier2`, `EffectDamageMultiplier3`, `MinimumFactionId`, `MinimumReputation`, `RequiredAuraVision`, `TotemCategory1`, `TotemCategory2`, `AreaGroupID`, `SchoolMask`, `RuneCostID`, `SpellMissileID`, `PowerDisplayId`, `EffectBonusMultiplier1`, `EffectBonusMultiplier2`, `EffectBonusMultiplier3`, `SpellDescriptionVariableID`, `SpellDifficultyID`) VALUES
(200425, 0, 0, 0, 448, 0, 0, 268435456, 34603008, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 101, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 'Nemesis', '', '', '', '', '', '', '', '', '', '', '', '', '', '', '', 16712190, '', '', '', '', '', '', '', '', '', '', '', '', '', '', '', '', 16712172, 'Attacks may grant the summoner a Soul Shard.', '', '', '', '', '', '', '', '', 0, 0, 0, 0, 0, 0, 0, 16712188, '', '', '', '', '', '', '', '', '', 0, 0, 0, 0, 0, 0, 0, 16712188, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0);

INSERT INTO `alonecraft_spell_dbc` (`ID`, `Category`, `Dispel`, `Mechanic`, `Attributes`, `AttributesEx`, `AttributesEx2`, `AttributesEx3`, `AttributesEx4`, `AttributesEx5`, `AttributesEx6`, `AttributesEx7`, `Stances`, `Unknown1`, `StancesNot`, `Unknown2`, `Targets`, `TargetCreatureType`, `RequiresSpellFocus`, `FacingCasterFlags`, `CasterAuraState`, `TargetAuraState`, `CasterAuraStateNot`, `TargetAuraStateNot`, `CasterAuraSpell`, `TargetAuraSpell`, `ExcludeCasterAuraSpell`, `ExcludeTargetAuraSpell`, `CastingTimeIndex`, `RecoveryTime`, `CategoryRecoveryTime`, `InterruptFlags`, `AuraInterruptFlags`, `ChannelInterruptFlags`, `ProcFlags`, `ProcChance`, `ProcCharges`, `MaximumLevel`, `BaseLevel`, `SpellLevel`, `DurationIndex`, `PowerType`, `ManaCost`, `ManaCostPerLevel`, `ManaPerSecond`, `ManaPerSecondPerLevel`, `RangeIndex`, `Speed`, `ModalNextSpell`, `StackAmount`, `Totem1`, `Totem2`, `Reagent1`, `Reagent2`, `Reagent3`, `Reagent4`, `Reagent5`, `Reagent6`, `Reagent7`, `Reagent8`, `ReagentCount1`, `ReagentCount2`, `ReagentCount3`, `ReagentCount4`, `ReagentCount5`, `ReagentCount6`, `ReagentCount7`, `ReagentCount8`, `EquippedItemClass`, `EquippedItemSubClassMask`, `EquippedItemInventoryTypeMask`, `Effect1`, `Effect2`, `Effect3`, `EffectDieSides1`, `EffectDieSides2`, `EffectDieSides3`, `EffectRealPointsPerLevel1`, `EffectRealPointsPerLevel2`, `EffectRealPointsPerLevel3`, `EffectBasePoints1`, `EffectBasePoints2`, `EffectBasePoints3`, `EffectMechanic1`, `EffectMechanic2`, `EffectMechanic3`, `EffectImplicitTargetA1`, `EffectImplicitTargetA2`, `EffectImplicitTargetA3`, `EffectImplicitTargetB1`, `EffectImplicitTargetB2`, `EffectImplicitTargetB3`, `EffectRadiusIndex1`, `EffectRadiusIndex2`, `EffectRadiusIndex3`, `EffectApplyAuraName1`, `EffectApplyAuraName2`, `EffectApplyAuraName3`, `EffectAmplitude1`, `EffectAmplitude2`, `EffectAmplitude3`, `EffectMultipleValue1`, `EffectMultipleValue2`, `EffectMultipleValue3`, `EffectChainTarget1`, `EffectChainTarget2`, `EffectChainTarget3`, `EffectItemType1`, `EffectItemType2`, `EffectItemType3`, `EffectMiscValue1`, `EffectMiscValue2`, `EffectMiscValue3`, `EffectMiscValueB1`, `EffectMiscValueB2`, `EffectMiscValueB3`, `EffectTriggerSpell1`, `EffectTriggerSpell2`, `EffectTriggerSpell3`, `EffectPointsPerComboPoint1`, `EffectPointsPerComboPoint2`, `EffectPointsPerComboPoint3`, `EffectSpellClassMaskA1`, `EffectSpellClassMaskA2`, `EffectSpellClassMaskA3`, `EffectSpellClassMaskB1`, `EffectSpellClassMaskB2`, `EffectSpellClassMaskB3`, `EffectSpellClassMaskC1`, `EffectSpellClassMaskC2`, `EffectSpellClassMaskC3`, `SpellVisual1`, `SpellVisual2`, `SpellIconID`, `ActiveIconID`, `SpellPriority`, `SpellName0`, `SpellName1`, `SpellName2`, `SpellName3`, `SpellName4`, `SpellName5`, `SpellName6`, `SpellName7`, `SpellName8`, `SpellNameFlag0`, `SpellNameFlag1`, `SpellNameFlag2`, `SpellNameFlag3`, `SpellNameFlag4`, `SpellNameFlag5`, `SpellNameFlag6`, `SpellNameFlag7`, `SpellRank0`, `SpellRank1`, `SpellRank2`, `SpellRank3`, `SpellRank4`, `SpellRank5`, `SpellRank6`, `SpellRank7`, `SpellRank8`, `SpellRankFlags0`, `SpellRankFlags1`, `SpellRankFlags2`, `SpellRankFlags3`, `SpellRankFlags4`, `SpellRankFlags5`, `SpellRankFlags6`, `SpellRankFlags7`, `SpellDescription0`, `SpellDescription1`, `SpellDescription2`, `SpellDescription3`, `SpellDescription4`, `SpellDescription5`, `SpellDescription6`, `SpellDescription7`, `SpellDescription8`, `SpellDescriptionFlags0`, `SpellDescriptionFlags1`, `SpellDescriptionFlags2`, `SpellDescriptionFlags3`, `SpellDescriptionFlags4`, `SpellDescriptionFlags5`, `SpellDescriptionFlags6`, `SpellDescriptionFlags7`, `SpellToolTip0`, `SpellToolTip1`, `SpellToolTip2`, `SpellToolTip3`, `SpellToolTip4`, `SpellToolTip5`, `SpellToolTip6`, `SpellToolTip7`, `SpellToolTip8`, `SpellToolTipFlags0`, `SpellToolTipFlags1`, `SpellToolTipFlags2`, `SpellToolTipFlags3`, `SpellToolTipFlags4`, `SpellToolTipFlags5`, `SpellToolTipFlags6`, `SpellToolTipFlags7`, `ManaCostPercentage`, `StartRecoveryCategory`, `StartRecoveryTime`, `MaximumTargetLevel`, `SpellFamilyName`, `SpellFamilyFlags`, `SpellFamilyFlags1`, `SpellFamilyFlags2`, `MaximumAffectedTargets`, `DamageClass`, `PreventionType`, `StanceBarOrder`, `EffectDamageMultiplier1`, `EffectDamageMultiplier2`, `EffectDamageMultiplier3`, `MinimumFactionId`, `MinimumReputation`, `RequiredAuraVision`, `TotemCategory1`, `TotemCategory2`, `AreaGroupID`, `SchoolMask`, `RuneCostID`, `SpellMissileID`, `PowerDisplayId`, `EffectBonusMultiplier1`, `EffectBonusMultiplier2`, `EffectBonusMultiplier3`, `SpellDescriptionVariableID`, `SpellDifficultyID`) VALUES
(200426, 0, 0, 0, 448, 0, 0, 268435456, 34603008, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 101, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 'Nemesis', '', '', '', '', '', '', '', '', '', '', '', '', '', '', '', 16712190, '', '', '', '', '', '', '', '', '', '', '', '', '', '', '', '', 16712172, 'Attacks may grant the summoner a Soul Shard.', '', '', '', '', '', '', '', '', 0, 0, 0, 0, 0, 0, 0, 16712188, '', '', '', '', '', '', '', '', '', 0, 0, 0, 0, 0, 0, 0, 16712188, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0);

-- 200422 only needs its description reworded now that the crit gate is gone.
-- A single-column change is an UPDATE, never a 234-column re-INSERT.
UPDATE `alonecraft_spell_dbc`
    SET `SpellDescription0` = 'Attacks may grant the summoner a Soul Shard.'
    WHERE `ID` = 200422;

-- ---------------------------------------------------------------------------
-- Talent descriptions
-- ---------------------------------------------------------------------------
-- A PPM rate has no honest percentage to print, so ranks 2 and 3 use the
-- stock Blizzard wording for exactly this case -- Killing Machine ranks 2-5
-- read "Effect occurs more often than Killing Machine (Rank N-1)." (note the
-- two spaces before "Effect", which is how the client's own strings are
-- punctuated).  The cooldown clause still states its percentage, because that
-- effect is a plain spell modifier and does have a number to show.

UPDATE `alonecraft_spell_dbc`
    SET `SpellDescription0` = 'Reduces the cooldown of your Demonic Empowerment, Metamorphosis and Fel Domination spells by 10%, and your attacks and those of your demon have a chance to generate a Soul Shard.'
    WHERE `ID` = 63117;

UPDATE `alonecraft_spell_dbc`
    SET `SpellDescription0` = 'Reduces the cooldown of your Demonic Empowerment, Metamorphosis and Fel Domination spells by 20%, and your attacks and those of your demon have a chance to generate a Soul Shard.  Effect occurs more often than Nemesis (Rank 1).'
    WHERE `ID` = 63121;

UPDATE `alonecraft_spell_dbc`
    SET `SpellDescription0` = 'Reduces the cooldown of your Demonic Empowerment, Metamorphosis and Fel Domination spells by 30%, and your attacks and those of your demon have a chance to generate a Soul Shard.  Effect occurs more often than Nemesis (Rank 2).'
    WHERE `ID` = 63123;

-- ---------------------------------------------------------------------------
-- Pet aura anchors, now one per rank
-- ---------------------------------------------------------------------------
DELETE FROM `spell_pet_auras` WHERE `spell` IN (63117, 63121, 63123);
INSERT INTO `spell_pet_auras` (`spell`, `effectId`, `pet`, `aura`) VALUES
(63117, 2, 0, 200422),
(63121, 2, 0, 200425),
(63123, 2, 0, 200426);

-- ---------------------------------------------------------------------------
-- The proc rows
-- ---------------------------------------------------------------------------
-- Chance is left at 0 on purpose: CalcProcChance ignores it entirely once
-- ProcsPerMinute is non-zero and the event carries damage, and SpellMgr's
-- "no Chance and no ProcsPerMinute" warning only fires when both are 0.
-- SpellPhaseMask stays 2 (PROC_SPELL_PHASE_HIT); Killing Machine can leave it
-- at 0 because it procs off melee auto-attacks, which are not spell phases,
-- but every source here includes real spell casts.

DELETE FROM `spell_proc` WHERE ABS(`SpellId`) IN
    (63117, 63121, 63123, 200422, 200425, 200426);

INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES

-- Owner side: the warlock's own attacks, 2/4/6 PPM by rank.  Effect1 of these
-- spells is a passive cooldown spellmod, not a proc, so it is unaffected.
(63117, 0, 0, 0, 0, 0, 69652, 1, 2, 0, 0, 0, 2, 0, 0, 0),
(63121, 0, 0, 0, 0, 0, 69652, 1, 2, 0, 0, 0, 4, 0, 0, 0),
(63123, 0, 0, 0, 0, 0, 69652, 1, 2, 0, 0, 0, 6, 0, 0, 0),

-- Pet side: the same rates off the demon's own attacks.  The caster of a
-- spell_pet_auras aura is the pet, so a Felguard's 2.0s swing at 6 PPM is a
-- 20% roll per swing -- 6 shards a minute, as intended.
(200422, 0, 0, 0, 0, 0, 69652, 1, 2, 0, 0, 0, 2, 0, 0, 0),
(200425, 0, 0, 0, 0, 0, 69652, 1, 2, 0, 0, 0, 4, 0, 0, 0),
(200426, 0, 0, 0, 0, 0, 69652, 1, 2, 0, 0, 0, 6, 0, 0, 0);

-- ---------------------------------------------------------------------------
-- The two new pet auras need the same script as 200422
-- ---------------------------------------------------------------------------
DELETE FROM `spell_script_names`
    WHERE `spell_id` IN (200425, 200426)
      AND `ScriptName` = 'spell_warl_demon_nemesis';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(200425, 'spell_warl_demon_nemesis'),
(200426, 'spell_warl_demon_nemesis');
