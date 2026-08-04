-- Alonecraft 4.61 -- Fieldcraft becomes Heightened Senses, healing on dodge.
--
-- woa_2026_08_03_09.sql renamed Heightened Senses (30894/30895) to Fieldcraft
-- and gave it a chance per combo point spent on a finisher to heal;
-- woa_2026_08_04_22.sql made that heal 5% of maximum health. The payoff was
-- still gated behind spending a full finisher AND winning a roll, so the
-- talent contributed almost nothing to a normal rotation.
--
-- It now heals on every dodge, with no roll and no internal cooldown, which
-- pays out constantly while tanking -- where a solo rogue actually needs it.
-- The stock name fits that far better than Fieldcraft did, so it goes back.
--
--   Rank 1 (30894)  2% of maximum health per dodge
--   Rank 2 (30895)  4% of maximum health per dodge
--
-- NO C++.  The whole talent is DBC plus one spell_proc row.  BLOOD CRAZE
-- (16487) IS THE EXACT PRECEDENT: a taken-side proc aura on the player whose
-- EffectTriggerSpell is a self-targeted heal, with no script anywhere in core.
-- The chain here is the same, one link longer only because the amount is a
-- percentage that differs per rank:
--
--   30894/30895  aura 231 SPELL_AURA_PROC_TRIGGER_SPELL_WITH_VALUE
--                  -> passes its own amount (2 or 4) as base point 0
--   200501       effect 136 SPELL_EFFECT_HEAL_PCT
--                  -> heals CountPctFromMaxHealth(that value) (SpellEffects.cpp:1602)
--
-- Using WITH_VALUE (231) rather than plain PROC_TRIGGER_SPELL (42) is what
-- keeps both ranks on a single heal spell: the per-rank number stays in the
-- talent's own base points, so retuning is one UPDATE here and nothing else.
--
-- Target note: HandleProcTriggerSpellWithValueAuraProc (SpellAuraEffects.cpp:6925)
-- casts at the *attacker*, because on a taken-side proc the rogue is not the
-- actor. That is harmless -- 200501's EffectImplicitTargetA1 is 1
-- (TARGET_UNIT_CASTER), so the effect resolves back to the rogue. Blood Craze's
-- triggered spell (16488) is built exactly this way and goes through the
-- identical code path (SpellAuraEffects.cpp:6908).
--
-- DODGE IS A HIT MASK, NOT A PROC FLAG (SpellMgr.h:254). ProcFlags picks the
-- event side, HitMask says it was a dodge:
--
--   ProcFlags = 40  8  PROC_FLAG_TAKEN_MELEE_AUTO_ATTACK      (dodged a swing)
--                   32 PROC_FLAG_TAKEN_SPELL_MELEE_DMG_CLASS  (dodged an ability)
--   HitMask   = 16  PROC_HIT_DODGE
--
-- HitMask MUST be set: left at 0 it defaults to PROC_HIT_NORMAL|PROC_HIT_CRITICAL
-- (SpellMgr.cpp:931) and would never fire on a dodge.
--
-- SpellPhaseMask is 0, NOT the usual 2. REQ_SPELL_PHASE_PROC_FLAG_MASK is
-- `SPELL_PROC_FLAG_MASK & DONE_HIT_PROC_FLAG_MASK` (SpellMgr.h:184) -- DONE
-- side only. Both flags here are TAKEN, so the phase check at SpellMgr.cpp:917
-- never runs and a non-zero value only trips the sql.sql warning at
-- SpellMgr.cpp:2116.
--
-- EffectBasePoints1 is 1/3 with EffectDieSides1 = 1, so the amount is 2/4.
-- $s1 renders that same BasePoints + max(1, DieSides), so the description
-- tracks the DBC instead of hardcoding the number.
--
-- Ordering note: UPDATEs only, no full-row re-INSERT, so this cannot clobber
-- the other columns woa_2026_08_03_09.sql set.

-- Heightened Senses rank 1 -- 2% of maximum health per dodge
UPDATE `alonecraft_spell_dbc` SET
    `SpellName0` = 'Heightened Senses',
    `EffectApplyAuraName1` = 231,
    `EffectTriggerSpell1` = 200501,
    `EffectBasePoints1` = 1,
    `EffectDieSides1` = 1,
    `Effect2` = 0,
    `EffectApplyAuraName2` = 0,
    `EffectImplicitTargetA2` = 0,
    `EffectDieSides2` = 0,
    `EffectBasePoints2` = 0,
    `EffectMiscValue2` = 0,
    `SpellDescription0` = 'When you dodge an attack, you restore $s1% of your maximum health.'
WHERE `ID` = 30894;

-- Heightened Senses rank 2 -- 4% of maximum health per dodge
UPDATE `alonecraft_spell_dbc` SET
    `SpellName0` = 'Heightened Senses',
    `EffectApplyAuraName1` = 231,
    `EffectTriggerSpell1` = 200501,
    `EffectBasePoints1` = 3,
    `EffectDieSides1` = 1,
    `Effect2` = 0,
    `EffectApplyAuraName2` = 0,
    `EffectImplicitTargetA2` = 0,
    `EffectDieSides2` = 0,
    `EffectBasePoints2` = 0,
    `EffectMiscValue2` = 0,
    `SpellDescription0` = 'When you dodge an attack, you restore $s1% of your maximum health.'
WHERE `ID` = 30895;

-- Heal carrier (200501). Effect 10 SPELL_EFFECT_HEAL -> 136 SPELL_EFFECT_HEAL_PCT:
-- the amount is no longer an absolute number computed elsewhere, it is the
-- percentage handed over by the talent's aura. Base points stay 0 because
-- CastCustomSpell overrides them. Attributes is already 0, so the PASSIVE bit
-- that silently no-ops a self-cast (see 2026_03_31_05.sql:114) is clear.
UPDATE `alonecraft_spell_dbc` SET
    `SpellName0` = 'Heightened Senses',
    `Effect1` = 136,
    `EffectBasePoints1` = 0,
    `EffectDieSides1` = 0,
    `EffectImplicitTargetA1` = 1
WHERE `ID` = 200501;

-- Proc: fire on any dodged melee swing or melee ability, every time.
DELETE FROM `spell_proc` WHERE `SpellId` IN (30894, 30895);
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(30894, 0, 0, 0, 0, 0, 40, 0, 0, 16, 0, 0, 0, 100, 0, 0),
(30895, 0, 0, 0, 0, 0, 40, 0, 0, 16, 0, 0, 0, 100, 0, 0);

-- No script registration: the talent is entirely data-driven. This DELETE is
-- kept because an earlier revision of this file did register one.
DELETE FROM `spell_script_names` WHERE `ScriptName` = 'spell_rog_heightened_senses';
