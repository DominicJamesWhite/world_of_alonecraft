-- Alonecraft 4.61 -- two inaccurate warlock tooltips.
--
--
-- 1. MOLTEN FURY (200420) RENDERS "0 SHADOW DAMAGE"
--
--   Its buff-bar tooltip is '$s1 Shadow damage every $t1 seconds.', but
--   EffectBasePoints1 is 0 -- the per-tick amount is computed at runtime from
--   the Shadow Bolt that spawned it and passed via CastCustomSpell
--   (WarlockDemonology.cpp:337).  The client substitutes $s1 from its own copy
--   of Spell.dbc and knows nothing about the server's custom base points, so
--   the debuff reads "0 Shadow damage every 3 seconds" no matter how hard it
--   is actually ticking.
--
--   This is the case CLAUDE.md calls out: a variable is only safe when the
--   number is in the DBC.  Replaced with plain text.  $t1 stays -- amplitude
--   (3000 ms) is a real DBC field, so it renders 3 correctly.
--
--
-- 2. FEL DOMINATION (18708) STILL DESCRIBES THE OLD TALENT
--
--   woa_2026_08_02_03.sql rewrote SpellDescription0 for the redesign but left
--   SpellToolTip0 as Blizzard shipped it -- "Imp, Voidwalker, Succubus,
--   Felhunter and Felguard casting time reduced by $/1000;S1 sec.  Mana cost
--   reduced by $s2%."  Both of those variables now resolve to 0 as well, since
--   the effects they read were repurposed.
--
--   The talent is neither passive nor DO_NOT_DISPLAY and carries a 30 sec
--   duration, so this string is what players see hovering the buff they just
--   pressed.  The pet-side carrier 200416 got a correct tooltip in
--   woa_2026_08_03_18.sql; the player-facing spell was missed.
--
--   The description also gains the cap: spell_warl_demon_fel_domination
--   (WarlockDemonPets.cpp:400-401) is PCT_PER_DOT = 10 with MAX_DOTS = 8, so
--   the bonus stops climbing at +80% and the old wording implied no ceiling.
--   Both numbers stay literal -- 18708's own base points are 0, so a variable
--   would render 0 here too.

UPDATE `alonecraft_spell_dbc` SET
    `SpellToolTip0` = 'Suffering additional Shadow damage every $t1 sec.'
WHERE `ID` = 200420;

UPDATE `alonecraft_spell_dbc` SET
    `SpellToolTip0` = 'Your demons deal 10% additional damage for each of your damage-over-time effects on their target, up to 80%.',
    `SpellDescription0` = 'For 30 sec, your demons deal 10% additional damage for each of your damage-over-time effects on their target, up to a maximum of 80%.'
WHERE `ID` = 18708;
