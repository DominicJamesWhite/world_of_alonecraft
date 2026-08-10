-- ============================================================
-- Warrior (Protection): Safeguard -- clear the inherited Intervene mask
-- ============================================================
-- Hygiene follow-up to woa_2026_08_09_11.sql.
--
-- Stock Safeguard was "reduces damage taken by the target of your Intervene",
-- and both ranks carried an Intervene SpellClassMask on EFFECT_0
-- (EffectSpellClassMaskA1 = 1073741825, EffectSpellClassMaskB1 = 65536).  The
-- redesign moved the proc to EFFECT_1 and cleared the mask there, but left
-- EFFECT_0's behind.
--
-- This is currently harmless -- 46945/46949 have Effect1 = 0, so core never
-- builds an AuraEffect for that slot and the mask is never read.  It is cleared
-- anyway because a stale family mask on a redesigned talent is exactly the kind
-- of thing that silently changes meaning the moment somebody puts an effect in
-- that slot, and because leaving it makes the row read as though Safeguard
-- still has something to do with Intervene.
--
-- NOTE FOR ANYONE CHASING "SAFEGUARD DOES NOT PROC": it does.  The reason it
-- appeared not to was Victory Rush, which shipped restricted to Battle and
-- Berserker Stance -- so the buff applied correctly and the button it enables
-- stayed unusable for a warrior standing in Defensive Stance.  That is fixed in
-- woa_2026_08_09_19.sql, and it applied equally to Small Victories and
-- Improved Disciplines.
--
-- Single-column edits, so UPDATE rather than a re-INSERT.
--
-- Spells:
--   46945 / 46949 = Safeguard ranks 1-2
-- ============================================================

UPDATE `alonecraft_spell_dbc`
   SET `EffectSpellClassMaskA1` = 0,
       `EffectSpellClassMaskB1` = 0,
       `EffectSpellClassMaskC1` = 0
 WHERE `ID` IN (46945, 46949);
