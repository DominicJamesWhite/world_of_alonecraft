-- Alonecraft 4.61 -- Sacrifice of Blood becomes a pure data change.
--
-- The talent (18703/18704, the Improved Health Funnel reskin) used to be a
-- 208-line AuraScript on the Health Funnel rank chain that read inert
-- SPELL_AURA_DUMMY value-holders off the talent and pushed them into a runtime
-- CastCustomSpell of custom carrier 200411.  All of that is deleted here.
--
--
-- CORE ALREADY DOES THE DELIVERY
--
--   spell_warl_health_funnel (spell_warlock.cpp:1100-1118) hooks the same place
--   the module script did -- OnEffectApply/OnEffectRemove of 755 EFFECT_0,
--   SPELL_AURA_PERIODIC_HEAL -- and already does exactly what the talent wants:
--
--       if (caster->HasAura(18703))  target->CastSpell(target, 60955, true);
--       ... RemoveEffect() removes both 60955 and 60956.
--
--   So the whole talent reduces to "what are 60955/60956?", which is a DBC
--   question.  Both implementations were running at once until now, so the
--   demon was carrying two overlapping buffs.  Retiring the module script (see
--   the spell_script_names section at the bottom) leaves core's as the only one.
--
--
-- THE HEALING BONUS IS A SPELLMOD ON THE TALENT, NOT AN AURA ON THE DEMON
--
--   The obvious shape -- a third effect on the demon buff -- does not work, and
--   the version that half-works is worse than useless:
--
--     * SPELL_AURA_MOD_HEALING_RECEIVED (283) is the correctly-scoped aura
--       ("healing from THIS caster, matching THIS class mask"), but Unit.cpp:9782
--       gates it on `caster->GetGUID() == aurEff->GetCasterGUID()` and core has
--       the PET self-cast the buff (`target->CastSpell(target, ...)`,
--       spell_warlock.cpp:1108).  The aura's caster is the demon, so the check
--       never passes and the effect silently does nothing.
--
--     * SPELL_AURA_MOD_HEALING_PCT (118) does fire from a pet-cast aura, but it
--       has no caster filter and no class mask: it would boost EVERY heal the
--       demon receives from anyone, which is not what this talent is.  It also
--       reads through GetMaxPositiveAuraModifier (Unit.cpp:9700), so it takes
--       the single largest modifier instead of stacking.
--
--   So the healing goes on the talent as a plain spell modifier instead, which
--   is where Blizzard put it too ("Increases the amount of Health transferred by
--   your Health Funnel spell by ...").  The path:
--
--     AuraEffect::CalculateAmount (SpellAuraEffects.cpp:566-568) -- a
--     SPELL_AURA_PERIODIC_HEAL on a UNIT_AURA calls SpellHealingBonusDone with
--     damagetype DOT, which at Unit.cpp:9681 does
--         modOwner->ApplySpellMod(spellProto->Id, SPELLMOD_DOT, heal);
--
--   Hence ADD_PCT_MODIFIER with MiscValue 22 (SPELLMOD_DOT), SpellFamilyName 5
--   and the Health Funnel class mask.  It touches Health Funnel's heal and
--   nothing else in the game, stacks additively with other spellmods the normal
--   way, and needs no C++.  It is applied once at channel start rather than per
--   tick -- SpellHealingBonusDone is only called at tick time for DYNOBJ auras
--   (SpellAuraEffects.cpp:6635), so there is no double-dip.
--
--
-- THE HEALTH COST REDUCTION IS GONE
--
--   The old talent advertised -40/-80% Health Funnel cost.  Health Funnel charges
--   health twice -- ManaCost up front (which honours SPELLMOD_COST) and
--   ManaPerSecond every second, drained straight from Aura::Update with no
--   spellmod anywhere in the path.  The per-second drain is the larger share, so
--   the plain ADD_PCT_MODIFIER delivered a fraction of what it claimed and the
--   script had a 45-line per-tick refund to close the gap.  Effect and hack both
--   deleted; the talent is now purely the demon buff.
--
--   Note the channel cost does NOT rise to pay for the bigger heal:
--   SpellAuraEffects.cpp:6683-6696 drains min(ManaPerSecond, gain), so a larger
--   gain only pushes the drain up to the cap it already sat at.
--
--
-- DIE SIDES
--
--   The base rows ship EffectDieSides1 = 1, so CalcValue = BasePoints + 1
--   (SpellInfo.cpp:431-437).  Every base point below is therefore N-1.  Kept at 1
--   rather than zeroed so the surviving Blizzard effect is not silently retuned.
--
--                             ON THE DEMON (60955/60956)   ON THE TALENT
--       rank   talent   buff     dmg taken   dmg done     funnel healing
--         1    18703    60955       -15%       +25%            +25%
--         2    18704    60956       -30%       +50%            +50%
--
--   Tooltips use $s1/$s2/$s3 so they cannot drift from the data.  $s1 is stored
--   negative and the client renders it unsigned -- same shape as Blizzard's own
--   60955 tooltip ('Damage taken is reduced by $s1%.' with BasePoints -16) and
--   Sunder Armor's 'Armor decreased by $s1%.'


-- ---------------------------------------------------------------------------
-- 60955 / 60956: the demon buff
-- ---------------------------------------------------------------------------
-- Base rows are already the right shape: Attributes 0 (buff-bar visible),
-- DurationIndex 21 (permanent -- core removes it by hand when the channel ends),
-- EffectImplicitTargetA 1, and Effect1 = aura 87 MOD_DAMAGE_PERCENT_TAKEN with
-- MiscValue 127 (all schools).  Effect2 was free; Effect3 stays empty.
--
-- SpellFamilyName stays 0 and all nine EffectSpellClassMask words stay 0 --
-- neither aura 87 nor 79 reads a class mask.

DELETE FROM `alonecraft_spell_dbc` WHERE `ID` IN (60955, 60956);

INSERT INTO `alonecraft_spell_dbc` (`ID`, `Category`, `Dispel`, `Mechanic`, `Attributes`, `AttributesEx`, `AttributesEx2`, `AttributesEx3`, `AttributesEx4`, `AttributesEx5`, `AttributesEx6`, `AttributesEx7`, `Stances`, `Unknown1`, `StancesNot`, `Unknown2`, `Targets`, `TargetCreatureType`, `RequiresSpellFocus`, `FacingCasterFlags`, `CasterAuraState`, `TargetAuraState`, `CasterAuraStateNot`, `TargetAuraStateNot`, `CasterAuraSpell`, `TargetAuraSpell`, `ExcludeCasterAuraSpell`, `ExcludeTargetAuraSpell`, `CastingTimeIndex`, `RecoveryTime`, `CategoryRecoveryTime`, `InterruptFlags`, `AuraInterruptFlags`, `ChannelInterruptFlags`, `ProcFlags`, `ProcChance`, `ProcCharges`, `MaximumLevel`, `BaseLevel`, `SpellLevel`, `DurationIndex`, `PowerType`, `ManaCost`, `ManaCostPerLevel`, `ManaPerSecond`, `ManaPerSecondPerLevel`, `RangeIndex`, `Speed`, `ModalNextSpell`, `StackAmount`, `Totem1`, `Totem2`, `Reagent1`, `Reagent2`, `Reagent3`, `Reagent4`, `Reagent5`, `Reagent6`, `Reagent7`, `Reagent8`, `ReagentCount1`, `ReagentCount2`, `ReagentCount3`, `ReagentCount4`, `ReagentCount5`, `ReagentCount6`, `ReagentCount7`, `ReagentCount8`, `EquippedItemClass`, `EquippedItemSubClassMask`, `EquippedItemInventoryTypeMask`, `Effect1`, `Effect2`, `Effect3`, `EffectDieSides1`, `EffectDieSides2`, `EffectDieSides3`, `EffectRealPointsPerLevel1`, `EffectRealPointsPerLevel2`, `EffectRealPointsPerLevel3`, `EffectBasePoints1`, `EffectBasePoints2`, `EffectBasePoints3`, `EffectMechanic1`, `EffectMechanic2`, `EffectMechanic3`, `EffectImplicitTargetA1`, `EffectImplicitTargetA2`, `EffectImplicitTargetA3`, `EffectImplicitTargetB1`, `EffectImplicitTargetB2`, `EffectImplicitTargetB3`, `EffectRadiusIndex1`, `EffectRadiusIndex2`, `EffectRadiusIndex3`, `EffectApplyAuraName1`, `EffectApplyAuraName2`, `EffectApplyAuraName3`, `EffectAmplitude1`, `EffectAmplitude2`, `EffectAmplitude3`, `EffectMultipleValue1`, `EffectMultipleValue2`, `EffectMultipleValue3`, `EffectChainTarget1`, `EffectChainTarget2`, `EffectChainTarget3`, `EffectItemType1`, `EffectItemType2`, `EffectItemType3`, `EffectMiscValue1`, `EffectMiscValue2`, `EffectMiscValue3`, `EffectMiscValueB1`, `EffectMiscValueB2`, `EffectMiscValueB3`, `EffectTriggerSpell1`, `EffectTriggerSpell2`, `EffectTriggerSpell3`, `EffectPointsPerComboPoint1`, `EffectPointsPerComboPoint2`, `EffectPointsPerComboPoint3`, `EffectSpellClassMaskA1`, `EffectSpellClassMaskA2`, `EffectSpellClassMaskA3`, `EffectSpellClassMaskB1`, `EffectSpellClassMaskB2`, `EffectSpellClassMaskB3`, `EffectSpellClassMaskC1`, `EffectSpellClassMaskC2`, `EffectSpellClassMaskC3`, `SpellVisual1`, `SpellVisual2`, `SpellIconID`, `ActiveIconID`, `SpellPriority`, `SpellName0`, `SpellName1`, `SpellName2`, `SpellName3`, `SpellName4`, `SpellName5`, `SpellName6`, `SpellName7`, `SpellName8`, `SpellNameFlag0`, `SpellNameFlag1`, `SpellNameFlag2`, `SpellNameFlag3`, `SpellNameFlag4`, `SpellNameFlag5`, `SpellNameFlag6`, `SpellNameFlag7`, `SpellRank0`, `SpellRank1`, `SpellRank2`, `SpellRank3`, `SpellRank4`, `SpellRank5`, `SpellRank6`, `SpellRank7`, `SpellRank8`, `SpellRankFlags0`, `SpellRankFlags1`, `SpellRankFlags2`, `SpellRankFlags3`, `SpellRankFlags4`, `SpellRankFlags5`, `SpellRankFlags6`, `SpellRankFlags7`, `SpellDescription0`, `SpellDescription1`, `SpellDescription2`, `SpellDescription3`, `SpellDescription4`, `SpellDescription5`, `SpellDescription6`, `SpellDescription7`, `SpellDescription8`, `SpellDescriptionFlags0`, `SpellDescriptionFlags1`, `SpellDescriptionFlags2`, `SpellDescriptionFlags3`, `SpellDescriptionFlags4`, `SpellDescriptionFlags5`, `SpellDescriptionFlags6`, `SpellDescriptionFlags7`, `SpellToolTip0`, `SpellToolTip1`, `SpellToolTip2`, `SpellToolTip3`, `SpellToolTip4`, `SpellToolTip5`, `SpellToolTip6`, `SpellToolTip7`, `SpellToolTip8`, `SpellToolTipFlags0`, `SpellToolTipFlags1`, `SpellToolTipFlags2`, `SpellToolTipFlags3`, `SpellToolTipFlags4`, `SpellToolTipFlags5`, `SpellToolTipFlags6`, `SpellToolTipFlags7`, `ManaCostPercentage`, `StartRecoveryCategory`, `StartRecoveryTime`, `MaximumTargetLevel`, `SpellFamilyName`, `SpellFamilyFlags`, `SpellFamilyFlags1`, `SpellFamilyFlags2`, `MaximumAffectedTargets`, `DamageClass`, `PreventionType`, `StanceBarOrder`, `EffectDamageMultiplier1`, `EffectDamageMultiplier2`, `EffectDamageMultiplier3`, `MinimumFactionId`, `MinimumReputation`, `RequiredAuraVision`, `TotemCategory1`, `TotemCategory2`, `AreaGroupID`, `SchoolMask`, `RuneCostID`, `SpellMissileID`, `PowerDisplayId`, `EffectBonusMultiplier1`, `EffectBonusMultiplier2`, `EffectBonusMultiplier3`, `SpellDescriptionVariableID`, `SpellDifficultyID`) VALUES
(60955, 0, 0, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 101, 0, 0, 1, 1, 21, 0, 0, 0, 0, 0, 13, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 6, 6, 0, 1, 1, 0, 0, 0, 0, -16, 24, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 87, 79, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 127, 127, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 153, 0, 50, 'Sacrifice of Blood', '', '', '', '', '', '', '', '', '', '', '', '', '', '', '', 16712190, '', '', '', '', '', '', '', '', '', '', '', '', '', '', '', '', 16712172, 'Damage taken reduced by $s1%, damage dealt increased by $s2%.', '', '', '', '', '', '', '', '', 0, 0, 0, 0, 0, 0, 0, 16712190, 'Damage taken reduced by $s1%, damage dealt increased by $s2%.', '', '', '', '', '', '', '', '', 0, 0, 0, 0, 0, 0, 0, 16712190, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 32, 0, 0, 0, 0, 0, 0, 0, 0);

-- Rank 2 differs from rank 1 in two numbers, so it is cloned rather than
-- transcribed as a second 234-column INSERT (the woa_2026_08_04_04.sql pattern).

DROP TEMPORARY TABLE IF EXISTS `woa_sob_tmp`;
CREATE TEMPORARY TABLE `woa_sob_tmp`
    AS SELECT * FROM `alonecraft_spell_dbc` WHERE `ID` = 60955;

UPDATE `woa_sob_tmp` SET `ID` = 60956,
    `EffectBasePoints1` = -31,
    `EffectBasePoints2` = 49;

INSERT INTO `alonecraft_spell_dbc` SELECT * FROM `woa_sob_tmp`;

DROP TEMPORARY TABLE `woa_sob_tmp`;


-- ---------------------------------------------------------------------------
-- 18703 / 18704: the talent itself
-- ---------------------------------------------------------------------------
-- Effect1 keeps its ADD_PCT_MODIFIER shape but changes what it modifies:
-- MiscValue 14 (SPELLMOD_COST) becomes 22 (SPELLMOD_DOT), and the amount flips
-- from -41/-81 to 24/49 (= +25/+50% after the DieSides 1 roll).  That is the
-- Health Funnel healing bonus.  The two dummy value-holders in Effects 2 and 3
-- are cleared -- their reader is gone.
--
-- SpellFamilyName is already 5 and Health Funnel's family flag is 0x01000000 on
-- all nine ranks, so EffectSpellClassMaskA1 keeps the value the talent shipped
-- with.  Per the trap at the top of woa_2026_08_02_03.sql the DBC stores masks
-- [effect][word], so effect 1's three words are the columns named A1/A2/A3;
-- 18703 also carried the flag in B1 for its second (cost) spellmod, and that one
-- is now dead and must be cleared with the rest of effect 2.
--
-- Core only calls HasAura() on these, and Effect1 is still an APPLY_AURA, so the
-- talent continues to exist as an aura on learn.
--
-- UPDATE rather than a full-row re-INSERT so nothing else set on these rows by
-- earlier files is reverted.

UPDATE `alonecraft_spell_dbc` SET
    `Effect1` = 6, `EffectApplyAuraName1` = 108, `EffectMiscValue1` = 22,
    `EffectBasePoints1` = 24, `EffectDieSides1` = 1, `EffectImplicitTargetA1` = 1,
    `EffectSpellClassMaskA1` = 16777216, `EffectSpellClassMaskA2` = 0, `EffectSpellClassMaskA3` = 0,
    `Effect2` = 0, `EffectApplyAuraName2` = 0, `EffectMiscValue2` = 0,
    `EffectBasePoints2` = 0, `EffectDieSides2` = 0, `EffectImplicitTargetA2` = 0,
    `EffectSpellClassMaskB1` = 0, `EffectSpellClassMaskB2` = 0, `EffectSpellClassMaskB3` = 0,
    `Effect3` = 0, `EffectApplyAuraName3` = 0, `EffectMiscValue3` = 0,
    `EffectBasePoints3` = 0, `EffectDieSides3` = 0, `EffectImplicitTargetA3` = 0,
    `EffectSpellClassMaskC1` = 0, `EffectSpellClassMaskC2` = 0, `EffectSpellClassMaskC3` = 0,
    `SpellDescription0` = 'Your Health Funnel transfers $s1% more health, and while you are channelling it the targeted demon takes $60955s1% less damage and deals $60955s2% more damage.'
WHERE `ID` = 18703;

UPDATE `alonecraft_spell_dbc` SET
    `Effect1` = 6, `EffectApplyAuraName1` = 108, `EffectMiscValue1` = 22,
    `EffectBasePoints1` = 49, `EffectDieSides1` = 1, `EffectImplicitTargetA1` = 1,
    `EffectSpellClassMaskA1` = 16777216, `EffectSpellClassMaskA2` = 0, `EffectSpellClassMaskA3` = 0,
    `Effect2` = 0, `EffectApplyAuraName2` = 0, `EffectMiscValue2` = 0,
    `EffectBasePoints2` = 0, `EffectDieSides2` = 0, `EffectImplicitTargetA2` = 0,
    `EffectSpellClassMaskB1` = 0, `EffectSpellClassMaskB2` = 0, `EffectSpellClassMaskB3` = 0,
    `Effect3` = 0, `EffectApplyAuraName3` = 0, `EffectMiscValue3` = 0,
    `EffectBasePoints3` = 0, `EffectDieSides3` = 0, `EffectImplicitTargetA3` = 0,
    `EffectSpellClassMaskC1` = 0, `EffectSpellClassMaskC2` = 0, `EffectSpellClassMaskC3` = 0,
    `SpellDescription0` = 'Your Health Funnel transfers $s1% more health, and while you are channelling it the targeted demon takes $60956s1% less damage and deals $60956s2% more damage.'
WHERE `ID` = 18704;


-- ---------------------------------------------------------------------------
-- Retire custom carrier 200411
-- ---------------------------------------------------------------------------
-- Allocated in woa_2026_08_02_02.sql, tooltip patched in woa_2026_08_03_18.sql.
-- Nothing casts it any more, so 200411 returns to the free pool alongside
-- 200425-200499.
--
-- The allocation table in woa_2026_08_02_02.sql is deliberately NOT edited to
-- say so: the updater tracks applied files by name AND hash, so a comment-only
-- edit to an applied file makes it look modified.  Same reason 08_02_03,
-- 08_02_06 and 08_03_18 are superseded here rather than rewritten in place.
--
-- woa_2026_08_03_18.sql's UPDATE of 200411's tooltip becomes a harmless 0-row
-- no-op on a fresh database: it sorts before this file, so it runs first.

DELETE FROM `alonecraft_spell_dbc` WHERE `ID` = 200411;


-- ---------------------------------------------------------------------------
-- spell_script_names: the module script on -755 loses its Sacrifice of Blood half
-- ---------------------------------------------------------------------------
-- WarlockHealthFunnel.cpp now carries only Mana Feed (talent 30326), which still
-- needs a per-tick hook, so the row stays but changes name.  Core's own
-- spell_warl_health_funnel registration is deliberately left in place -- it is
-- the delivery mechanism for the buffs above.

DELETE FROM `spell_script_names`
    WHERE `spell_id` = -755
      AND `ScriptName` IN ('spell_warl_sacrifice_of_blood', 'spell_warl_health_funnel_mana_feed');

INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(-755, 'spell_warl_health_funnel_mana_feed');
