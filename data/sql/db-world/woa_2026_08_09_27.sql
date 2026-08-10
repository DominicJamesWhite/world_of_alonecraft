-- ============================================================
-- Warrior (Protection): Concussion Blow -> Barricade
-- ============================================================
-- TODO.md: "Concussion Blow (1, 4) replaced with Barricade: Toggled.  Costs 5
--           rage to activate and 5 rage per second thereafter; while active
--           your Shield Slam and Revenge deal 50% more damage and you take 10%
--           less damage, but they generate no rage."
--
-- Protection had no rage spender.  Shield Slam and Revenge were turned into
-- rage *generators* (woa_2026_08_08_03.sql), which made the surplus worse, and
-- Concussion Blow is a 30 second stun on a tree that fights stun-immune
-- targets.  Barricade turns the surplus into damage and mitigation by shutting
-- off the two faucets it buffs, so it is only sustainable while auto-attack and
-- damage-taken rage cover the drain.
--
-- Spells:
--   200651  = Barricade (the ability AND its own buff; three effects is
--             exactly enough, so no second spell is needed)
-- Talent: 152, position unchanged (TabID 163, Tier 4, Column 1, Flags 1).
--         Only SpellRank_1 moves, 12809 -> 200651.  Spell 12809 itself is left
--         intact -- NPCs still use it; the talent simply stops teaching it.
--
-- The rage suppression is NOT in this file, and deliberately so.  Two reasons
-- an ADD_PCT_MODIFIER at -100% cannot do it:
--   1. The two spells carry the energize on different effect indices --
--      Shield Slam on Effect3, Revenge on Effect2 -- so no single SPELLMOD
--      reaches both, and SPELLMOD_ALL_EFFECTS would zero the damage too.
--   2. Player::ApplySpellMod (Player.cpp:9896) accumulates PCT mods
--      *additively* for every op except SPELLMOD_DAMAGE and SPELLMOD_DOT.
--      Sword and Board (50227) grants +100% SPELLMOD_EFFECT3 on Shield Slam's
--      energize, so +100 and -100 would sum to totalmul == 1.0 and hand back
--      the full rage, silently.
-- spell_warr_barricade_rage_suppress does it in C++ instead, on the same
-- PreventHitDefaultEffect / SPELL_EFFECT_ENERGIZE hook core uses in
-- boss_icecrown_gunship_battle.cpp:2341.
--
-- SPELLMOD_DAMAGE *is* multiplicative in that same function, so the +50% half
-- stays here as Effect1.
-- ============================================================

-- ============================================================
-- 200651: Barricade
-- ============================================================
-- Cloned from Shield Wall 871 -- instant, self-targeted, already a Protection
-- ability.  Four donor fields had to be undone:
--   Stances 131072 -> 0.  Shield Wall is Defensive Stance only.  Left in, the
--     button greys out the moment you leave it, which is the exact failure
--     Victory Rush shipped with (TODO.md 4.66).
--   EquippedItemClass 4 / SubClassMask 64 -> -1 / 0.  The donor requires a
--     shield.  Shield Slam and Revenge already do, so the restriction adds
--     nothing but a second failure message.
--   Category 132 / CategoryRecoveryTime 12000 and RecoveryTime -> 0.  A toggle
--     has no cooldown.
--   SpellFamilyFlags 8192 -> 0.  SpellFamilyName stays 4 because the Effect1
--     spellmod has to match warrior spells, but Barricade must not itself feed
--     the family-mask procs this tree is full of.
--
-- StartRecoveryCategory / StartRecoveryTime are 0: the toggle is off the GCD,
-- so flipping it never costs a Shield Slam.
--
-- ManaCost 50 is 5 rage (rage is stored x10).  That single column buys both the
-- activation cost and the "not enough rage" cast failure, with no C++.
--
-- Attributes 16 is SPELL_ATTR0_IS_ABILITY and NOT 0x80000000
-- SPELL_ATTR0_NO_AURA_CANCEL, so right-clicking the buff cancels it.
--
-- Effects:
--   1  ADD_PCT_MODIFIER (108) / SPELLMOD_DAMAGE (MiscValue 0), +50%, masked to
--      Revenge (ClassMaskA1 1024) and Shield Slam (ClassMaskA2 512)
--
-- Read those two mask columns carefully, because the naming invites exactly
-- one mistake.  In EffectSpellClassMask<letter><digit> the LETTER is the
-- effect index and the DIGIT is the word of that effect's flag96 --
-- DBCStructure.h:1750 stores the field as
-- std::array<flag96, MAX_SPELL_EFFECTS>, which is effect-major.  So a
-- modifier living on Effect1 keeps its whole mask in A1/A2/A3; B* and C*
-- belong to Effects 2 and 3.  Revenge is warrior SpellFamilyFlags word0 bit
-- 1024 and Shield Slam is word1 bit 512, so both bits go on A.  Sword and
-- Board (50227) is the proof: one modifier on Effect1 that affects Shield
-- Slam, and it carries 512 in A2.
--
-- Getting this wrong is not a no-op in either direction.  Writing 512 into B1
-- would have left Shield Slam unbuffed; and had A1 been 0 as well, an
-- all-zero flag96 is falsy, so SpellInfo::IsAffected (SpellInfo.cpp:1333)
-- skips the mask test entirely and the modifier applies to EVERY warrior
-- spell.
--   2  MOD_DAMAGE_PERCENT_TAKEN (87), -10%, MiscValue 127 = all schools.  This
--      aura reads MiscValue as a school *mask*, so 0 would match nothing.
--   3  PERIODIC_DUMMY (226) every 1000ms, amount 50 = the 5 rage upkeep.  The
--      script reads the cost off this effect rather than hardcoding it, so
--      retuning the drain is a one-column UPDATE.
--
-- Duration is permanent (index 21).  Core's built-in ManaPerSecond drain is not
-- an option for exactly that reason -- Aura::Update nests it inside
-- `if (m_duration > 0)`, so it never runs on a permanent aura.  The dummy
-- heartbeat is the same shape Metamorphosis uses.
--
-- Both text fields are set: the buff bar renders SpellToolTip, not
-- SpellDescription, and a clone inherits the donor's tooltip verbatim.

DELETE FROM `alonecraft_spell_dbc` WHERE `ID` = 200651;
INSERT INTO `alonecraft_spell_dbc` (`ID`, `Category`, `Dispel`, `Mechanic`, `Attributes`, `AttributesEx`, `AttributesEx2`, `AttributesEx3`, `AttributesEx4`, `AttributesEx5`, `AttributesEx6`, `AttributesEx7`, `Stances`, `Unknown1`, `StancesNot`, `Unknown2`, `Targets`, `TargetCreatureType`, `RequiresSpellFocus`, `FacingCasterFlags`, `CasterAuraState`, `TargetAuraState`, `CasterAuraStateNot`, `TargetAuraStateNot`, `CasterAuraSpell`, `TargetAuraSpell`, `ExcludeCasterAuraSpell`, `ExcludeTargetAuraSpell`, `CastingTimeIndex`, `RecoveryTime`, `CategoryRecoveryTime`, `InterruptFlags`, `AuraInterruptFlags`, `ChannelInterruptFlags`, `ProcFlags`, `ProcChance`, `ProcCharges`, `MaximumLevel`, `BaseLevel`, `SpellLevel`, `DurationIndex`, `PowerType`, `ManaCost`, `ManaCostPerLevel`, `ManaPerSecond`, `ManaPerSecondPerLevel`, `RangeIndex`, `Speed`, `ModalNextSpell`, `StackAmount`, `Totem1`, `Totem2`, `Reagent1`, `Reagent2`, `Reagent3`, `Reagent4`, `Reagent5`, `Reagent6`, `Reagent7`, `Reagent8`, `ReagentCount1`, `ReagentCount2`, `ReagentCount3`, `ReagentCount4`, `ReagentCount5`, `ReagentCount6`, `ReagentCount7`, `ReagentCount8`, `EquippedItemClass`, `EquippedItemSubClassMask`, `EquippedItemInventoryTypeMask`, `Effect1`, `Effect2`, `Effect3`, `EffectDieSides1`, `EffectDieSides2`, `EffectDieSides3`, `EffectRealPointsPerLevel1`, `EffectRealPointsPerLevel2`, `EffectRealPointsPerLevel3`, `EffectBasePoints1`, `EffectBasePoints2`, `EffectBasePoints3`, `EffectMechanic1`, `EffectMechanic2`, `EffectMechanic3`, `EffectImplicitTargetA1`, `EffectImplicitTargetA2`, `EffectImplicitTargetA3`, `EffectImplicitTargetB1`, `EffectImplicitTargetB2`, `EffectImplicitTargetB3`, `EffectRadiusIndex1`, `EffectRadiusIndex2`, `EffectRadiusIndex3`, `EffectApplyAuraName1`, `EffectApplyAuraName2`, `EffectApplyAuraName3`, `EffectAmplitude1`, `EffectAmplitude2`, `EffectAmplitude3`, `EffectMultipleValue1`, `EffectMultipleValue2`, `EffectMultipleValue3`, `EffectChainTarget1`, `EffectChainTarget2`, `EffectChainTarget3`, `EffectItemType1`, `EffectItemType2`, `EffectItemType3`, `EffectMiscValue1`, `EffectMiscValue2`, `EffectMiscValue3`, `EffectMiscValueB1`, `EffectMiscValueB2`, `EffectMiscValueB3`, `EffectTriggerSpell1`, `EffectTriggerSpell2`, `EffectTriggerSpell3`, `EffectPointsPerComboPoint1`, `EffectPointsPerComboPoint2`, `EffectPointsPerComboPoint3`, `EffectSpellClassMaskA1`, `EffectSpellClassMaskA2`, `EffectSpellClassMaskA3`, `EffectSpellClassMaskB1`, `EffectSpellClassMaskB2`, `EffectSpellClassMaskB3`, `EffectSpellClassMaskC1`, `EffectSpellClassMaskC2`, `EffectSpellClassMaskC3`, `SpellVisual1`, `SpellVisual2`, `SpellIconID`, `ActiveIconID`, `SpellPriority`, `SpellName0`, `SpellName1`, `SpellName2`, `SpellName3`, `SpellName4`, `SpellName5`, `SpellName6`, `SpellName7`, `SpellName8`, `SpellNameFlag0`, `SpellNameFlag1`, `SpellNameFlag2`, `SpellNameFlag3`, `SpellNameFlag4`, `SpellNameFlag5`, `SpellNameFlag6`, `SpellNameFlag7`, `SpellRank0`, `SpellRank1`, `SpellRank2`, `SpellRank3`, `SpellRank4`, `SpellRank5`, `SpellRank6`, `SpellRank7`, `SpellRank8`, `SpellRankFlags0`, `SpellRankFlags1`, `SpellRankFlags2`, `SpellRankFlags3`, `SpellRankFlags4`, `SpellRankFlags5`, `SpellRankFlags6`, `SpellRankFlags7`, `SpellDescription0`, `SpellDescription1`, `SpellDescription2`, `SpellDescription3`, `SpellDescription4`, `SpellDescription5`, `SpellDescription6`, `SpellDescription7`, `SpellDescription8`, `SpellDescriptionFlags0`, `SpellDescriptionFlags1`, `SpellDescriptionFlags2`, `SpellDescriptionFlags3`, `SpellDescriptionFlags4`, `SpellDescriptionFlags5`, `SpellDescriptionFlags6`, `SpellDescriptionFlags7`, `SpellToolTip0`, `SpellToolTip1`, `SpellToolTip2`, `SpellToolTip3`, `SpellToolTip4`, `SpellToolTip5`, `SpellToolTip6`, `SpellToolTip7`, `SpellToolTip8`, `SpellToolTipFlags0`, `SpellToolTipFlags1`, `SpellToolTipFlags2`, `SpellToolTipFlags3`, `SpellToolTipFlags4`, `SpellToolTipFlags5`, `SpellToolTipFlags6`, `SpellToolTipFlags7`, `ManaCostPercentage`, `StartRecoveryCategory`, `StartRecoveryTime`, `MaximumTargetLevel`, `SpellFamilyName`, `SpellFamilyFlags`, `SpellFamilyFlags1`, `SpellFamilyFlags2`, `MaximumAffectedTargets`, `DamageClass`, `PreventionType`, `StanceBarOrder`, `EffectDamageMultiplier1`, `EffectDamageMultiplier2`, `EffectDamageMultiplier3`, `MinimumFactionId`, `MinimumReputation`, `RequiredAuraVision`, `TotemCategory1`, `TotemCategory2`, `AreaGroupID`, `SchoolMask`, `RuneCostID`, `SpellMissileID`, `PowerDisplayId`, `EffectBonusMultiplier1`, `EffectBonusMultiplier2`, `EffectBonusMultiplier3`, `SpellDescriptionVariableID`, `SpellDifficultyID`) VALUES
(200651, 0, 0, 0, 16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 101, 0, 0, 30, 30, 21, 1, 50, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 6, 6, 6, 1, 1, 1, 0, 0, 0, 49, -11, 49, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 108, 87, 226, 0, 0, 1000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 127, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1024, 512, 0, 0, 0, 0, 0, 0, 0, 345, 0, 276, 0, 50, 'Barricade', '', '', '', '', '', '', '', '', '', '', '', '', '', '', '', 16712190, '', '', '', '', '', '', '', '', '', '', '', '', '', '', '', '', 16712188, 'Brace behind your shield.  While active, your Shield Slam and Revenge deal $s1% additional damage and you take $s2% less damage, but Shield Slam and Revenge generate no rage and you burn $/10;s3 rage per second.  Ends when your rage runs out.', '', '', '', '', '', '', '', '', 0, 0, 0, 0, 0, 0, 0, 16712190, 'Shield Slam and Revenge deal $s1% additional damage and generate no rage.  Damage taken reduced by $s2%.  Draining $/10;s3 rage per second.', '', '', '', '', '', '', '', '', 0, 0, 0, 0, 0, 0, 0, 16712190, 0, 0, 0, 0, 4, 0, 0, 0, 0, 2, 2, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0);

-- ============================================================
-- Script registrations
-- ============================================================
-- Negative ids match all ranks: -23922 is every Shield Slam, -6572 every
-- Revenge.  The suppressor hooks SPELL_EFFECT_ENERGIZE with EFFECT_ALL, so it
-- does not care that the two spells put the energize on different indices.

DELETE FROM `spell_script_names` WHERE `ScriptName` IN ('spell_warr_barricade', 'spell_warr_barricade_rage_suppress');
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(200651, 'spell_warr_barricade'),
(-23922, 'spell_warr_barricade_rage_suppress'),
(-6572, 'spell_warr_barricade_rage_suppress');

-- ============================================================
-- Talent 152: Concussion Blow -> Barricade
-- ============================================================
-- Position, prerequisites and Flags are unchanged; only SpellRank_1 moves.
-- Flags 1 keeps it an active ability that is taught into the spellbook.
-- build_dbc.py re-sorts talent records by (TabID, TierID, ColumnIndex), so no
-- manual ordering is needed here -- but note that getting that ordering wrong
-- blanks the entire talent tab, not just this talent.

DELETE FROM `talent_dbc` WHERE `ID` = 152;
INSERT INTO `talent_dbc` (
    `ID`, `TabID`, `TierID`, `ColumnIndex`,
    `SpellRank_1`, `SpellRank_2`, `SpellRank_3`, `SpellRank_4`, `SpellRank_5`,
    `SpellRank_6`, `SpellRank_7`, `SpellRank_8`, `SpellRank_9`,
    `PrereqTalent_1`, `PrereqTalent_2`, `PrereqTalent_3`,
    `PrereqRank_1`, `PrereqRank_2`, `PrereqRank_3`,
    `Flags`, `RequiredSpellID`, `CategoryMask_1`, `CategoryMask_2`
) VALUES (
    152, 163, 4, 1,
    200651, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    1, 0, 0, 0
);
