-- Remove stale spell_proc entry for Nature's Guardian (redesigned as passive DUMMY aura, no longer procs)
DELETE FROM `spell_proc` WHERE `SpellId` = -30881;
