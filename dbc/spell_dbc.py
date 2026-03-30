"""
spell_dbc.py - Shared Spell.dbc constants and read/write utilities

Provides the canonical column names, format string, and binary DBC
read/write functions used by both build_dbc.py and gen_sql.py.
"""

import os
import struct
import sys

# ── Column Names ───────────────────────────────────────────────────────────
# The 234 Spell.dbc column names in binary field order.
# WoW 3.3.5a format is frozen -- these never change.

SPELL_COLUMNS = (
    "ID", "Category", "Dispel", "Mechanic", "Attributes", "AttributesEx",
    "AttributesEx2", "AttributesEx3", "AttributesEx4", "AttributesEx5",
    "AttributesEx6", "AttributesEx7", "Stances", "Unknown1", "StancesNot",
    "Unknown2", "Targets", "TargetCreatureType", "RequiresSpellFocus",
    "FacingCasterFlags", "CasterAuraState", "TargetAuraState",
    "CasterAuraStateNot", "TargetAuraStateNot", "CasterAuraSpell",
    "TargetAuraSpell", "ExcludeCasterAuraSpell", "ExcludeTargetAuraSpell",
    "CastingTimeIndex", "RecoveryTime", "CategoryRecoveryTime",
    "InterruptFlags", "AuraInterruptFlags", "ChannelInterruptFlags",
    "ProcFlags", "ProcChance", "ProcCharges", "MaximumLevel", "BaseLevel",
    "SpellLevel", "DurationIndex", "PowerType", "ManaCost",
    "ManaCostPerLevel", "ManaPerSecond", "ManaPerSecondPerLevel",
    "RangeIndex", "Speed", "ModalNextSpell", "StackAmount", "Totem1",
    "Totem2", "Reagent1", "Reagent2", "Reagent3", "Reagent4", "Reagent5",
    "Reagent6", "Reagent7", "Reagent8", "ReagentCount1", "ReagentCount2",
    "ReagentCount3", "ReagentCount4", "ReagentCount5", "ReagentCount6",
    "ReagentCount7", "ReagentCount8", "EquippedItemClass",
    "EquippedItemSubClassMask", "EquippedItemInventoryTypeMask", "Effect1",
    "Effect2", "Effect3", "EffectDieSides1", "EffectDieSides2",
    "EffectDieSides3", "EffectRealPointsPerLevel1",
    "EffectRealPointsPerLevel2", "EffectRealPointsPerLevel3",
    "EffectBasePoints1", "EffectBasePoints2", "EffectBasePoints3",
    "EffectMechanic1", "EffectMechanic2", "EffectMechanic3",
    "EffectImplicitTargetA1", "EffectImplicitTargetA2",
    "EffectImplicitTargetA3", "EffectImplicitTargetB1",
    "EffectImplicitTargetB2", "EffectImplicitTargetB3",
    "EffectRadiusIndex1", "EffectRadiusIndex2", "EffectRadiusIndex3",
    "EffectApplyAuraName1", "EffectApplyAuraName2", "EffectApplyAuraName3",
    "EffectAmplitude1", "EffectAmplitude2", "EffectAmplitude3",
    "EffectMultipleValue1", "EffectMultipleValue2", "EffectMultipleValue3",
    "EffectChainTarget1", "EffectChainTarget2", "EffectChainTarget3",
    "EffectItemType1", "EffectItemType2", "EffectItemType3",
    "EffectMiscValue1", "EffectMiscValue2", "EffectMiscValue3",
    "EffectMiscValueB1", "EffectMiscValueB2", "EffectMiscValueB3",
    "EffectTriggerSpell1", "EffectTriggerSpell2", "EffectTriggerSpell3",
    "EffectPointsPerComboPoint1", "EffectPointsPerComboPoint2",
    "EffectPointsPerComboPoint3", "EffectSpellClassMaskA1",
    "EffectSpellClassMaskA2", "EffectSpellClassMaskA3",
    "EffectSpellClassMaskB1", "EffectSpellClassMaskB2",
    "EffectSpellClassMaskB3", "EffectSpellClassMaskC1",
    "EffectSpellClassMaskC2", "EffectSpellClassMaskC3", "SpellVisual1",
    "SpellVisual2", "SpellIconID", "ActiveIconID", "SpellPriority",
    "SpellName0", "SpellName1", "SpellName2", "SpellName3", "SpellName4",
    "SpellName5", "SpellName6", "SpellName7", "SpellName8",
    "SpellNameFlag0", "SpellNameFlag1", "SpellNameFlag2", "SpellNameFlag3",
    "SpellNameFlag4", "SpellNameFlag5", "SpellNameFlag6", "SpellNameFlag7",
    "SpellRank0", "SpellRank1", "SpellRank2", "SpellRank3", "SpellRank4",
    "SpellRank5", "SpellRank6", "SpellRank7", "SpellRank8",
    "SpellRankFlags0", "SpellRankFlags1", "SpellRankFlags2",
    "SpellRankFlags3", "SpellRankFlags4", "SpellRankFlags5",
    "SpellRankFlags6", "SpellRankFlags7", "SpellDescription0",
    "SpellDescription1", "SpellDescription2", "SpellDescription3",
    "SpellDescription4", "SpellDescription5", "SpellDescription6",
    "SpellDescription7", "SpellDescription8", "SpellDescriptionFlags0",
    "SpellDescriptionFlags1", "SpellDescriptionFlags2",
    "SpellDescriptionFlags3", "SpellDescriptionFlags4",
    "SpellDescriptionFlags5", "SpellDescriptionFlags6",
    "SpellDescriptionFlags7", "SpellToolTip0", "SpellToolTip1",
    "SpellToolTip2", "SpellToolTip3", "SpellToolTip4", "SpellToolTip5",
    "SpellToolTip6", "SpellToolTip7", "SpellToolTip8",
    "SpellToolTipFlags0", "SpellToolTipFlags1", "SpellToolTipFlags2",
    "SpellToolTipFlags3", "SpellToolTipFlags4", "SpellToolTipFlags5",
    "SpellToolTipFlags6", "SpellToolTipFlags7", "ManaCostPercentage",
    "StartRecoveryCategory", "StartRecoveryTime", "MaximumTargetLevel",
    "SpellFamilyName", "SpellFamilyFlags", "SpellFamilyFlags1",
    "SpellFamilyFlags2", "MaximumAffectedTargets", "DamageClass",
    "PreventionType", "StanceBarOrder", "EffectDamageMultiplier1",
    "EffectDamageMultiplier2", "EffectDamageMultiplier3",
    "MinimumFactionId", "MinimumReputation", "RequiredAuraVision",
    "TotemCategory1", "TotemCategory2", "AreaGroupID", "SchoolMask",
    "RuneCostID", "SpellMissileID", "PowerDisplayId",
    "EffectBonusMultiplier1", "EffectBonusMultiplier2",
    "EffectBonusMultiplier3", "SpellDescriptionVariableID",
    "SpellDifficultyID",
)

# ── DBC Format ─────────────────────────────────────────────────────────────
# Server format (SpellEntryfmt in DBCfmt.h) marks client-only fields as 'x'.
# For the client DBC we need the TRUE binary types: SpellDescription and
# SpellToolTip locale slots are string offsets even though the server skips them.
#
# Key: n/i = int32, f = float32, s = string offset (uint32), x = int32

_SERVER_FMT = (
    "niiiiiiiiiiiixixiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiifx"
    "iiiiiiiiiiiiiiiiiiiiiiiiiiiifffiiiiiiiiiiiiiiiiii"
    "iiifffiiiiiiiiiiiiiiifffiiiiiiiiiiiiiissssssssssssssssxssssssssssssssss"
    "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxiiiiiiiiiiixfffxxxiiiiixxfffxx"
)

_fmt = list(_SERVER_FMT)
for _i in range(170, 179):  # SpellDescription0-8
    _fmt[_i] = "s"
for _i in range(187, 196):  # SpellToolTip0-8
    _fmt[_i] = "s"
SPELL_FMT = "".join(_fmt)

WDBC_MAGIC = b"WDBC"
FIELD_COUNT = len(SPELL_FMT)  # 234
RECORD_SIZE = FIELD_COUNT * 4  # 936

assert FIELD_COUNT == len(SPELL_COLUMNS), (
    f"SPELL_FMT has {FIELD_COUNT} fields but SPELL_COLUMNS has {len(SPELL_COLUMNS)}"
)

# Indices of float fields (format char 'f')
FLOAT_INDICES = frozenset(i for i, c in enumerate(SPELL_FMT) if c == "f")

# Indices of string fields (format char 's')
STRING_INDICES = frozenset(i for i, c in enumerate(SPELL_FMT) if c == "s")

# ── Column Type Sets (for SQL generation) ──────────────────────────────────

FLOAT_COLUMNS = {
    "Speed",
    "EffectRealPointsPerLevel1", "EffectRealPointsPerLevel2", "EffectRealPointsPerLevel3",
    "EffectMultipleValue1", "EffectMultipleValue2", "EffectMultipleValue3",
    "EffectPointsPerComboPoint1", "EffectPointsPerComboPoint2", "EffectPointsPerComboPoint3",
    "EffectDamageMultiplier1", "EffectDamageMultiplier2", "EffectDamageMultiplier3",
    "EffectBonusMultiplier1", "EffectBonusMultiplier2", "EffectBonusMultiplier3",
}

# TEXT columns in the DDL.  Note: SpellNameFlag7, SpellRankFlags7,
# SpellDescriptionFlags0-7, and SpellToolTipFlags0-7 are INT, not TEXT.
TEXT_COLUMNS = set()
for _prefix in ("SpellName", "SpellRank", "SpellDescription", "SpellToolTip"):
    for _i in range(9):
        TEXT_COLUMNS.add(f"{_prefix}{_i}")
for _prefix in ("SpellNameFlag", "SpellRankFlags"):
    for _i in range(7):  # 0-6 are TEXT; 7 is INT
        TEXT_COLUMNS.add(f"{_prefix}{_i}")


# ── DBC Read/Write ─────────────────────────────────────────────────────────

def read_base_dbc(path):
    """Read a WDBC Spell.dbc file. Returns (records_dict, string_block_bytes).

    records_dict: spell_id (int) -> raw record bytes (936 bytes)
    """
    with open(path, "rb") as f:
        magic = f.read(4)
        if magic != WDBC_MAGIC:
            sys.exit(f"ERROR: {path} is not a valid WDBC file (magic: {magic})")

        record_count, field_count, record_size, string_size = struct.unpack("<4I", f.read(16))
        if field_count != FIELD_COUNT:
            sys.exit(f"ERROR: Spell.dbc has {field_count} fields, expected {FIELD_COUNT}")
        if record_size != RECORD_SIZE:
            sys.exit(f"ERROR: Spell.dbc record size {record_size}, expected {RECORD_SIZE}")

        records = {}
        for _ in range(record_count):
            data = f.read(record_size)
            spell_id = struct.unpack_from("<I", data, 0)[0]
            records[spell_id] = data

        string_block = f.read(string_size)

    print(f"Read {len(records)} spells from {path}")
    print(f"String block: {len(string_block)} bytes")
    return records, string_block


def decode_record(raw_bytes, string_block):
    """Unpack a 936-byte Spell.dbc record into a {column_name: value} dict.

    Integer fields become int, float fields become float, string fields
    become str (resolved from the string block).
    """
    row = {}
    for i in range(FIELD_COUNT):
        offset = i * 4
        col = SPELL_COLUMNS[i]
        fmt_char = SPELL_FMT[i]

        if fmt_char == "f":
            row[col] = struct.unpack_from("<f", raw_bytes, offset)[0]
        elif fmt_char == "s":
            str_offset = struct.unpack_from("<I", raw_bytes, offset)[0]
            if str_offset == 0:
                row[col] = ""
            else:
                end = string_block.index(b"\x00", str_offset)
                row[col] = string_block[str_offset:end].decode("utf-8", errors="replace")
        else:
            # 'n' (signed) or 'i' (unsigned) -- treat as signed to preserve
            # negative values (e.g. EffectBasePoints can be negative)
            row[col] = struct.unpack_from("<i", raw_bytes, offset)[0]

    return row


def load_spell_index(dbc_path):
    """Read a Spell.dbc and return {spell_id: {column: value}} dict.

    This is the direct replacement for the old CSV-based spell index.
    """
    records, string_block = read_base_dbc(dbc_path)
    return {sid: decode_record(data, string_block) for sid, data in records.items()}


def encode_record(row, string_block, new_strings):
    """Encode a row dict into a 936-byte DBC record.

    row: dict mapping column names to values
    string_block: existing string block bytes (base offset for new strings)
    new_strings: list of bytes -- mutable, new encoded strings appended here

    Returns 936 bytes.
    """
    parts = []
    base_offset = len(string_block)
    extra_offset = sum(len(b) for b in new_strings)

    for i in range(FIELD_COUNT):
        col = SPELL_COLUMNS[i]
        val = row.get(col, 0)

        if i in STRING_INDICES:
            text = str(val) if val else ""
            if not text:
                parts.append(struct.pack("<I", 0))
            else:
                offset = base_offset + extra_offset
                encoded = text.encode("utf-8") + b"\x00"
                new_strings.append(encoded)
                extra_offset += len(encoded)
                parts.append(struct.pack("<I", offset))
        elif i in FLOAT_INDICES:
            parts.append(struct.pack("<f", float(val) if val is not None else 0.0))
        else:
            ival = int(val) if val is not None else 0
            parts.append(struct.pack("<i", ival) if ival < 0 else struct.pack("<I", ival))

    return b"".join(parts)


def write_dbc(path, records, string_block):
    """Write a WDBC file from records dict and string block."""
    sorted_ids = sorted(records.keys())
    record_count = len(sorted_ids)

    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "wb") as f:
        f.write(WDBC_MAGIC)
        f.write(struct.pack("<4I", record_count, FIELD_COUNT, RECORD_SIZE, len(string_block)))
        for spell_id in sorted_ids:
            f.write(records[spell_id])
        f.write(string_block)

    print(f"Wrote {record_count} spells to {path}")
    print(f"File size: {os.path.getsize(path):,} bytes")
