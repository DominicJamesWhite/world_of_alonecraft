-- Gift of Nature: give the proc carriers a proc flag.
--
-- 200006-200010 are the five rank carriers for Gift of Nature ("When you are
-- healed by one of your own heal over time spells, you have a 2/4/6/8/10%
-- chance for your next Wrath to be instant cast and cost no mana"). Each one
-- has SpellPhaseMask = 2 and a Chance, so it looks configured -- but ProcFlags
-- was 0, which matches no event at all, so none of them could ever fire.
--
-- Consequences, all silent: the Nature's Gift buff (200004) never existed, so a
-- restoration druid never got a free instant Wrath, and the
-- aura_proc_self_hot_only script bound to these five ids (EmpoweredTouch.cpp)
-- had never once run. Found because the bot rotation gates Wrath on that buff
-- and cast Wrath zero times in a 360-second measurement.
--
-- PROC_FLAG_DONE_PERIODIC (0x00040000) rather than TAKEN: the script filters on
-- eventInfo.GetActor() vs GetActionTarget() and keeps only heals the caster
-- landed on itself, which is the "done" side of the event.
UPDATE `spell_proc` SET `ProcFlags` = 262144
WHERE `SpellId` IN (200006, 200007, 200008, 200009, 200010);
