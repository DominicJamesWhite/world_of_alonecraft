--
-- Simulator dummies: make them genuinely inert.
--
-- woa_2026_08_13_00.sql gave these an aggressive dummy with DamageModifier
-- 0.01, because a playerbot only runs its rotation in BOT_STATE_COMBAT and the
-- only way found at the time to get it there was to let the dummy start the
-- fight. That bought engagement at two costs:
--
--   * The target fights back, so it pollutes the number being measured. A
--     priest starts healing itself; a warrior gains rage it would not otherwise
--     have. "0.01 damage" is small, not zero, and a DPS measurement should not
--     have to argue about how small.
--   * It was unreliable. One run in two measured exactly 0 damage, because the
--     AttackStart handshake silently did not take.
--
-- The simulator now drives the bot's own AI into combat directly (SimRunner.cpp,
-- EngageActor), which is both deterministic and closer to what the bot does in
-- the field, so the target no longer needs to do anything at all.
--
-- ScriptName sim_target_dummy gives them NullCreatureAI. Every no-op in it is
-- load-bearing: AttackStart and UpdateAI mean the dummy never hits back, and
-- EnterEvadeMode means a dummy attacked for sixty seconds without ever
-- attacking cannot decide the fight is over and reset it.
--
-- Note this is NOT core's npc_training_dummy, which zeroes DamageTaken and
-- would report every spec at 0 DPS. These keep real health and real armour, so
-- damage against them is computed exactly as against any other creature.
--
-- DamageModifier returns to 1: it is now unused (the dummy never swings) and
-- leaving 0.01 in place would be a misleading fossil of the old workaround.
--

UPDATE `creature_template`
   SET `ScriptName`     = 'sim_target_dummy',
       `DamageModifier` = 1
 WHERE `entry` BETWEEN 2000100 AND 2000199;
