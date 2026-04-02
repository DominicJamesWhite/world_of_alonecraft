-- ============================================================
-- Fix Healing Focus buffs: proper ADD_PCT_MODIFIER + ProcCharges
-- Same pattern as Gift of Nature (200004) / Ancestral Fury (200210)
-- ============================================================

-- 200211 (Surging Focus) — instant free Healing Wave, 1 charge
--   Effect1: ADD_PCT_MODIFIER, SPELLMOD_COST (14), BP=-101 (-100%)
--   Effect2: ADD_PCT_MODIFIER, SPELLMOD_CASTING_TIME (10), BP=-101 (-100%)
--   SpellClassMaskA = 64 (Healing Wave = 0x40)
UPDATE `alonecraft_spell_dbc` SET
    `EffectApplyAuraName1` = 108,  -- ADD_PCT_MODIFIER
    `EffectMiscValue1`     = 14,   -- SPELLMOD_COST
    `EffectBasePoints1`    = -101,
    `EffectDieSides1`      = 1,
    `EffectSpellClassMaskA1` = 64, -- Healing Wave
    `EffectImplicitTargetA1` = 1,  -- TARGET_UNIT_CASTER
    `Effect2`              = 6,    -- SPELL_EFFECT_APPLY_AURA
    `EffectApplyAuraName2` = 108,  -- ADD_PCT_MODIFIER
    `EffectMiscValue2`     = 10,   -- SPELLMOD_CASTING_TIME
    `EffectBasePoints2`    = -101,
    `EffectDieSides2`      = 1,
    `EffectSpellClassMaskA2` = 64, -- Healing Wave
    `EffectImplicitTargetA2` = 1,  -- TARGET_UNIT_CASTER
    `ProcCharges`          = 1
WHERE `ID` = 200211;

-- 200212 (Focused Assault) — +25% damage on next 5 casts, 5 charges
--   Effect1: ADD_PCT_MODIFIER, SPELLMOD_DAMAGE (0), BP=24 (+25%)
--   SpellClassMaskA = 0x90100003 = LB|CL|ES|FlS|FrS (signed: -1877999613)
--   SpellClassMaskB = 0x1000 = Lava Burst
UPDATE `alonecraft_spell_dbc` SET
    `EffectApplyAuraName1` = 108,  -- ADD_PCT_MODIFIER
    `EffectMiscValue1`     = 0,    -- SPELLMOD_DAMAGE
    `EffectBasePoints1`    = 24,
    `EffectDieSides1`      = 1,
    `EffectSpellClassMaskA1` = -1877999613, -- 0x90100003
    `EffectSpellClassMaskB1` = 4096,        -- 0x1000 Lava Burst
    `EffectImplicitTargetA1` = 1,  -- TARGET_UNIT_CASTER
    `ProcCharges`          = 5
WHERE `ID` = 200212;
