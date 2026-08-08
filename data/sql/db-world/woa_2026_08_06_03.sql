-- ===========================================================================
-- Warlock (Affliction): Malfeasance tuning + Soulshatter clears stacks
-- ===========================================================================
--
-- Two changes on top of woa_2026_08_06_01.sql / woa_2026_08_06_02.sql:
--
--   1. Malfeasance's clear chance doubles, 10/20% -> 20/40%.
--   2. Casting Soulshatter clears Mark of Gul'dan stacks outright: 2 with
--      Fel Interdiction, 4 once Malfeasance is taken.
--
-- Soulshatter (29858) is a baseline ability on a 5 minute cooldown, so this
-- gives the stagger a genuine panic dump to sit alongside Drain Soul's slow
-- bleed -- and it gives Malfeasance a second axis to improve on, since the
-- rank 1 -> rank 2 step previously moved only the proc chance.
--
-- The stack count is NOT hardcoded in C++.  Both talents gain a second DUMMY
-- effect whose amount IS the number of stacks Soulshatter clears, so the
-- number lives in the DBC, drives the behaviour, and renders as $s2 in the
-- tooltip -- one value, one source.  The C++ reads EFFECT_1 of whichever
-- talent the player has, preferring Malfeasance.
--
-- These are UPDATEs, not DELETE + 234-column INSERT.  A full-row re-INSERT
-- would silently revert every other column to whatever the generating tool
-- thought it was, clobbering the fields the two earlier files set.
-- ===========================================================================


-- ============================================================
-- Fel Interdiction: Soulshatter clears 2 stacks
-- ============================================================
-- Effect 2 is a bare SPELL_AURA_DUMMY carrying the stack count.  DieSides 1
-- means CalcValue = BasePoints + 1, so BasePoints 1 -> 2 stacks, and $s2
-- renders 2.  Effect 1 (the stagger %) and the EFFECT_0 proc binding used by
-- the Drain Soul clear are untouched.
--
-- The class masks for effect 2 are forced to 0: Improved Howl of Terror had
-- EffectSpellClassMaskA2 = 8, and a leftover mask on a newly added effect is
-- the classic silent failure (see CLAUDE.md, SpellClassMask inheritance).

UPDATE `alonecraft_spell_dbc` SET
    `Effect2`                 = 6,   -- SPELL_EFFECT_APPLY_AURA
    `EffectApplyAuraName2`    = 4,   -- SPELL_AURA_DUMMY
    `EffectBasePoints2`       = 1,   -- + DieSides 1 = 2 stacks
    `EffectDieSides2`         = 1,
    `EffectImplicitTargetA2`  = 1,   -- TARGET_UNIT_CASTER
    `EffectMiscValue2`        = 0,
    `EffectSpellClassMaskA2`  = 0,
    `EffectSpellClassMaskB2`  = 0,
    `EffectSpellClassMaskC2`  = 0,
    `SpellDescription0`       = 'When using Fel Armor, $s1% of damage taken is converted to a Mark of Gul''dan, dealing damage periodically. Stacks can be removed by dealing damage with Drain Soul, and casting Soulshatter removes $s2 stacks.',
    `SpellToolTip0`           = 'When using Fel Armor, $s1% of damage taken is converted to a Mark of Gul''dan.'
WHERE `ID` = 30054;


-- ============================================================
-- Malfeasance: 20/40% clear chance, Soulshatter clears 4 stacks
-- ============================================================
-- EffectBasePoints1 9 -> 19 and 19 -> 39; with DieSides 1 that renders 20%
-- and 40%, matching the spell_proc Chance column below.
--
-- Effect 2 mirrors Fel Interdiction's: BasePoints 3 + DieSides 1 = 4 stacks.
-- The C++ prefers this value over Fel Interdiction's when both are present,
-- so Malfeasance replaces the 2 rather than adding to it.

UPDATE `alonecraft_spell_dbc` SET
    `EffectBasePoints1`       = 19,  -- + DieSides 1 = 20%
    `Effect2`                 = 6,
    `EffectApplyAuraName2`    = 4,
    `EffectBasePoints2`       = 3,   -- + DieSides 1 = 4 stacks
    `EffectDieSides2`         = 1,
    `EffectImplicitTargetA2`  = 1,
    `EffectMiscValue2`        = 0,
    `EffectSpellClassMaskA2`  = 0,
    `EffectSpellClassMaskB2`  = 0,
    `EffectSpellClassMaskC2`  = 0,
    `SpellDescription0`       = 'Damage from your Drain Life, Shadow Bolt and Haunt has a $s1% chance to clear a stack of Mark of Gul''dan, and your Soulshatter clears $s2 stacks instead of 2.',
    `SpellToolTip0`           = 'Damage from your Drain Life, Shadow Bolt and Haunt has a $s1% chance to clear a stack of Mark of Gul''dan.'
WHERE `ID` = 200521;

UPDATE `alonecraft_spell_dbc` SET
    `EffectBasePoints1`       = 39,  -- + DieSides 1 = 40%
    `Effect2`                 = 6,
    `EffectApplyAuraName2`    = 4,
    `EffectBasePoints2`       = 3,
    `EffectDieSides2`         = 1,
    `EffectImplicitTargetA2`  = 1,
    `EffectMiscValue2`        = 0,
    `EffectSpellClassMaskA2`  = 0,
    `EffectSpellClassMaskB2`  = 0,
    `EffectSpellClassMaskC2`  = 0,
    `SpellDescription0`       = 'Damage from your Drain Life, Shadow Bolt and Haunt has a $s1% chance to clear a stack of Mark of Gul''dan, and your Soulshatter clears $s2 stacks instead of 2.',
    `SpellToolTip0`           = 'Damage from your Drain Life, Shadow Bolt and Haunt has a $s1% chance to clear a stack of Mark of Gul''dan.'
WHERE `ID` = 200522;


-- ============================================================
-- spell_proc: double the Malfeasance clear chance
-- ============================================================
-- Only the Chance column moves; every other filter is as
-- woa_2026_08_06_02.sql set it.

UPDATE `spell_proc` SET `Chance` = 20 WHERE `SpellId` = 200521;
UPDATE `spell_proc` SET `Chance` = 40 WHERE `SpellId` = 200522;


-- ============================================================
-- Script registration: Soulshatter
-- ============================================================
-- Core already registers its own spell_warl_soulshatter on 29858 (the threat
-- drop).  AzerothCore allows several scripts per spell, so this is an
-- additional row rather than a replacement -- do NOT delete by spell_id here.
--
-- The script hooks AfterCast, not OnEffectHitTarget: core's threat-drop
-- effect only runs against a target that already has the caster on its threat
-- list, and clearing your own debuff should not depend on that.

DELETE FROM `spell_script_names` WHERE `ScriptName` = 'spell_warl_soulshatter_mark';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(29858, 'spell_warl_soulshatter_mark');
