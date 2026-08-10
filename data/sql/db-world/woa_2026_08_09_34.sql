-- ============================================================
-- Barricade: stop impersonating Shield Block
-- ============================================================
-- Reported symptom: Barricade had no cooldown in game, despite
-- RecoveryTime 10000 being present in both the deployed Spell.dbc and the
-- database.
--
-- Cause: woa_2026_08_09_31.sql cloned all three Barricade spells from Shield
-- Block (2565) and never cleared SpellFamilyFlags, so 200651/200652/200653 all
-- inherited warrior family bit 4096 -- Shield Block's own bit.  Every
-- family-mask modifier written for Shield Block was therefore matching
-- Barricade too:
--
--   29598 / 29599  Shield Mastery      SPELLMOD_COOLDOWN  -10s / -20s
--   67273          T9 Tank 4P bonus    SPELLMOD_COOLDOWN  -10s
--   61571          Spirits of the Lost SPELLMOD_COST      -50%
--   200641-200645  Anticipation        SPELLMOD_DAMAGE    +5..25%  (inert:
--                                      Barricade deals no damage)
--
-- Player::AddSpellAndCategoryCooldowns runs
-- ApplySpellMod(SPELLMOD_COOLDOWN, rec) and then clamps a negative result to
-- zero, so a Protection warrior with Shield Mastery at any rank had the whole
-- 10 second cooldown erased.  Nothing is logged; the ability simply becomes
-- spammable.
--
-- This is the same trap the Riposte counterattacks documented in
-- woa_2026_08_07_10.sql -- "as a warrior-family spell these would feed every
-- warrior family-mask proc in the tree" -- and the first, toggle version of
-- Barricade explicitly set SpellFamilyFlags 0 for exactly this reason.  The
-- rewrite changed donor and lost that line.
--
-- SpellFamilyName stays 4: Barricade is a warrior ability and should still be
-- caught by any genuinely blanket warrior modifier.  It is the *flags* that
-- must be empty, so no mask written for another spell can claim it.
--
-- General rule for cloned custom spells: unless the new spell is deliberately
-- meant to inherit the donor's talent support, clear SpellFamilyFlags.
--
-- UPDATE rather than a re-INSERT, so nothing else in these rows is restated.
-- ============================================================

UPDATE `alonecraft_spell_dbc`
SET `SpellFamilyFlags` = 0,
    `SpellFamilyFlags1` = 0,
    `SpellFamilyFlags2` = 0
WHERE `ID` IN (200651, 200652, 200653);
