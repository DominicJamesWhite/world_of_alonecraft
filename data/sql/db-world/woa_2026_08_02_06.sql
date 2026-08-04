-- ===========================================================================
-- Warlock / Demonology: spell_script_names
-- ===========================================================================
--
-- Sixth file of the Demonology batch.  Binds the new C++ scripts and removes
-- the one core registration this batch invalidates.
--
-- A negative spell_id applies the script to every rank of the chain.  Several
-- rows per spell are legal (-755 already carries both spell_hun_check_pet_los
-- and spell_warl_health_funnel), which is what lets the module attach its own
-- scripts to core-scripted spells without touching core files.
--
-- Every ScriptName below must match a SpellScriptLoader constructor string
-- exactly -- a mismatch means the script silently never runs.  tools/
-- verify_scripts.py cross-checks these against the C++ and MP_loader.cpp.
--
-- ===========================================================================

DELETE FROM `spell_script_names` WHERE `ScriptName` IN (
    'spell_warl_improved_healthstone_mana',
    'spell_warl_imperious_flames',
    'spell_warl_demonic_aegis_armor',
    'spell_warl_molten_core',
    'spell_warl_nemesis_shard',
    'spell_warl_demon_dodge',
    'spell_warl_demon_fel_synergy',
    'spell_warl_demon_brutality',
    'spell_warl_demon_lash',
    'spell_warl_demon_fel_domination',
    'spell_warl_demon_mana_feed',
    'spell_warl_demon_attunement',
    'spell_warl_demon_resilience',
    'spell_warl_demon_nemesis',
    'spell_warl_sacrifice_of_blood',
    'spell_warl_metamorphosis'
);

-- ---------------------------------------------------------------------------
-- Removal: core's Demonic Aegis script no longer matches its spell
-- ---------------------------------------------------------------------------
-- spell_warl_demonic_aegis (spell_warlock.cpp:256) binds OnEffectRemove to
-- EFFECT_0 / SPELL_AURA_ADD_PCT_MODIFIER.  File 03 turns 30143-30145's
-- Effect1 into SPELL_AURA_DUMMY, so that handler would no longer match and
-- AzerothCore would log "Spell `30143` Effect Index `0` ... did not match" at
-- startup.  Deleting a DB row is module-only -- no core file is edited.
DELETE FROM `spell_script_names`
    WHERE `ScriptName` = 'spell_warl_demonic_aegis';

-- ---------------------------------------------------------------------------
-- Owner-side scripts (WarlockDemonology.cpp)
-- ---------------------------------------------------------------------------
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES

-- Improved Healthstone: bound to the healthstone USE spells.  These are listed
-- individually rather than by rank chain because the chain is not contiguous
-- and because the family class mask (word A 0x00010000) also catches two
-- unrelated spells -- 18671 Curse of Agony and 41237 Charged Crystal Focus.
(5720,  'spell_warl_improved_healthstone_mana'),
(5723,  'spell_warl_improved_healthstone_mana'),
(6262,  'spell_warl_improved_healthstone_mana'),
(6263,  'spell_warl_improved_healthstone_mana'),
(11732, 'spell_warl_improved_healthstone_mana'),
(23468, 'spell_warl_improved_healthstone_mana'),
(23469, 'spell_warl_improved_healthstone_mana'),
(23470, 'spell_warl_improved_healthstone_mana'),
(23471, 'spell_warl_improved_healthstone_mana'),
(23472, 'spell_warl_improved_healthstone_mana'),
(23473, 'spell_warl_improved_healthstone_mana'),
(23474, 'spell_warl_improved_healthstone_mana'),
(23475, 'spell_warl_improved_healthstone_mana'),
(23476, 'spell_warl_improved_healthstone_mana'),
(23477, 'spell_warl_improved_healthstone_mana'),
(27235, 'spell_warl_improved_healthstone_mana'),
(27236, 'spell_warl_improved_healthstone_mana'),
(27237, 'spell_warl_improved_healthstone_mana'),
(47872, 'spell_warl_improved_healthstone_mana'),
(47873, 'spell_warl_improved_healthstone_mana'),
(47874, 'spell_warl_improved_healthstone_mana'),
(47875, 'spell_warl_improved_healthstone_mana'),
(47876, 'spell_warl_improved_healthstone_mana'),
(47877, 'spell_warl_improved_healthstone_mana'),

-- Imperious Flames: the Imp's Firebolt rank chain (3110 .. 47964, from
-- `spell_ranks`).  NOT bound by name -- many unrelated creatures have a spell
-- called "Firebolt" -- and NOT to 47965, which is the "Teaches Imp Firebolt"
-- learn spell rather than a rank.
(-3110, 'spell_warl_imperious_flames'),

-- Demonic Aegis: applied from Demon Armor, so the bonus is gated on Demon
-- Armor specifically and never on Fel Armor.
(-706, 'spell_warl_demonic_aegis_armor'),

-- Molten Core: the talent itself.  Effect0 keeps the original Corruption proc
-- (its 4/8/12% chance re-rolled in script), Effect2 adds the Shadow Bolt DoT.
(-47245, 'spell_warl_molten_core'),

-- Nemesis, owner half.  The pet half is spell_warl_demon_nemesis on 200422.
(-63117, 'spell_warl_nemesis_shard'),

-- ---------------------------------------------------------------------------
-- Health Funnel (WarlockHealthFunnel.cpp)
-- ---------------------------------------------------------------------------
-- Registered alongside core's spell_warl_health_funnel, which keeps running.
(-755, 'spell_warl_sacrifice_of_blood'),

-- ---------------------------------------------------------------------------
-- Pet-side scripts (WarlockDemonPets.cpp)
-- ---------------------------------------------------------------------------
-- These bind to the CUSTOM auras that spell_pet_auras pushes onto the demon,
-- not to the talents -- procs are never forwarded from a pet to its owner
-- (Unit.cpp:6789), so the aura has to live on the demon.
(200409, 'spell_warl_demon_dodge'),
(200410, 'spell_warl_demon_fel_synergy'),
(200412, 'spell_warl_demon_brutality'),
(200414, 'spell_warl_demon_lash'),
(200416, 'spell_warl_demon_fel_domination'),
(200418, 'spell_warl_demon_mana_feed'),
(200421, 'spell_warl_demon_resilience'),
(200422, 'spell_warl_demon_nemesis'),

-- Fel Attunement.  A single spell carries haste (aura 193), spell crit (71)
-- and melee crit (52).
(200419, 'spell_warl_demon_attunement'),

-- ---------------------------------------------------------------------------
-- Metamorphosis (WarlockMetamorphosis.cpp)
-- ---------------------------------------------------------------------------
-- One loader supplying both a SpellScript (entry cost) and an AuraScript
-- (6-second upkeep and auto-cancel).
(47241, 'spell_warl_metamorphosis');
