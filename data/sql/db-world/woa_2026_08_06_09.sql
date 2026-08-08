-- ===========================================================================
-- Warlock (Affliction): Improved Drain Soul -- the missing script registration
-- ===========================================================================
--
-- Third file in the same recovery as woa_2026_08_06_06.sql and _08.sql: the
-- Improved Drain Soul rework was applied straight to the live database and
-- only ever half-committed.  woa_2026_08_06_08.sql reconstructed the DBC rows;
-- the spell_script_names row was still live-only, which is what
-- tools/verify_scripts.py reports as
--
--   [C++ -> SQL] Script 'spell_warl_improved_drain_soul_shards' in
--   WarlockDrainChanneling.cpp has no SQL registration
--
-- On the live database the row exists, so the talent works there; on any fresh
-- database the AuraScript compiles, loads, and is never attached to a spell.
-- This file makes that state reproducible.
--
-- -1120 = all ranks of Drain Soul.  Registered on the channel, not on the
-- talent, because the extra shard roll happens on each channel tick --
-- spell_warl_improved_drain_soul_shards_AuraScript::HandleTick hooks
-- OnEffectPeriodic at EFFECT_1 / SPELL_AURA_PERIODIC_DAMAGE, which is Drain
-- Soul's damage tick ($o2 in its description).  The talent itself is read from
-- inside the hook via GetAuraOfRankedSpell.
--
-- Core already registers spell_warl_drain_soul on -1120 and
-- spell_warl_improved_drain_soul on -18213; AzerothCore allows several scripts
-- per spell, so both stay.  No overlap in what they read either: core's talent
-- script uses EFFECT_2 (the mana-on-kill percentage), this one uses EFFECT_1
-- (the 50 / 100% shard rate woa_2026_08_06_08.sql wrote).
--
-- Delete by ScriptName, never by spell_id -- deleting by -1120 would drop
-- core's rows and spell_warl_fel_concentration with them.
-- ===========================================================================

DELETE FROM `spell_script_names` WHERE `ScriptName` = 'spell_warl_improved_drain_soul_shards';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(-1120, 'spell_warl_improved_drain_soul_shards');
