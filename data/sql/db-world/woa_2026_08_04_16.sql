-- Alonecraft 4.61 -- Molten Fury: the attributes Unholy Blight carries.
--
--   Molten Fury (200420) is a precomputed-amount DoT, the same construction as
--   Unholy Blight's DoT (50536): a script works out the per-tick damage from a
--   hit that has already been fully modified, and pushes it in as base points.
--   Diffing the two rows, 50536 carries three attributes that 200420 was
--   cloned without.
--
--   SPELL_ATTR2_CANT_CRIT (0x20000000)
--     The Shadow Bolt that seeded the DoT already rolled its own crit, and the
--     share is taken from the post-crit damage.  Letting the ticks crit again
--     pays the crit twice.
--
--   SPELL_ATTR3_ALWAYS_HIT (0x00040000)
--     The Shadow Bolt already landed -- its damage is the input.  Without this
--     the follow-up cast rolls its own hit check and can miss outright, and the
--     DoT silently never appears.
--
--   SPELL_ATTR4_IGNORE_DAMAGE_TAKEN_MODIFIERS (0x00000100), "deals fixed
--   damage"
--     This is the one that makes the number match the tooltip.  The per-tick
--     amount is derived from damage that already went through spell power,
--     talents and the target's modifiers; without the attribute every tick is
--     put through the damage pipeline a second time, so "10% of the Shadow
--     Bolt" is not what actually lands.
--
--   Also clearing MaximumLevel.  It is 9, inherited from whatever 200420 was
--   cloned off, and feeds Unit::CalculateLevelPenalty (Unit.cpp:3236) --
--   currently inert only because SpellLevel happens to be 0.  50536 has 0.
--
--   Left alone deliberately: InterruptFlags 15, StartRecoveryCategory 133,
--   StartRecoveryTime 1500, ManaCostPercentage 9.  All are clone leftovers and
--   all are meaningless for a triggered aura-only spell -- untangling them is
--   not worth a behaviour risk here.

UPDATE `alonecraft_spell_dbc` SET
    `AttributesEx2` = `AttributesEx2` | 0x20000000,
    `AttributesEx3` = `AttributesEx3` | 0x00040000,
    `AttributesEx4` = `AttributesEx4` | 0x00000100,
    `MaximumLevel`  = 0
WHERE `ID` = 200420;
