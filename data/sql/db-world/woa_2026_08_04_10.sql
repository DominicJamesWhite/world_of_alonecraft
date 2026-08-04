-- ===========================================================================
-- Warlock / Demonology: Felguard damage retune (Imperious Flames, Demonic Lash)
-- ===========================================================================
--
-- In-game testing showed both Felguard damage sources from these talents
-- hitting far harder than intended.  Two separate causes.
--
-- 1) IMPERIOUS FLAMES -- Immolation Aura payload 200408
--
--    200408 is an unaltered clone of Blizzard's 50590, whose numbers (250 base
--    + 11.5/level + 0.143 SP) were tuned for a 30-second Metamorphosis burst.
--    Here the aura is a near-permanent pet ability, so the AoE tick is ~3x too
--    strong.  Cut the flat portion to one third; the spell power coefficient
--    (EffectBonusMultiplier1 = 0.143) is deliberately left alone.
--
--    MaximumLevel = 0 so the per-level term is uncapped, and BaseLevel = 60 is
--    the subtraction point in SpellEffectInfo::CalcValue (SpellInfo.cpp:417).
--    At level 80 the flat portion goes 250 + 1 + 11.5*20 = 481 down to
--    82 + 1 + 3.83*20 = ~160 per tick.
--
--    No tooltip edits needed: 200407 and 200408 both describe the damage with
--    $200408s1, which re-reads the new value.
--
-- 2) DEMONIC LASH -- Felguard payload 200415
--
--    A real bug.  200415 was cloned from 50590 with EffectBasePoints1 and
--    EffectBonusMultiplier1 zeroed so it would carry only the amount passed by
--    CastCustomSpell (WarlockDemonPets.cpp:432) -- but
--    EffectRealPointsPerLevel1 = 11.5 was never cleared, and the clone has
--    BaseLevel = SpellLevel = 0.  SpellEffectInfo::CalcValue adds
--    level * RealPointsPerLevel ON TOP of the caller's base points
--    (SpellInfo.cpp:417-428), so at level 80 every Felguard auto-attack gained
--    a flat +920 shadow damage.
--
--    The target behaviour is the Death Knight's Necrosis (spell_dk.cpp:2591):
--    a plain percentage of the auto-attack damage dealt, re-dealt as shadow,
--    with no further modifiers.  Necrosis' payload 51460 gets that from three
--    DBC fields this clone lacks, so match all three:
--
--      EffectRealPointsPerLevel1 = 0     -- the bug above
--      AttributesEx6 = 0x20000000        -- SPELL_ATTR6_IGNORE_CASTER_DAMAGE_MODIFIERS.
--                                           SpellInfo::ValidateAttribute6SpellDamageMods
--                                           (SpellInfo.cpp:1631) then rejects every
--                                           done/taken damage modifier.
--      DamageClass = 0                   -- SPELL_DAMAGE_CLASS_NONE; the added hit is
--                                           not a separately critting/resisting spell.
--
--    The C++ proc logic needs no change: the Felguard branch requires
--    spellInfo == nullptr (WarlockDemonPets.cpp:417), so it already fires only
--    on white swings, exactly like Necrosis' ProcFlags = 4.
--
-- 3) DEMONIC LASH -- per-rank Felguard scaling (18754 / 18755 / 18756)
--
--    EFFECT_1 (the half the Felguard code reads) was a flat 15% at every rank,
--    so ranks 2-3 did nothing for a Felguard user while only the Succubus
--    Nether Scar half (EFFECT_0) scaled.  Make it scale 5/10/15% to match that
--    half and Necrosis' per-rank design.  DieSides = 1 adds +1, so base points
--    are value - 1.
--
-- Single-column UPDATEs, not DELETE + full-row INSERT: a re-INSERT would revert
-- the fixes applied by woa_2026_08_04_01.sql and woa_2026_08_04_09.sql.
--
-- Blizzard's own 50589/50590/51460 stay untouched.
--
-- ===========================================================================

-- ---------------------------------------------------------------------------
-- 1) Imperious Flames: Felguard Immolation tick to 1/3 flat damage
-- ---------------------------------------------------------------------------
UPDATE `alonecraft_spell_dbc` SET
    `EffectBasePoints1`         = 82,
    `EffectRealPointsPerLevel1` = 3.83
WHERE `ID` = 200408;

-- ---------------------------------------------------------------------------
-- 2) Demonic Lash payload: behave like Necrosis (51460)
-- ---------------------------------------------------------------------------
UPDATE `alonecraft_spell_dbc` SET
    `EffectRealPointsPerLevel1` = 0,
    `AttributesEx6`             = 536870912,
    `DamageClass`               = 0
WHERE `ID` = 200415;

-- ---------------------------------------------------------------------------
-- 3) Demonic Lash talent ranks: Felguard half scales 5 / 10 / 15%
-- ---------------------------------------------------------------------------
UPDATE `alonecraft_spell_dbc` SET
    `EffectBasePoints2`  = 4,
    `SpellDescription0`  = 'Your Succubus'' Lash of Pain leaves a Nether Scar, increasing your Shadow damage against that enemy by 5%.  Your Felguard''s auto attacks deal an additional 5% Shadow damage.'
WHERE `ID` = 18754;

UPDATE `alonecraft_spell_dbc` SET
    `EffectBasePoints2`  = 9,
    `SpellDescription0`  = 'Your Succubus'' Lash of Pain leaves a Nether Scar, increasing your Shadow damage against that enemy by 10%.  Your Felguard''s auto attacks deal an additional 10% Shadow damage.'
WHERE `ID` = 18755;

UPDATE `alonecraft_spell_dbc` SET
    `EffectBasePoints2`  = 14,
    `SpellDescription0`  = 'Your Succubus'' Lash of Pain leaves a Nether Scar, increasing your Shadow damage against that enemy by 15%.  Your Felguard''s auto attacks deal an additional 15% Shadow damage.'
WHERE `ID` = 18756;
