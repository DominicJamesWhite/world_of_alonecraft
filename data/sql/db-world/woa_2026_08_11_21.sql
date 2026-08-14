-- ============================================================
-- Taste for Blood: fix a tooltip that names the wrong attacker
-- ============================================================
--
-- The description shipped in woa_2026_08_11_09.sql opened with "When you attack
-- a target affected by your Lacerating Shot...", which reads as though the
-- HUNTER has to attack for the pet to gain anything.  It does not.
--
-- What actually triggers it: aura 200745 sits on the PET (spell_pet_auras rows
-- for 19549/19550/19551) and its spell_proc row is ProcFlags 20 =
-- PROC_FLAG_DONE_MELEE_AUTO_ATTACK (0x4) | PROC_FLAG_DONE_SPELL_MELEE_DMG_CLASS
-- (0x10) -- DONE_*, on the pet's own aura, so the pet is the actor.  The hunter
-- contributes exactly two things and neither is an attack:
--
--   * the bleed must be THEIRS -- spell_hun_taste_for_blood::CheckProc looks up
--     the Lacerating Shot aura by the owner's GUID, so another hunter's bleed on
--     the same target does not feed this pet;
--   * their ranged attack power supplies the amount.
--
-- The wording came from TODO.md's "When attacking targets affected by Lacerating
-- Shot your pet does additional damage", which never says who is attacking.  The
-- subject has to be the pet, because that is the only thing being tested.
--
-- Single-column change to rows woa_2026_08_11_09.sql already inserted -- UPDATE,
-- never a 234-column re-INSERT.  $s1 still reads Effect1's base points, which
-- are unchanged at 20/40/60.

UPDATE `alonecraft_spell_dbc`
   SET `SpellDescription0` = 'Your pet''s attacks against targets affected by your Lacerating Shot deal additional damage equal to $s1% of your ranged attack power.'
 WHERE `ID` IN (19549, 19550, 19551);

-- The pet-side carrier's own hidden text described the effect from the pet's
-- point of view but implied the pet had to be the one bleeding the target.
UPDATE `alonecraft_spell_dbc`
   SET `SpellDescription0` = 'Your attacks against targets bleeding from your master''s Lacerating Shot deal additional damage.',
       `SpellToolTip0`     = 'Your attacks against targets bleeding from your master''s Lacerating Shot deal additional damage.'
 WHERE `ID` = 200745;
