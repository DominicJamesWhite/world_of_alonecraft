-- Fix Lightform (200060) being consumed on any spell cast
-- Root cause: Form 23 in SpellShapeshiftForm.dbc has flags=0x0 (full shapeshift),
-- so the client auto-cancels it before any spell cast. With the DBC patched to
-- flags=0x109 (STANCE | CAN_NPC_INTERACT | DONT_AUTO_UNSHIFT), the client no
-- longer auto-cancels. Instead, the AuraScript's OnProc handler cancels Lightform
-- only when the priest casts a non-Holy spell.

-- Server-side shapeshift form override: set Lightform form (23) flags to match
-- Shadowform behavior (stance + NPC interact) plus DONT_AUTO_UNSHIFT.
DELETE FROM `spellshapeshiftform_dbc` WHERE `ID` = 23;
INSERT INTO `spellshapeshiftform_dbc` (`ID`, `BonusActionBar`,
    `Name_Lang_enUS`, `Name_Lang_enGB`, `Name_Lang_koKR`, `Name_Lang_frFR`,
    `Name_Lang_deDE`, `Name_Lang_enCN`, `Name_Lang_zhCN`, `Name_Lang_enTW`,
    `Name_Lang_zhTW`, `Name_Lang_esES`, `Name_Lang_esMX`, `Name_Lang_ruRU`,
    `Name_Lang_ptPT`, `Name_Lang_ptBR`, `Name_Lang_itIT`, `Name_Lang_Unk`,
    `Name_Lang_Mask`, `Flags`, `CreatureType`, `AttackIconID`,
    `CombatRoundTime`, `CreatureDisplayID_1`, `CreatureDisplayID_2`,
    `CreatureDisplayID_3`, `CreatureDisplayID_4`,
    `PresetSpellID_1`, `PresetSpellID_2`, `PresetSpellID_3`, `PresetSpellID_4`,
    `PresetSpellID_5`, `PresetSpellID_6`, `PresetSpellID_7`, `PresetSpellID_8`)
VALUES (23, 0,
    'Lightform', '', '', '', '', '', '', '', '', '', '', '', '', '', '', '',
    0, 265, 0, 0,   -- Flags=0x109 (STANCE|CAN_NPC_INTERACT|DONT_AUTO_UNSHIFT)
    0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0);

-- spell_proc for Lightform: trigger proc on any positive or negative magic spell cast.
-- The C++ CheckProc filters to only proc (and cancel) on non-Holy school spells.
-- ProcFlags = 0x14000 (DONE_SPELL_MAGIC_DMG_CLASS_POS | DONE_SPELL_MAGIC_DMG_CLASS_NEG)
-- SpellPhaseMask = 2 (PROC_SPELL_PHASE_HIT) — MUST be non-zero or proc silently never fires.
DELETE FROM `spell_proc` WHERE `SpellId` = 200060;
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`,
    `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`,
    `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`,
    `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`,
    `Cooldown`, `Charges`)
VALUES (200060, 0, 0,
    0, 0, 0,
    0x14000, 0, 2, 0,
    0, 0, 0, 100,
    0, 0);
