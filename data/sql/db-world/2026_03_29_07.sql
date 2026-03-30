-- Alonecraft: spell_script_names corrections
-- Fixes wrong script names and adds missing entries discovered at server startup.

-- Remove entries that have no C++ implementation
DELETE FROM `spell_script_names` WHERE (`spell_id`, `ScriptName`) IN (
    (-44543, 'spell_mage_fingers_of_frost_proc_aura'), -- no code exists for this script
    (29441,  'spell_magic_absorption_rank_1'),          -- MagicAbsorption.cpp uses 'spell_magic_absorption_absorb', not this
    (29444,  'spell_magic_absorption_rank_2')           -- same
);

-- 200037 is an aura spell (not a damage spell); the -30455 family registration
-- already covers Ice Lance damage spells for spell_mage_ice_lance_heal.
DELETE FROM `spell_script_names` WHERE `spell_id` = 200037;

-- Add missing entries: scripts with C++ code but no DB entry
DELETE FROM `spell_script_names` WHERE (`spell_id`, `ScriptName`) IN (
    (33891, 'spell_no_swing_reset_arakkoa'),  -- arakkoaswingtimer.cpp, Arakkoa form aura
    (11247, 'spell_magic_attunement_rank_1'), -- MagicAttunement.cpp R1
    (12606, 'spell_magic_attunement_rank_2')  -- MagicAttunement.cpp R2
);
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
    (33891, 'spell_no_swing_reset_arakkoa'),  -- Arakkoa form: prevents swing timer reset
    (11247, 'spell_magic_attunement_rank_1'), -- Magic Attunement R1: mana refund during Invocation
    (12606, 'spell_magic_attunement_rank_2'); -- Magic Attunement R2
