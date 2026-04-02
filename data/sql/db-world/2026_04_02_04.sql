-- ============================================================
-- Fix Tidal Surge (200220): charge consumption + SpellClassMask
-- Aura 57 (MOD_SPELL_CRIT_CHANCE) is a global stat aura with no
-- SpellClassMask, so the engine never matches it against a cast
-- and charges are never consumed. Switch to ADD_FLAT_MODIFIER(107)
-- with SPELLMOD_CRITICAL_CHANCE(7) and mask targeting:
--   Healing Wave (0x40), Lesser Healing Wave (0x80),
--   Chain Heal (0x100), Riptide (flags2=0x10)
-- ============================================================
UPDATE `alonecraft_spell_dbc` SET
    `EffectApplyAuraName1` = 107,   -- ADD_FLAT_MODIFIER
    `EffectMiscValue1`     = 7,     -- SPELLMOD_CRITICAL_CHANCE
    `EffectSpellClassMaskA1` = 448, -- 0x1C0 = HW|LHW|CH
    `EffectSpellClassMaskB1` = 0,
    `EffectSpellClassMaskC1` = 16,  -- 0x10 = Riptide
    `ProcCharges`          = 1
WHERE `ID` = 200220;


-- ============================================================
-- Fix Tidal Waves passive (51562-51566): 20% damage AND healing
-- Effect1 was SPELL_AURA_MOD_DAMAGE_DONE (13) = flat +20 SP.
-- Change to MOD_DAMAGE_PERCENT_DONE (79) for 20% spell damage.
-- Add MOD_HEALING_DONE_PERCENT (136) on Effect3 for 20% healing.
-- MiscValue=126 = all magic schools (no physical).
-- ============================================================
UPDATE `alonecraft_spell_dbc` SET
    `EffectApplyAuraName1` = 79,  -- SPELL_AURA_MOD_DAMAGE_PERCENT_DONE
    `Effect3`              = 6,   -- SPELL_EFFECT_APPLY_AURA
    `EffectApplyAuraName3` = 136, -- SPELL_AURA_MOD_HEALING_DONE_PERCENT
    `EffectBasePoints3`    = 19,  -- 20%
    `EffectDieSides3`      = 1,
    `EffectMiscValue3`     = 126, -- all magic schools
    `EffectImplicitTargetA3` = 1  -- TARGET_UNIT_CASTER
WHERE `ID` IN (51562, 51563, 51564, 51565, 51566);
