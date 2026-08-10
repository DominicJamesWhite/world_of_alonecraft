-- ============================================================
-- Warrior (Protection): Improved Bloodrage class-mask fix
-- ============================================================
-- TODO.md: "Improved Bloodrage (3, 2): Redesigned.  Each time you take damage
--           while Bloodrage is active, the damage of your Shield Slam is
--           increased by 5/10% per stack.  Stacks 5 times."
--
-- Shipped in woa_2026_08_09_13.sql with Shield Slam's family bit in the wrong
-- column, which quietly turned "the damage of your Shield Slam" into the
-- damage of every warrior ability.
--
-- 200647 and 200648 carry a single ADD_PCT_MODIFIER (108) on Effect1 with
-- MiscValue 0 (SPELLMOD_DAMAGE).  The mask was written as
-- EffectSpellClassMaskB1 = 512.  In EffectSpellClassMask<letter><digit> the
-- LETTER is the effect index and the DIGIT is the word of that effect's
-- flag96 -- DBCStructure.h:1750 declares the field as
-- std::array<flag96, MAX_SPELL_EFFECTS>, which is effect-major.  B1 is
-- therefore word0 of Effect2, an effect these spells do not have, and
-- Effect1's own mask was left (0, 0, 0).
--
-- An all-zero flag96 is falsy, so SpellInfo::IsAffected (SpellInfo.cpp:1333)
-- skips the mask comparison entirely:
--
--     if (familyFlags && !(familyFlags & SpellFamilyFlags))
--         return false;
--
-- With SpellFamilyName 4 still set, the modifier matched every warrior spell.
-- At 2/2 and five stacks that is +50% to the whole kit, not to Shield Slam.
--
-- Shield Slam is warrior SpellFamilyFlags **word1** bit 512, so the bit
-- belongs in A2 -- letter A for Effect1, digit 2 for word1.  Sword and Board
-- (50227) is the reference: one modifier on Effect1 affecting Shield Slam,
-- carrying 512 in EffectSpellClassMaskA2.
--
-- UPDATE rather than a re-INSERT, per the project rule: a full 234-column
-- INSERT would silently restate every other column as whatever the generating
-- tool believed at the time, clobbering anything a later file had set.
-- ============================================================

UPDATE `alonecraft_spell_dbc`
SET `EffectSpellClassMaskA2` = 512,
    `EffectSpellClassMaskB1` = 0
WHERE `ID` IN (200647, 200648);
