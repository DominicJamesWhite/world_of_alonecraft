-- ===========================================================================
-- Warlock / Demonology: spell_pet_auras
-- ===========================================================================
--
-- Fourth file of the Demonology batch.  Wires each talent's dummy anchor to
-- the custom aura the demon should receive.
--
-- Path: talent dummy effect -> AuraEffect::HandleAuraDummy
-- (SpellAuraEffects.cpp:5171) -> Unit::AddPetAura (Unit.cpp:13746) ->
-- Pet::CastPetAuras (Pet.cpp:2354).  Applied on summon and on talent apply,
-- removed on talent removal, so respecs and re-summons need no bookkeeping.
--
-- COLUMN NOTES
--
--   `effectId` is ZERO-BASED -- it indexes SpellInfo::Effects[], so the DBC's
--   "Effect2" is effectId 1.  SpellMgr.cpp:2470 rejects any row whose target
--   effect is neither SPELL_EFFECT_DUMMY nor APPLY_AURA/SPELL_AURA_DUMMY, and
--   logs "does not have dummy aura or dummy effect".
--
--   `pet` is a creature entry, or 0 meaning "any pet".  PetAura::GetAura
--   (SpellMgr.h:483) looks up the exact entry first and falls back to the 0
--   row, so specific and catch-all rows can coexist.
--
--   `aura` amounts are NOT taken from here.  Unit::CastPetAura (Unit.cpp:13770)
--   special-cases only spell 35696 for custom base points; everything else is
--   a bare CastSpell.  All amounts are computed in DoEffectCalcAmount by
--   reading the owner's talent rank.
--
--   ONE AURA PER (spell, effectId, pet).  The map key is (spell << 8) + eff
--   (SpellMgr.cpp:2461) and PetAura::AddAura stores one aura per pet entry, so
--   two rows sharing a triple silently overwrite -- only the second survives.
--   This is why Fel Attunement anchors its two auras on two DIFFERENT talent
--   effects rather than both on Effect1.
--
-- Demon entries: 416 Imp, 417 Felhunter, 1860 Voidwalker, 1863 Succubus,
-- 17252 Felguard.  (There is no Incubus in creature_template.)
--
-- ===========================================================================

DELETE FROM `spell_pet_auras` WHERE `aura` BETWEEN 200406 AND 200424;

INSERT INTO `spell_pet_auras` (`spell`, `effectId`, `pet`, `aura`) VALUES
-- Demonic Embrace -> 200409 dodge from the owner's intellect.
-- Voidwalker and Felguard only: the TODO scopes this to the two tanking demons.
(18697, 1,  1860, 200409),
(18697, 1, 17252, 200409),
(18698, 1,  1860, 200409),
(18698, 1, 17252, 200409),
(18699, 1,  1860, 200409),
(18699, 1, 17252, 200409),

-- Fel Synergy -> 200410 "pet damage heals the warlock" proc carrier.
-- Any demon.
(47230, 1, 0, 200410),
(47231, 1, 0, 200410),

-- Demonic Brutality -> 200412 melee threat multiplier.
-- Voidwalker only -- it is the tanking half of the talent.
(18705, 1, 1860, 200412),
(18706, 1, 1860, 200412),
(18707, 1, 1860, 200412),

-- Demonic Lash -> 200414 proc carrier.
-- Succubus (Lash of Pain applies Nether Scar) and Felguard (bonus shadow
-- damage on melee); the one script branches on which fired.
(18754, 0,  1863, 200414),
(18754, 0, 17252, 200414),
(18755, 0,  1863, 200414),
(18755, 0, 17252, 200414),
(18756, 0,  1863, 200414),
(18756, 0, 17252, 200414),

-- Fel Domination -> 200416, +10% damage per owner DoT on the demon's target.
-- 200416 carries its own 30s duration: Pet::CastPetAura cannot pass the
-- talent buff's remaining time, so the two are matched in the DBC instead.
(18708, 0, 0, 200416),

-- Mana Feed -> 200418 "demon damage returns mana" proc carrier.
(30326, 0, 0, 200418),

-- Fel Attunement -> 200419, which carries all of haste, spell crit and melee
-- crit.  It fits in one spell because aura 193 SPELL_AURA_MELEE_SLOW covers
-- melee, ranged and cast speed in a single effect (the same aura Death Knight
-- Pet Scaling 02 puts on the ghoul), so only one anchor is needed.
(18767, 0, 0, 200419),
(18768, 0, 0, 200419),

-- Demonic Resilience -> 200421 demon damage reduction.
-- This is the half of the talent that never worked: its DBC effect was an
-- ADD_FLAT_MODIFIER whose class mask matched nothing, and no core C++ or
-- spell_pet_auras row ever referenced 30319-30321.
(30319, 1, 0, 200421),
(30320, 1, 0, 200421),
(30321, 1, 0, 200421),

-- Nemesis -> 200422, demon critical strikes may grant a Soul Shard.
-- One shared aura for all three ranks, anchored on Effect3, so the per-rank
-- chance is rolled in script rather than in spell_proc.
(63117, 2, 0, 200422),
(63121, 2, 0, 200422),
(63123, 2, 0, 200422);
