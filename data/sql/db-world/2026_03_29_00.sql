-- Alonecraft: spell_script_names
-- Registers C++ script hooks for all custom spell mechanics.
-- Negative spell_id values apply to all ranks of a spell family group.

-- Remove any pre-existing entries before inserting (idempotent)
DELETE FROM `spell_script_names` WHERE (`spell_id`, `ScriptName`) IN (
    -- Druid
    (-5176,'spell_natures_focus_wrath'),
    (17123,'aura_proc_moonfire_only'),
    (17124,'aura_proc_moonfire_only'),
    (18562,'spell_swiftmend_trigger'),
    (200000,'spell_bloomstrike'),
    (200006,'aura_proc_self_hot_only'),
    (200007,'aura_proc_self_hot_only'),
    (200008,'aura_proc_self_hot_only'),
    (200009,'aura_proc_self_hot_only'),
    (200010,'aura_proc_self_hot_only'),
    -- Mage (Arcane)
    (-1463,'spell_mage_arcane_fortitude_mana_shield'),
    (-1008,'spell_mage_aegis_of_antonidas'),
    (29441,'spell_magic_absorption_rank_1'),
    (29444,'spell_magic_absorption_rank_2'),
    (200035,'spell_magic_absorption_absorb'),
    (200036,'spell_magic_absorption_absorb'),
    -- Mage (Fire)
    (-2136,'spell_firebreak'),
    (-6117,'spell_mage_mage_armor_custom'),
    (30482,'spell_molten_armor_apply'),
    (43045,'spell_molten_armor_apply'),
    (43046,'spell_molten_armor_apply'),
    (31638,'spell_spark_of_alar'),
    (31639,'spell_spark_of_alar'),
    (31640,'spell_spark_of_alar'),
    (54747,'spell_mage_burning_determination'),
    (54749,'spell_mage_burning_determination'),
    (200023,'spell_ember_scars'),
    (200040,'spell_molten_armor_crit_aura'),
    -- Mage (Frost)
    (-44543,'spell_mage_fingers_of_frost_proc_aura'),
    (-30455,'spell_mage_ice_lance_heal'),
    (-7302,'spell_frost_ice_armor_enhanced'),
    (-168,'spell_frost_ice_armor_enhanced'),
    (11083,'spell_mage_fire_leech'),
    (11160,'spell_mage_ablative_armor'),
    (12351,'spell_mage_fire_leech'),
    (12518,'spell_mage_ablative_armor'),
    (12519,'spell_mage_ablative_armor'),
    (55080,'spell_mage_shattered_barrier_cooldown_reset'),
    (200032,'spell_permafrost_absorb'),
    (200033,'spell_permafrost_absorb'),
    (200034,'spell_permafrost_absorb'),
    (200037,'spell_ice_lance_heal'),
    -- Paladin
    (42463,'spell_pal_seal_of_vengeance'),
    (53739,'spell_pal_seal_of_vengeance'),
    -- Priest
    (-976,'spell_mark_of_penitence'),
    (-724,'spell_pri_lightwell'),
    (200060,'spell_priest_aura_interaction'),
    (200073,'spell_divine_aegis_crit_tracker'),
    (200074,'spell_divine_aegis_crit_tracker'),
    (200075,'spell_divine_aegis_crit_tracker'),
    (200086,'spell_pri_refresh_devouring_plague'),
    (200087,'spell_pri_refresh_mark_of_penitence')
);

-- Remove old Arcane Fortitude entry that was replaced (was spell_mage_mana_shield)
DELETE FROM `spell_script_names` WHERE `spell_id` = -1463 AND `ScriptName` = 'spell_mage_mana_shield';

INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
    -- Druid
    (-5176,'spell_natures_focus_wrath'),           -- Nature's Focus (all ranks)
    (17123,'aura_proc_moonfire_only'),              -- Lunar Storm proc aura R1
    (17124,'aura_proc_moonfire_only'),              -- Lunar Storm proc aura R2
    (18562,'spell_swiftmend_trigger'),              -- Swiftmend trigger
    (200000,'spell_bloomstrike'),                   -- Bloomstrike
    (200006,'aura_proc_self_hot_only'),             -- Empowered Touch R1
    (200007,'aura_proc_self_hot_only'),             -- Empowered Touch R2
    (200008,'aura_proc_self_hot_only'),             -- Empowered Touch R3
    (200009,'aura_proc_self_hot_only'),             -- Empowered Touch R4
    (200010,'aura_proc_self_hot_only'),             -- Empowered Touch R5
    -- Mage (Arcane)
    (-1463,'spell_mage_arcane_fortitude_mana_shield'), -- Arcane Fortitude (all ranks)
    (-1008,'spell_mage_aegis_of_antonidas'),        -- Aegis of Antonidas (all ranks)
    (29441,'spell_magic_absorption_rank_1'),        -- Magic Absorption R1
    (29444,'spell_magic_absorption_rank_2'),        -- Magic Absorption R2
    (200035,'spell_magic_absorption_absorb'),       -- Magic Absorption absorb R1
    (200036,'spell_magic_absorption_absorb'),       -- Magic Absorption absorb R2
    -- Mage (Fire)
    (-2136,'spell_firebreak'),                      -- Firebreak (all ranks)
    (-6117,'spell_mage_mage_armor_custom'),         -- Mage Armor (all ranks)
    (30482,'spell_molten_armor_apply'),             -- Molten Armor R1
    (43045,'spell_molten_armor_apply'),             -- Molten Armor R2
    (43046,'spell_molten_armor_apply'),             -- Molten Armor R3
    (31638,'spell_spark_of_alar'),                  -- Spark of Al'ar R1
    (31639,'spell_spark_of_alar'),                  -- Spark of Al'ar R2
    (31640,'spell_spark_of_alar'),                  -- Spark of Al'ar R3
    (54747,'spell_mage_burning_determination'),     -- Burning Determination R1
    (54749,'spell_mage_burning_determination'),     -- Burning Determination R2
    (200023,'spell_ember_scars'),                   -- Ember Scars DoT
    (200040,'spell_molten_armor_crit_aura'),        -- Molten Armor crit tracker
    -- Mage (Frost)
    (-44543,'spell_mage_fingers_of_frost_proc_aura'), -- Fingers of Frost proc (all ranks)
    (-30455,'spell_mage_ice_lance_heal'),           -- Ice Lance heal (all ranks)
    (-7302,'spell_frost_ice_armor_enhanced'),       -- Frost Armor enhanced (family group A)
    (-168,'spell_frost_ice_armor_enhanced'),        -- Frost/Ice Armor enhanced (family group B)
    (11083,'spell_mage_fire_leech'),                -- Fire Leech R1
    (11160,'spell_mage_ablative_armor'),            -- Ablative Armor R1
    (12351,'spell_mage_fire_leech'),                -- Fire Leech R2
    (12518,'spell_mage_ablative_armor'),            -- Ablative Armor R2
    (12519,'spell_mage_ablative_armor'),            -- Ablative Armor R3
    (55080,'spell_mage_shattered_barrier_cooldown_reset'), -- Shattered Barrier
    (200032,'spell_permafrost_absorb'),             -- Permafrost absorb R1
    (200033,'spell_permafrost_absorb'),             -- Permafrost absorb R2
    (200034,'spell_permafrost_absorb'),             -- Permafrost absorb R3
    (200037,'spell_ice_lance_heal'),                -- Ice Lance heal trigger
    -- Paladin
    (42463,'spell_pal_seal_of_vengeance'),          -- Seal of Vengeance R1
    (53739,'spell_pal_seal_of_vengeance'),          -- Seal of Vengeance R2
    -- Priest
    (-976,'spell_mark_of_penitence'),               -- Mark of Penitence (all ranks)
    (-724,'spell_pri_lightwell'),                   -- Lightform (hooks into Lightwell spell family)
    (200060,'spell_priest_aura_interaction'),       -- Lightform aura
    (200073,'spell_divine_aegis_crit_tracker'),     -- Divine Aegis crit tracker R1
    (200074,'spell_divine_aegis_crit_tracker'),     -- Divine Aegis crit tracker R2
    (200075,'spell_divine_aegis_crit_tracker'),     -- Divine Aegis crit tracker R3
    (200086,'spell_pri_refresh_devouring_plague'),  -- Refresh DoTs via Mind Blast
    (200087,'spell_pri_refresh_mark_of_penitence'); -- Refresh Mark of Penitence via Smite
