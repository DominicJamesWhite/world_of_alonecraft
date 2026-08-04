-- Alonecraft 4.61 -- Fieldcraft heals off max health, not damage done.
--
-- woa_2026_08_03_09.sql gave Fieldcraft (30894/30895) "restore health equal to
-- 10% of the damage done". Several of the finishers spell_rog_finishers is
-- registered on -- Slice and Dice, Expose Armor, Kidney Shot -- deal little or
-- no direct damage, so the proc fired and healed for almost nothing. The
-- talent's value tracked which finisher was used rather than the talent.
--
-- The heal is now a flat 5% of maximum health on a successful proc. The
-- trigger is untouched: still a 10/20% chance per combo point spent, so a
-- 5 point finisher procs half the time at rank 1 and every time at rank 2.
--
-- The percentage lives in a second dummy effect rather than in C++ so
-- retuning stays a DBC edit, matching how the per-rank chance is already
-- stored in EFFECT_0. RogueFinishers.cpp reads it back with
-- GetAuraEffect(...)->GetAmount() on EFFECT_1.
--
--   Effect2                = 6  SPELL_EFFECT_APPLY_AURA
--   EffectApplyAuraName2   = 4  SPELL_AURA_DUMMY (inert; read by the script)
--   EffectImplicitTargetA2 = 1  TARGET_UNIT_CASTER, as effect 1
--   EffectDieSides2        = 1  so the amount is BasePoints + 1
--   EffectBasePoints2      = 4  -> amount 5, i.e. 5% of maximum health
--
-- $s2 renders that same BasePoints + max(1, DieSides), so the description
-- tracks the DBC instead of hardcoding the number the way the old text did.
--
-- Ordering note: UPDATEs only, no full-row re-INSERT, so this cannot clobber
-- the other columns woa_2026_08_03_09.sql set.

-- Fieldcraft rank 1 -- 10% chance per combo point
UPDATE `alonecraft_spell_dbc` SET
    `Effect2` = 6,
    `EffectApplyAuraName2` = 4,
    `EffectImplicitTargetA2` = 1,
    `EffectDieSides2` = 1,
    `EffectBasePoints2` = 4,
    `EffectMiscValue2` = 0,
    `SpellDescription0` = 'Your finishing moves have a $s1% chance per combo point spent to restore $s2% of your maximum health.'
WHERE `ID` = 30894;

-- Fieldcraft rank 2 -- 20% chance per combo point
UPDATE `alonecraft_spell_dbc` SET
    `Effect2` = 6,
    `EffectApplyAuraName2` = 4,
    `EffectImplicitTargetA2` = 1,
    `EffectDieSides2` = 1,
    `EffectBasePoints2` = 4,
    `EffectMiscValue2` = 0,
    `SpellDescription0` = 'Your finishing moves have a $s1% chance per combo point spent to restore $s2% of your maximum health.'
WHERE `ID` = 30895;

-- Fieldcraft heal (200501) -- amount still supplied by CastCustomSpell, only
-- the wording changes.
UPDATE `alonecraft_spell_dbc` SET
    `SpellDescription0` = 'Restores a portion of your maximum health.'
WHERE `ID` = 200501;
