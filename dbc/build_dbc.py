"""
build_dbc.py - Automated DBC build pipeline for Alonecraft

Reads the base Spell.dbc, applies custom/modified spells from the
alonecraft_spell_dbc MySQL table, writes a patched Spell.dbc, and
packs it into patch-4.mpq for the WoW 3.3.5a client.
If patch-4.mpq already exists, the Spell.dbc is replaced in-place
so that all other DBC files in the patch are preserved.

Usage: python build_dbc.py
"""

import os
import struct
import subprocess
import sys

import mysql.connector

import config
from spell_dbc import (
    SPELL_COLUMNS, SPELL_FMT, FIELD_COUNT, RECORD_SIZE, WDBC_MAGIC,
    FLOAT_INDICES, STRING_INDICES,
    read_base_dbc, encode_record, write_dbc,
)

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))

# ── Talent.dbc Format ──────────────────────────────────────────────────────
# TalentEntryfmt = "niiiiiiiixxxxixxixxixxx" — 23 fields, all int32, no strings/floats
TALENT_FIELD_COUNT = 23
TALENT_RECORD_SIZE = TALENT_FIELD_COUNT * 4  # 92 bytes
TALENT_COLUMNS = [
    "ID", "TabID", "TierID", "ColumnIndex",
    "SpellRank_1", "SpellRank_2", "SpellRank_3", "SpellRank_4", "SpellRank_5",
    "SpellRank_6", "SpellRank_7", "SpellRank_8", "SpellRank_9",
    "PrereqTalent_1", "PrereqTalent_2", "PrereqTalent_3",
    "PrereqRank_1", "PrereqRank_2", "PrereqRank_3",
    "Flags", "RequiredSpellID", "CategoryMask_1", "CategoryMask_2",
]


# ── Integer-only DBC Read/Write (Talent.dbc, etc.) ─────────────────────────

def read_int_dbc(path, field_count, record_size):
    """Read a WDBC file where all fields are int32 (no strings).

    Returns dict: id (int) -> list of int32 values.
    """
    with open(path, "rb") as f:
        magic = f.read(4)
        if magic != WDBC_MAGIC:
            sys.exit(f"ERROR: {path} is not a valid WDBC file (magic: {magic})")

        rc, fc, rs, ss = struct.unpack("<4I", f.read(16))
        if fc != field_count:
            sys.exit(f"ERROR: {path} has {fc} fields, expected {field_count}")
        if rs != record_size:
            sys.exit(f"ERROR: {path} record size {rs}, expected {record_size}")

        records = {}
        for _ in range(rc):
            data = f.read(rs)
            values = list(struct.unpack(f"<{field_count}I", data))
            records[values[0]] = values

        string_block = f.read(ss)

    print(f"Read {len(records)} records from {path}")
    return records, string_block


def write_int_dbc(path, records, field_count, record_size, string_block, sort_key=None):
    """Write a WDBC file where all fields are uint32 (no strings).

    sort_key: optional function(values_list) -> sort tuple.  If None, sorts by ID.
    """
    if sort_key:
        sorted_values = sorted(records.values(), key=sort_key)
    else:
        sorted_values = [records[k] for k in sorted(records.keys())]

    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "wb") as f:
        f.write(WDBC_MAGIC)
        f.write(struct.pack("<4I", len(sorted_values), field_count, record_size, len(string_block)))
        for values in sorted_values:
            f.write(struct.pack(f"<{field_count}I", *values))
        f.write(string_block)

    print(f"Wrote {len(sorted_values)} records to {path}")


# ── MySQL ───────────────────────────────────────────────────────────────────

def fetch_overrides(field_names):
    """Fetch custom/modified spells from alonecraft_spell_dbc table."""
    try:
        conn = mysql.connector.connect(
            host=config.MYSQL_HOST,
            user=config.MYSQL_USER,
            password=config.MYSQL_PASS,
            database=config.MYSQL_DB,
        )
    except mysql.connector.Error as e:
        sys.exit(f"ERROR: Cannot connect to MySQL: {e}")

    cursor = conn.cursor(dictionary=True)

    cursor.execute(
        "SELECT COUNT(*) AS cnt FROM information_schema.tables "
        "WHERE table_schema = %s AND table_name = 'alonecraft_spell_dbc'",
        (config.MYSQL_DB,),
    )
    if cursor.fetchone()["cnt"] == 0:
        print("WARNING: alonecraft_spell_dbc table does not exist yet. No overrides applied.")
        cursor.close()
        conn.close()
        return []

    cursor.execute("SELECT * FROM alonecraft_spell_dbc")
    rows = cursor.fetchall()
    cursor.close()
    conn.close()

    print(f"Fetched {len(rows)} custom spell(s) from alonecraft_spell_dbc")
    return rows


def fetch_talent_overrides():
    """Fetch custom talent entries from talent_dbc table."""
    try:
        conn = mysql.connector.connect(
            host=config.MYSQL_HOST,
            user=config.MYSQL_USER,
            password=config.MYSQL_PASS,
            database=config.MYSQL_DB,
        )
    except mysql.connector.Error as e:
        sys.exit(f"ERROR: Cannot connect to MySQL: {e}")

    cursor = conn.cursor(dictionary=True)
    cursor.execute("SELECT COUNT(*) AS cnt FROM talent_dbc")
    count = cursor.fetchone()["cnt"]
    if count == 0:
        print("No talent overrides in talent_dbc.")
        cursor.close()
        conn.close()
        return []

    cursor.execute("SELECT * FROM talent_dbc")
    rows = cursor.fetchall()
    cursor.close()
    conn.close()

    print(f"Fetched {len(rows)} custom talent(s) from talent_dbc")
    return rows


# ── MPQ Packing ─────────────────────────────────────────────────────────────

def pack_mpq(output_dir, dbc_files):
    """Pack output DBC file(s) into patch-4.mpq using mpqcli.

    dbc_files: list of filenames relative to DBFilesClient/ (e.g. ["Spell.dbc", "Talent.dbc"])

    If patch-4.mpq already exists, files are replaced in-place so that
    all other DBC files in the patch are preserved.  If it doesn't exist,
    a fresh MPQ is created with the first file, then remaining files are added.
    """
    mpq_path = os.path.join(output_dir, "patch-4.mpq")

    # Use local mpqcli.exe next to this script, or fall back to PATH
    local_mpqcli = os.path.join(SCRIPT_DIR, "mpqcli.exe")
    mpqcli = local_mpqcli if os.path.isfile(local_mpqcli) else "mpqcli"

    success = True
    for idx, dbc_name in enumerate(dbc_files):
        dbc_file = os.path.join(output_dir, "DBFilesClient", dbc_name)
        archive_name = f"DBFilesClient\\{dbc_name}"

        if os.path.isfile(mpq_path):
            cmd = [
                mpqcli, "add",
                "--overwrite",
                "--game", "wow-wotlk",
                "-p", archive_name,
                dbc_file,
                mpq_path,
            ]
            action = "updated"
        else:
            cmd = [
                mpqcli, "create",
                "--game", "wow-wotlk",
                "--output", mpq_path,
                "--name-in-archive", archive_name,
                dbc_file,
            ]
            action = "created"

        print(f"Running: {' '.join(cmd)}")
        try:
            result = subprocess.run(cmd, capture_output=True, text=True)
            if result.returncode != 0:
                print(f"mpqcli error: {result.stderr}")
                print(f"WARNING: mpqcli failed for {dbc_name}. DBC was still written - pack manually.")
                success = False
                continue
            print(result.stdout.strip())
            print(f"MPQ {action} with {dbc_name}: {mpq_path}")
        except FileNotFoundError:
            print(f"WARNING: mpqcli not found. DBC files were still written - pack manually.")
            print("Download mpqcli.exe from: https://github.com/TheGrayDot/mpqcli/releases")
            return False

    return success


# ── Main ────────────────────────────────────────────────────────────────────

def main():
    print("=" * 60)
    print("Alonecraft DBC Build Pipeline")
    print("=" * 60)

    output_dir = os.path.join(SCRIPT_DIR, config.OUTPUT_DIR)
    dbc_files = []

    # ── Spell.dbc ──────────────────────────────────────────────
    print("\n--- Spell.dbc ---")

    if not os.path.isfile(config.BASE_DBC_PATH):
        sys.exit(f"ERROR: Base DBC not found: {config.BASE_DBC_PATH}")
    records, string_block = read_base_dbc(config.BASE_DBC_PATH)

    overrides = fetch_overrides(SPELL_COLUMNS)

    if not overrides:
        print("No overrides to apply. Output will match the base DBC.")

    new_strings = []
    added = 0
    modified = 0
    for row in overrides:
        spell_id = int(row["ID"])
        record_bytes = encode_record(row, string_block, new_strings)
        if spell_id in records:
            modified += 1
        else:
            added += 1
        records[spell_id] = record_bytes

    if new_strings:
        string_block = string_block + b"".join(new_strings)

    print(f"Applied {len(overrides)} override(s): {added} new, {modified} modified")

    spell_out = os.path.join(output_dir, "DBFilesClient", "Spell.dbc")
    write_dbc(spell_out, records, string_block)
    dbc_files.append("Spell.dbc")

    # ── Talent.dbc ─────────────────────────────────────────────
    base_talent = getattr(config, "BASE_TALENT_DBC_PATH", None)
    if base_talent and os.path.isfile(base_talent):
        print("\n--- Talent.dbc ---")
        talent_records, talent_sb = read_int_dbc(base_talent, TALENT_FIELD_COUNT, TALENT_RECORD_SIZE)
        talent_overrides = fetch_talent_overrides()

        t_added = 0
        t_modified = 0
        for row in talent_overrides:
            tid = int(row["ID"])
            values = [int(row.get(col, 0)) for col in TALENT_COLUMNS]
            if tid in talent_records:
                t_modified += 1
            else:
                t_added += 1
            talent_records[tid] = values

        print(f"Applied {len(talent_overrides)} override(s): {t_added} new, {t_modified} modified")

        talent_out = os.path.join(output_dir, "DBFilesClient", "Talent.dbc")
        # Client requires talents sorted by (TabID, TierID, ColumnIndex) — NOT by ID.
        # Incorrect ordering causes the entire talent tree to disappear.
        talent_sort_key = lambda v: (v[1], v[2], v[3])  # TabID, TierID, ColumnIndex
        write_int_dbc(talent_out, talent_records, TALENT_FIELD_COUNT, TALENT_RECORD_SIZE, talent_sb, sort_key=talent_sort_key)
        dbc_files.append("Talent.dbc")
    else:
        if base_talent:
            print(f"\nWARNING: Base Talent.dbc not found: {base_talent} — skipping talent patching.")
        # else: BASE_TALENT_DBC_PATH not configured, silently skip

    # ── SpellShapeshiftForm.dbc ────────────────────────────────
    base_shapeshift = getattr(config, "BASE_SHAPESHIFT_DBC_PATH", None)
    if base_shapeshift and os.path.isfile(base_shapeshift):
        print("\n--- SpellShapeshiftForm.dbc ---")
        SSF_FIELD_COUNT = 35
        SSF_RECORD_SIZE = SSF_FIELD_COUNT * 4  # 140 bytes
        ssf_records, ssf_sb = read_int_dbc(base_shapeshift, SSF_FIELD_COUNT, SSF_RECORD_SIZE)

        # Patch form 23 (Lightform): set flags to STANCE | CAN_NPC_INTERACT | DONT_AUTO_UNSHIFT
        LIGHTFORM_FORM_ID = 23
        LIGHTFORM_FLAGS = 0x109  # STANCE(0x01) | CAN_NPC_INTERACT(0x08) | DONT_AUTO_UNSHIFT(0x100)
        FLAGS_FIELD_INDEX = 19
        if LIGHTFORM_FORM_ID in ssf_records:
            old_flags = ssf_records[LIGHTFORM_FORM_ID][FLAGS_FIELD_INDEX]
            ssf_records[LIGHTFORM_FORM_ID][FLAGS_FIELD_INDEX] = LIGHTFORM_FLAGS
            print(f"Patched form {LIGHTFORM_FORM_ID}: flags {old_flags:#x} -> {LIGHTFORM_FLAGS:#x}")
        else:
            print(f"WARNING: Form {LIGHTFORM_FORM_ID} not found in SpellShapeshiftForm.dbc")

        ssf_out = os.path.join(output_dir, "DBFilesClient", "SpellShapeshiftForm.dbc")
        write_int_dbc(ssf_out, ssf_records, SSF_FIELD_COUNT, SSF_RECORD_SIZE, ssf_sb)
        dbc_files.append("SpellShapeshiftForm.dbc")
    else:
        if base_shapeshift:
            print(f"\nWARNING: Base SpellShapeshiftForm.dbc not found: {base_shapeshift} — skipping.")

    # ── MPQ Packing ────────────────────────────────────────────
    print()
    pack_mpq(output_dir, dbc_files)

    print("=" * 60)
    print("Done!")
    for dbc_name in dbc_files:
        print(f"  DBC: {os.path.abspath(os.path.join(output_dir, 'DBFilesClient', dbc_name))}")
    print(f"  MPQ: {os.path.abspath(os.path.join(output_dir, 'patch-4.mpq'))}")
    print("=" * 60)


if __name__ == "__main__":
    main()
