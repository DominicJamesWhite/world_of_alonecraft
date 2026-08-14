-- ============================================================
-- $h tooltips: make ProcChance agree with spell_proc.Chance
-- ============================================================
--
-- Every talent description written in this pass that quotes a proc chance uses
-- the $h tooltip variable, and $h renders the DBC's ProcChance field -- NOT the
-- spell_proc row the server actually rolls against.  Retail ships ProcChance =
-- 101 on essentially every talent (a "not applicable" marker, since a talent
-- aura does not proc on its own), so all four read "101%" in game.
--
-- The two numbers are genuinely independent:
--
--   server  SpellMgr.cpp:2065  procEntry.Chance = spell_proc.Chance, used as-is
--                              by Aura::CalcProcChance (SpellAuras.cpp:2267)
--   client  $h                 SpellInfo::ProcChance, straight from Spell.dbc
--
-- SpellMgr.cpp:2083 only falls back to ProcChance when spell_proc.Chance is 0,
-- which is why the behaviour was already correct while the tooltip lied.  That
-- asymmetry is the trap: a wrong ProcChance produces no error, no log line and
-- no gameplay difference, so nothing catches it except reading the tooltip.
--
-- Setting ProcChance to the real number is safe in both directions.  The
-- spell_proc row still wins server-side, and if one ever failed to load, the
-- fallback would now be the intended chance rather than a guaranteed proc.
--
-- Rule for anything added later: if a description uses $h, ProcChance must equal
-- that talent's spell_proc.Chance.  If the chance lives on a PET-side carrier
-- (Superior Training below), it is the TALENT's ProcChance the client reads, so
-- the talent still has to carry the number.
--
-- Single-column changes to rows woa_2026_08_11_11/12/13/16.sql already inserted
-- -- UPDATE, never a 234-column re-INSERT.

-- Share the Spoils (woa_2026_08_11_11.sql) -- spell_proc on the talent, 50/100.
UPDATE `alonecraft_spell_dbc` SET `ProcChance` = 50  WHERE `ID` = 24443;
UPDATE `alonecraft_spell_dbc` SET `ProcChance` = 100 WHERE `ID` = 19575;

-- Against the Odds (woa_2026_08_11_12.sql) -- spell_proc on the talent, 50/100.
UPDATE `alonecraft_spell_dbc` SET `ProcChance` = 50  WHERE `ID` = 19559;
UPDATE `alonecraft_spell_dbc` SET `ProcChance` = 100 WHERE `ID` = 19560;

-- Superior Training (woa_2026_08_11_13.sql) -- the chance lives on the PET-side
-- carriers 200750/200751, but $h in the talent description reads the TALENT's
-- own ProcChance, so it is mirrored here.  Keep the two in step if either moves.
UPDATE `alonecraft_spell_dbc` SET `ProcChance` = 50  WHERE `ID` = 19572;
UPDATE `alonecraft_spell_dbc` SET `ProcChance` = 100 WHERE `ID` = 19573;


-- ============================================================
-- Animal Handler: 1/2% -> 10/20% chance of a free Bestial Wrath
-- ============================================================
--
-- Retune, not a bug fix.  TODO.md originally specified 1/2%, which is a proc a
-- player would never notice fire: at roughly one ranged attack every 2 seconds,
-- 2% is about one Bestial Wrath every 100 seconds of sustained shooting, and it
-- is invisible against a 120s cooldown ability the hunter also presses manually.
-- 20% makes the talent something you can feel and play around.
--
-- Both halves have to move together -- spell_proc.Chance is what the server
-- rolls, ProcChance is what the tooltip prints.
UPDATE `alonecraft_spell_dbc` SET `ProcChance` = 10 WHERE `ID` = 34453;
UPDATE `alonecraft_spell_dbc` SET `ProcChance` = 20 WHERE `ID` = 34454;

UPDATE `spell_proc` SET `Chance` = 10 WHERE `SpellId` = 34453;
UPDATE `spell_proc` SET `Chance` = 20 WHERE `SpellId` = 34454;
