-- Alonecraft 4.61 -- Molten Core: restore the spell family filter.
--
--   Core's own row (data/sql/base/db_world/spell_proc.sql:131) is:
--
--     (-47245, 0, 5, 0x00000002, 0, 0, 262144, 0, 2, 0, 0, 0, 0, 0, 0, 0)
--                  ^  ^^^^^^^^^^         ^^^^^^
--                  |  Corruption         DONE_PERIODIC
--                  SPELLFAMILY_WARLOCK
--
--   woa_2026_08_02_05.sql replaced it to widen ProcFlags to 327680 so the new
--   Shadow Bolt effect (DONE_SPELL_MAGIC_DMG_CLASS_NEG) could be seen, but
--   rewrote SpellFamilyName and SpellFamilyMask0 to 0 at the same time.  That
--   removed the "must be Corruption" filter from the whole rank chain: every
--   periodic tick the warlock deals now reaches the aura.
--
--   Health Funnel (755) is SPELLFAMILY_WARLOCK with a periodic effect, so it
--   proc'd Molten Core.  So would Immolate, Curse of Agony, Rain of Fire, and
--   any other warlock DoT.
--
--   Restored to family 5 with mask 0x00000003 -- Corruption (0x2) for EFFECT_0
--   plus Shadow Bolt (0x1) for EFFECT_2, since one proc entry covers the whole
--   aura and both effects have to get through it.  The per-effect narrowing is
--   then done in spell_warl_molten_core: EFFECT_0 requires DONE_PERIODIC *and*
--   Corruption's family bit, EFFECT_2 requires Shadow Bolt's.
--
--   ProcFlags stays 327680 and Chance stays 100 for the reasons
--   woa_2026_08_02_05.sql gives: EFFECT_2 must be able to see a direct magic
--   hit and must fire on every Shadow Bolt, with the talent's real 4/8/12%
--   re-rolled in CheckOriginalProc.

DELETE FROM `spell_proc` WHERE `SpellId` = -47245;
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(-47245, 0, 5, 0x00000003, 0, 0, 327680, 0, 2, 0, 0, 0, 0, 100, 0, 0);
