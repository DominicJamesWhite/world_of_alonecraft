-- ===========================================================================
-- Warlock (Affliction): faster Mark of Gul'dan clearing
-- ===========================================================================
--
-- Testing showed stacks were not coming off fast enough.  Two changes, both
-- pure spell_proc -- the C++ scripts are generic and need no edit:
--
--   Fel Interdiction: Drain Soul, Drain Life AND Haunt each clear a stack,
--                     guaranteed.  Was Drain Soul alone.
--   Malfeasance:      20/40% chance from ANY of your periodic damage, rather
--                     than from Drain Life / Shadow Bolt / Haunt specifically.
--
-- This also swaps which talent covers which kind of damage.  The base talent
-- now keys off the three Affliction filler/drain spells you are pressing
-- anyway, and Malfeasance keys off the DoT suite you are maintaining -- so
-- the pair rewards the two halves of the rotation separately instead of both
-- watching the same handful of spells.
--
-- Shadow Bolt is deliberately dropped from Malfeasance: it is a direct hit,
-- not a periodic effect, and it was the only non-DoT in that list.
--
-- Note the deliberate overlap.  Drain Life and Drain Soul ticks are periodic,
-- so they clear a stack through Fel Interdiction AND roll for a second one
-- through Malfeasance.  That is intended -- channelling a drain is the most
-- committed thing an Affliction lock can do with a global, so it should be
-- the fastest way to dump the pool.
--
-- Mark of Gul'dan's own ticks are periodic Shadow damage the player deals to
-- themselves, so they would otherwise roll Malfeasance and clear the very
-- stacks they belong to.  Spell 200520 carries SPELL_ATTR3_SUPPRESS_CASTER_
-- PROCS (set in woa_2026_08_06_01.sql) precisely to stop that.
-- ===========================================================================


-- ============================================================
-- Fel Interdiction: Drain Soul + Drain Life + Haunt, guaranteed
-- ============================================================
-- SpellFamilyMask0 16392 = 0x4000 Drain Soul | 0x8 Drain Life
-- SpellFamilyMask1 262144 = 0x40000 Haunt
-- (computed with: gen_sql.py classmask --family 5 --spells 1120,689,48181)
--
-- ProcFlags stay 327680 = DONE_SPELL_MAGIC_DMG_CLASS_NEG | DONE_PERIODIC:
-- the two drains tick as periodic auras, Haunt lands as a direct hit, so both
-- halves are load-bearing.  Chance stays 100.

UPDATE `spell_proc` SET
    `SpellFamilyMask0` = 16392,
    `SpellFamilyMask1` = 262144
WHERE `SpellId` = 30054;


-- ============================================================
-- Malfeasance: 20/40% from any periodic damage
-- ============================================================
-- ProcFlags 262144 = 0x40000 = PROC_FLAG_DONE_PERIODIC only.  Dropping
-- DONE_SPELL_MAGIC_DMG_CLASS_NEG (65536) is what makes this DoT-only --
-- Corruption, Unstable Affliction, Curse of Agony, Immolate, Seed, and the
-- drain ticks, but not Shadow Bolt or Haunt's impact.
--
-- All three family masks are zeroed, which means "match everything" (see
-- CLAUDE.md, spell_proc pitfalls) -- so it is the whole DoT suite, not a
-- hand-listed subset that would need revisiting every time a talent adds a
-- new periodic effect.  SpellFamilyName 5 keeps it to the warlock's own
-- spells, so pet and trinket periodics do not count.
--
-- SpellTypeMask 1 = PROC_SPELL_TYPE_DAMAGE is what stops periodic HEALS and
-- mana drains rolling this; DONE_PERIODIC alone would not.
--
-- Chance stays 20 / 40 per rank.

UPDATE `spell_proc` SET
    `ProcFlags`        = 262144,
    `SpellFamilyMask0` = 0,
    `SpellFamilyMask1` = 0,
    `SpellFamilyMask2` = 0
WHERE `SpellId` IN (200521, 200522);


-- ============================================================
-- Tooltips
-- ============================================================
-- UPDATE, not DELETE + 234-column INSERT: a full-row re-INSERT would revert
-- every other column to whatever the generating tool thought it was and
-- clobber what woa_2026_08_06_01/02/03.sql set.

UPDATE `alonecraft_spell_dbc` SET
    `SpellDescription0` = 'When using Fel Armor, $s1% of damage taken is converted to a Mark of Gul''dan, dealing damage periodically. Dealing damage with Drain Soul, Drain Life or Haunt removes a stack, and casting Soulshatter removes $s2 stacks.'
WHERE `ID` = 30054;

UPDATE `alonecraft_spell_dbc` SET
    `SpellDescription0` = 'Your periodic damage has a $s1% chance to clear a stack of Mark of Gul''dan, and your Soulshatter clears $s2 stacks instead of 2.',
    `SpellToolTip0`     = 'Your periodic damage has a $s1% chance to clear a stack of Mark of Gul''dan.'
WHERE `ID` IN (200521, 200522);
