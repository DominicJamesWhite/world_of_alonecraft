-- ===========================================================================
-- Warlock / Demonology: Demonic Aegis survives a respec
-- ===========================================================================
--
-- Bug: the 200417 bonus stayed on after changing or resetting spec.  It is
-- cast from Demon Armor's aura script and has no duration, so once up nothing
-- took it back down -- dropping the talent left the old armor and crit
-- avoidance in place until Demon Armor happened to be re-applied.
--
-- Fix follows the pattern core already uses for this exact talent.  Retail
-- Demonic Aegis is an ADD_PCT_MODIFIER on Demon Armor / Fel Armor, and
-- spell_warl_demonic_aegis (spell_warlock.cpp:256) exists only to strip both
-- armors when the modifier aura goes away, forcing a recast.  File 06 had to
-- unregister that script when 30143-30145's Effect1 became SPELL_AURA_DUMMY;
-- spell_warl_demonic_aegis_talent is the module's replacement, bound to the
-- DUMMY effect instead and stripping Demon Armor only.
--
-- Binding to the talent rather than to a respec hook means the engine's own
-- aura removal drives it -- no ordering question about when talent auras are
-- actually gone during Player::resetTalents.  It also covers a rank change
-- (3/3 -> 1/3), which a plain "remove the buff" would leave at the old amount.
--
-- ===========================================================================

DELETE FROM `spell_script_names`
    WHERE `ScriptName` = 'spell_warl_demonic_aegis_talent';

-- Negative id applies to the whole rank chain (30143, 30144, 30145).
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(-30143, 'spell_warl_demonic_aegis_talent');
