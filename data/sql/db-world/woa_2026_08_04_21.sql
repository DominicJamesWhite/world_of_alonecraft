-- ============================================================
-- Alonecraft -- Bladework (14185) scales with combo points
-- ============================================================
--
-- Marked for Counterattack (200504) used to grant a flat three counters
-- regardless of how many combo points the finisher spent.  It now grants
-- one counter per combo point, so 1 point = 1 counter ... 5 points = 5.
--
-- The count itself is set in RogueBladework.cpp (spell_rog_bladework), which
-- overwrites the applied aura's charges with the points spent.  The
-- spell_proc row's Charges only has to be the *cap* now, because
-- Aura::CalcMaxCharges (SpellAuras.cpp:903) reads it on every refresh --
-- leaving it at 3 would silently clip a 4- or 5-point mark.
-- ============================================================

UPDATE `spell_proc` SET `Charges` = 5 WHERE `SpellId` = 200504;

-- ============================================================
-- Tooltips.  Single-column edits use UPDATE, never a full-row re-INSERT:
-- a 234-column INSERT would revert every other field these spells have
-- picked up from earlier files.
-- ============================================================

UPDATE `alonecraft_spell_dbc`
SET `SpellDescription0` = 'Mark all enemies within 10 yards for rapid counterattack.  Their next attacks on you, one per combo point spent, are immediately countered for $200505s1% weapon damage, or 180% if a dagger is equipped.  Lasts $200504d.'
WHERE `ID` = 14185;

UPDATE `alonecraft_spell_dbc`
SET `SpellDescription0` = 'Your next attacks on the marked rogue will be immediately countered.'
WHERE `ID` = 200504;
