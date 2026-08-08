-- ===========================================================================
-- Warlock (Affliction): Malfeasance also shortens Soulshatter's cooldown
-- ===========================================================================
--
-- Soulshatter is the panic dump for a full Mark of Gul'dan pool, but its base
-- cooldown is 3 minutes (RecoveryTime 180000 on spell 29858 -- the "5 minute"
-- figure in the old WarlockFelInterdiction.cpp comment was wrong).  Three
-- minutes is long enough that the button is effectively absent from most
-- fights, so the 4-stack clear Malfeasance already grants is a bonus you
-- rarely get to spend.
--
-- Malfeasance now carries the cooldown as a second Soulshatter benefit:
--
--   Rank 1: -60 sec  -> 2 minutes
--   Rank 2: -120 sec -> 1 minute
--
-- At full rank that puts it roughly on the cadence of the Mark pool itself,
-- which is the point: the dump should be available about as often as the
-- pool becomes dangerous.
--
-- Why C++ and not a SPELLMOD_COOLDOWN aura: spell modifiers only match spells
-- through SpellFamilyName + SpellClassMask, and Soulshatter (29858) has
-- SpellFamilyName 0 with no family flags, so no warlock modifier can ever
-- affect it.  Giving it family 5 to make a modifier work would also opt it in
-- to every other family-5 proc row with zeroed masks.  The existing
-- spell_warl_soulshatter_mark script already runs AfterCast on this exact
-- spell, so the reduction is applied there instead.
--
-- The amount still lives in the DBC (EFFECT_2 base points, in seconds) rather
-- than in the script, so the number the tooltip prints as $s3 is the same one
-- the server applies -- the same arrangement as the stack count on EFFECT_1.
-- ===========================================================================


-- ============================================================
-- Malfeasance: new EFFECT_2 carrying the cooldown reduction
-- ============================================================
-- DBC "Effect3" is EFFECT_2 in C++.  SPELL_AURA_DUMMY (4) with no handler is
-- inert -- it exists purely to hold the value and to give the tooltip a $s3.
--
-- BasePoints are N-1 because CalcValue = BasePoints + max(1, DieSides): 59+1
-- renders and calculates as 60, 119+1 as 120.
--
-- ImplicitTargetA3 = 1 (TARGET_UNIT_CASTER), matching the other two effects.
-- The SpellClassMask columns for effect 3 are already 0 on both rows, so
-- there is no inherited mask to zero out.
--
-- UPDATE, not DELETE + 234-column INSERT: a full-row re-INSERT would revert
-- everything woa_2026_08_06_01..04.sql set on these rows.

UPDATE `alonecraft_spell_dbc` SET
    `Effect3`                = 6,
    `EffectApplyAuraName3`   = 4,
    `EffectDieSides3`        = 1,
    `EffectBasePoints3`      = 59,
    `EffectImplicitTargetA3` = 1
WHERE `ID` = 200521;

UPDATE `alonecraft_spell_dbc` SET
    `Effect3`                = 6,
    `EffectApplyAuraName3`   = 4,
    `EffectDieSides3`        = 1,
    `EffectBasePoints3`      = 119,
    `EffectImplicitTargetA3` = 1
WHERE `ID` = 200522;


-- ============================================================
-- Tooltips
-- ============================================================
-- The talent pane description gets all three benefits.  The buff-bar tooltip
-- stays on the passive proc alone -- the Soulshatter clauses describe what
-- happens when you press a different button, which is spellbook material,
-- not hover-over-the-buff material.

UPDATE `alonecraft_spell_dbc` SET
    `SpellDescription0` = 'Your periodic damage has a $s1% chance to clear a stack of Mark of Gul''dan. Your Soulshatter clears $s2 stacks instead of 2, and its cooldown is reduced by $s3 sec.'
WHERE `ID` IN (200521, 200522);
