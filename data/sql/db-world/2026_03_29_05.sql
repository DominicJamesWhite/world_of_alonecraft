-- Alonecraft: spell_proc
-- Proc trigger configuration for custom Alonecraft spell mechanics.
-- Negative SpellId values apply to all ranks of a spell family group.
-- spell_proc_event was removed in a later AC version; this is the replacement table.
-- Column order: SpellId, SchoolMask, SpellFamilyName, SpellFamilyMask0, SpellFamilyMask1,
--               SpellFamilyMask2, ProcFlags, SpellTypeMask, SpellPhaseMask, HitMask,
--               AttributesMask, DisableEffectsMask, ProcsPerMinute, Chance, Cooldown, Charges

DELETE FROM `spell_proc` WHERE `SpellId` IN (
    -- Talent family groups (negative IDs)
    -33879, -11190,
    -- Specific spells
    14911, 15018,
    -- Druid: Lunar Storm proc auras
    17123, 17124,
    -- Alonecraft custom spell IDs (200000+ range)
    200006, 200007, 200008, 200009, 200010,
    200016, 200041, 200045,
    200050, 200052, 200053, 200054, 200055, 200056,
    200058, 200059,
    200063, 200064, 200065, 200066, 200067, 200068,
    200073, 200074, 200075,
    200080, 200081, 200082,
    200084, 200085, 200089, 200090, 200092, 200093
);

INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
    -- Talent family groups
    (-33879, 0, 7, 96, 33554432, 0, 65536, 0, 2, 0, 0, 0, 0, 0, 0, 0),
    (-11190, 0, 3, 160, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0),
    -- Specific spells
    (14911,  0, 6, 512, 0, 0, 0, 0, 2, 0, 0, 0, 0, 50, 30000, 0),
    (15018,  0, 6, 512, 0, 0, 0, 0, 2, 0, 0, 0, 0, 100, 30000, 0),
    -- Druid: Lunar Storm proc auras
    (17123,  0, 7, 4194304, 0, 0, 0, 0, 2, 0, 0, 0, 0, 50, 0, 0),
    (17124,  0, 7, 4194304, 0, 0, 0, 0, 2, 0, 0, 0, 0, 100, 0, 0),
    -- Empowered Touch (aura_proc_self_hot_only)
    (200006, 0, 7, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 10, 0, 0),
    (200007, 0, 7, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 8, 0, 0),
    (200008, 0, 7, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 6, 0, 0),
    (200009, 0, 7, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 4, 0, 0),
    (200010, 0, 7, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 2, 0, 0),
    -- Swiftmend trigger (procs on Swiftmend cast -> AoE damage explosion)
    (200016, 0, 7, 0, 2, 0, 16384, 0, 2, 0, 0, 0, 0, 100, 0, 0),
    -- Molten Armor crit tracker (procs on crit: HitMask=2)
    (200041, 4, 3, 0, 0, 0, 65536, 0, 2, 2, 0, 0, 0, 100, 0, 0),
    (200045, 0, 10, 0, 1048576, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0),
    -- Paladin procs
    (200050, 0, 10, 1073741824, 0, 0, 0, 0, 2, 0, 0, 0, 0, 100, 0, 0),
    (200052, 0, 10, 1073741824, 0, 0, 0, 0, 2, 0, 0, 0, 0, 50, 0, 0),
    (200053, 0, 10, 0, 2, 0, 0, 0, 2, 0, 0, 0, 0, 50, 0, 0),
    (200054, 0, 10, 1073741824, 0, 0, 0, 0, 2, 0, 0, 0, 0, 100, 0, 0),
    (200055, 0, 10, 0, 2, 0, 0, 0, 2, 0, 0, 0, 0, 100, 0, 0),
    (200056, 0, 10, 0, 0, 0, 4, 0, 0, 0, 0, 0, 2, 0, 0, 0),
    (200058, 0, 10, 0, 0, 0, 4, 0, 0, 0, 0, 0, 4, 0, 0, 0),
    (200059, 0, 10, 0, 0, 0, 4, 0, 0, 0, 0, 0, 6, 0, 0, 0),
    -- Priest: Divine Aegis / Lightform procs (proc on crit: HitMask=2)
    (200063, 2, 6, 0, 0, 0, 0, 0, 2, 2, 0, 0, 0, 0, 0, 0),
    (200064, 2, 6, 0, 0, 0, 0, 0, 2, 2, 0, 0, 0, 0, 0, 0),
    (200065, 2, 6, 0, 0, 0, 0, 0, 2, 2, 0, 0, 0, 0, 0, 0),
    (200066, 0, 6, 128, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0),
    (200067, 0, 6, 128, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0),
    (200068, 0, 6, 128, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0),
    -- Divine Aegis crit trackers (proc on crit: HitMask=2)
    (200073, 2, 6, 0, 0, 0, 0, 0, 2, 2, 0, 0, 0, 0, 0, 0),
    (200074, 2, 6, 0, 0, 0, 0, 0, 2, 2, 0, 0, 0, 0, 0, 0),
    (200075, 2, 6, 0, 0, 0, 0, 0, 2, 2, 0, 0, 0, 0, 0, 0),
    -- Priest DoT refresh procs
    (200080, 32, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0),
    (200081, 32, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0),
    (200082, 32, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0),
    (200084, 0, 0, 8192, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0),
    (200085, 0, 0, 128, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0),
    (200089, 0, 0, 8192, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0),
    (200090, 0, 0, 128, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0),
    (200092, 0, 0, 8192, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0),
    (200093, 0, 0, 128, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0);
