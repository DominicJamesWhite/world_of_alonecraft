-- ===========================================================================
-- Warlock / Demonology: spell_proc
-- ===========================================================================
--
-- Fifth file of the Demonology batch.  Arms the proc carriers created in
-- woa_2026_08_02_02.sql and the owner-side half of Nemesis.
--
-- DELETING BY ABS(SpellId)
--
--   The core ships all-ranks rows with a NEGATIVE SpellId.  Those load first
--   and cause any later positive per-rank row to be rejected as a duplicate
--   (SpellMgr.cpp:2071), so deleting only the positive id leaves the proc
--   dead.  Every DELETE below therefore uses ABS().
--
-- SpellPhaseMask MUST BE NON-ZERO
--
--   A spell_proc row with SpellPhaseMask = 0 silently never triggers -- no
--   error, no warning.  2 = PROC_SPELL_PHASE_HIT for all standard procs.
--
-- ProcFlags 331796 -- "the demon dealt damage, by any means":
--       0x00000004  PROC_FLAG_DONE_MELEE_AUTO_ATTACK        (melee swings)
--     | 0x00000010  PROC_FLAG_DONE_SPELL_MELEE_DMG_CLASS    (Cleave, Lash of Pain)
--     | 0x00001000  PROC_FLAG_DONE_SPELL_NONE_DMG_CLASS_NEG
--     | 0x00010000  PROC_FLAG_DONE_SPELL_MAGIC_DMG_CLASS_NEG (Firebolt, Shadow Bite)
--     | 0x00040000  PROC_FLAG_DONE_PERIODIC                  (Immolation Aura ticks)
--
-- HitMask 2 = PROC_HIT_CRITICAL.  SpellTypeMask 1 = PROC_SPELL_TYPE_DAMAGE.
--
-- MOLTEN CORE -- THE CORE ALREADY SHIPS A ROW, AND IT BLOCKED THE NEW EFFECT
--
--   `spell_proc` does NOT wholesale replace a spell's DBC proc configuration:
--   LoadSpellProcs (SpellMgr.cpp:74-79) falls back to the DBC for any field
--   left at zero --
--       if (!procEntry.ProcFlags) procEntry.ProcFlags = spellInfo->ProcFlags;
--       if (!procEntry.Charges)   procEntry.Charges   = spellInfo->ProcCharges;
--       if (!procEntry.Chance && !procEntry.ProcsPerMinute)
--                                 procEntry.Chance    = spellInfo->ProcChance;
--   -- but any NON-zero field does override it.
--
--   Core ships `(-47245, ProcFlags = 262144, SpellPhaseMask = 2)`.  262144 is
--   PROC_FLAG_DONE_PERIODIC alone, and being non-zero it overrides the DBC's
--   327680 for the whole rank chain.  Shadow Bolt's direct hit is
--   PROC_FLAG_DONE_SPELL_MAGIC_DMG_CLASS_NEG (0x10000), which is not in that
--   mask, so the talent's new Effect3 would never have fired.
--
--   The row is therefore rewritten with 327680 = 0x50000
--   (DONE_PERIODIC | DONE_SPELL_MAGIC_DMG_CLASS_NEG) so both effects can see
--   their own trigger, and Chance is pinned at 100 to match the DBC rather
--   than relying on the fallback.
--
--   Widening ProcFlags means Effect0 -- the original Corruption/Immolate
--   periodic proc, which carries no class mask -- would now also see direct
--   damage.  spell_warl_molten_core's CheckEffectProc on EFFECT_0 restores the
--   original behaviour exactly: it requires PROC_FLAG_DONE_PERIODIC and rolls
--   the talent's own 4/8/12%.  Effect2 is class-masked to Shadow Bolt alone.
--
--   SpellTypeMask stays 0, as shipped -- "if nonzero" (SpellMgr.h:291), so
--   zero means no filtering.
--
-- FEL DOMINATION -- STALE ROW REMOVED
--
--   Core ships `(18708, ProcFlags = 0, SpellPhaseMask = 1)`, which used to
--   inherit the DBC's ProcFlags 87376 and drive the consumed-on-next-summon
--   charge.  File 03 turns Fel Domination into a plain 30s cooldown with
--   ProcFlags 0 and ProcCharges 0, so the row would now resolve to
--   ProcFlags 0 and log "doesn't have `ProcFlags` value defined, proc will not
--   be triggered" at every startup.  It is deleted rather than left to rot.
--
-- ===========================================================================

DELETE FROM `spell_proc` WHERE ABS(`SpellId`) IN
    (200410, 200414, 200418, 200422, 63117, 63121, 63123, 47245, 18708);

INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES

-- Fel Synergy (pet side): any damage the demon deals heals the warlock for
-- 20/40% of it.  No ICD -- the heal is proportional to the damage, so a high
-- proc rate is self-limiting.
(200410, 0, 0, 0, 0, 0, 331796, 1, 2, 0, 0, 0, 0, 100, 0, 0),

-- Demonic Lash (pet side): Succubus Lash of Pain applies Nether Scar, Felguard
-- melee adds shadow damage.  The script decides which branch from the
-- proccing spell; its own payload 200415 is rejected in DoCheckProc so the
-- shadow hit cannot re-trigger itself.
(200414, 0, 0, 0, 0, 0, 331796, 1, 2, 0, 0, 0, 0, 100, 0, 0),

-- Mana Feed (pet side): 5% of demon damage back as owner mana, throttled to
-- once per second.  Without the ICD, Immolation Aura ticking on every nearby
-- enemy plus Felguard melee would make mana a non-resource.
(200418, 0, 0, 0, 0, 0, 331796, 1, 2, 0, 0, 0, 0, 100, 1000, 0),

-- Nemesis (pet side): one shared aura across all three ranks, so this row only
-- supplies the "was a critical strike" gate -- the 5/10/15% roll happens in
-- script, where the owner's rank is known.
(200422, 0, 0, 0, 0, 0, 331796, 1, 2, 2, 0, 0, 0, 100, 0, 0),

-- Nemesis (owner side): the warlock's own critical strikes.  Here the rank IS
-- the spell id, so the chance can live in the table.  Effect1 of these spells
-- is a passive cooldown spellmod, not a proc, so it is unaffected by this row.
(63117, 0, 0, 0, 0, 0, 331796, 1, 2, 2, 0, 0, 0,  5, 0, 0),
(63121, 0, 0, 0, 0, 0, 331796, 1, 2, 2, 0, 0, 0, 10, 0, 0),
(63123, 0, 0, 0, 0, 0, 331796, 1, 2, 2, 0, 0, 0, 15, 0, 0),

-- Molten Core: replaces core's own -47245 row, whose ProcFlags of 262144
-- (DONE_PERIODIC only) would have stopped the new Shadow Bolt effect from ever
-- proccing.  327680 = DONE_PERIODIC | DONE_SPELL_MAGIC_DMG_CLASS_NEG.  Negative
-- id keeps it applying to all three ranks, exactly as shipped.
(-47245, 0, 0, 0, 0, 0, 327680, 0, 2, 0, 0, 0, 0, 100, 0, 0);
