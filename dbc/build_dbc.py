"""
build_dbc.py - Automated DBC build pipeline for Alonecraft

Reads the base Spell.dbc, applies custom/modified spells from the
alonecraft_spell_dbc MySQL table, writes a patched Spell.dbc, and
packs it into patch-4.mpq for the WoW 3.3.5a client.
If patch-4.mpq already exists, the Spell.dbc is replaced in-place
so that all other DBC files in the patch are preserved.

Usage: python build_dbc.py
"""

import csv
import os
import struct
import subprocess
import sys

import mysql.connector

import config

# ── DBC Format ──────────────────────────────────────────────────────────────
# The server format (SpellEntryfmt in DBCfmt.h) marks client-only fields as 'x'.
# For the client DBC we need the TRUE binary types. SpellDescription and SpellToolTip
# locale slots are string offsets in the binary even though the server skips them.
#
# Key: n/i = int32, f = float32, s = string offset (uint32), x = int32 (unused by server)
# All fields are 4 bytes in the binary. 234 fields total, record size = 936 bytes.

_SERVER_FMT = (
    "niiiiiiiiiiiixixiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiifx"
    "iiiiiiiiiiiiiiiiiiiiiiiiiiiifffiiiiiiiiiiiiiiiiii"
    "iiifffiiiiiiiiiiiiiiifffiiiiiiiiiiiiiissssssssssssssssxssssssssssssssss"
    "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxiiiiiiiiiiixfffxxxiiiiixxfffxx"
)

# Build client format: override 'x' -> 's' for Description and Tooltip locale slots
_fmt = list(_SERVER_FMT)
for _i in range(170, 179):  # SpellDescription0-8
    _fmt[_i] = "s"
for _i in range(187, 196):  # SpellToolTip0-8
    _fmt[_i] = "s"
SPELL_FMT = "".join(_fmt)

WDBC_MAGIC = b"WDBC"
FIELD_COUNT = len(SPELL_FMT)  # 234
RECORD_SIZE = FIELD_COUNT * 4  # 936

# Indices of float fields (format char 'f')
FLOAT_INDICES = frozenset(i for i, c in enumerate(SPELL_FMT) if c == "f")

# Indices of string fields (format char 's') — stored as uint32 offsets into string block
STRING_INDICES = frozenset(i for i, c in enumerate(SPELL_FMT) if c == "s")

# CSV header gives us canonical column names in order
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
REPO_ROOT = os.path.abspath(os.path.join(SCRIPT_DIR, "..", "..", ".."))
CSV_PATH = os.path.join(REPO_ROOT, "DBC_data", "Spell.csv")


def load_field_names():
    """Read the 234 column names from the Spell.csv header."""
    with open(CSV_PATH, "r", encoding="utf-8") as f:
        reader = csv.reader(f)
        header = next(reader)
    if len(header) != FIELD_COUNT:
        sys.exit(f"ERROR: Spell.csv has {len(header)} columns, expected {FIELD_COUNT}")
    return header


# ── DBC Read/Write ──────────────────────────────────────────────────────────

def read_base_dbc(path):
    """Read a WDBC file. Returns (records_dict, string_block_bytes).

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


def encode_record(row, field_names, string_block, new_strings):
    """Encode a SQL row dict into a 936-byte DBC record.

    row: dict mapping column names to values
    string_block: existing string block bytes (to calculate base offset for new strings)
    new_strings: list of (bytes,) — mutable, new encoded strings are appended here

    Returns 936 bytes.
    """
    parts = []
    base_offset = len(string_block)
    # Running offset accounting for strings already queued in this batch
    extra_offset = sum(len(b) for b in new_strings)

    for i in range(FIELD_COUNT):
        col = field_names[i]
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


# ── MPQ Packing ─────────────────────────────────────────────────────────────

def pack_mpq(output_dir):
    """Pack the output DBC into patch-4.mpq using mpqcli.

    If patch-4.mpq already exists, Spell.dbc is replaced in-place so that
    all other DBC files in the patch are preserved.  If it doesn't exist,
    a fresh MPQ is created.
    """
    mpq_path = os.path.join(output_dir, "patch-4.mpq")
    dbc_file = os.path.join(output_dir, "DBFilesClient", "Spell.dbc")

    # Use local mpqcli.exe next to this script, or fall back to PATH
    local_mpqcli = os.path.join(SCRIPT_DIR, "mpqcli.exe")
    mpqcli = local_mpqcli if os.path.isfile(local_mpqcli) else "mpqcli"

    if os.path.isfile(mpq_path):
        # Update existing MPQ in-place — preserves all other DBC files
        cmd = [
            mpqcli, "add",
            "--overwrite",
            "--game", "wow-wotlk",
            "-p", "DBFilesClient\\Spell.dbc",
            dbc_file,
            mpq_path,
        ]
        action = "updated"
    else:
        # No existing MPQ — create a fresh one
        cmd = [
            mpqcli, "create",
            "--game", "wow-wotlk",
            "--output", mpq_path,
            "--name-in-archive", "DBFilesClient\\Spell.dbc",
            dbc_file,
        ]
        action = "created"

    print(f"Running: {' '.join(cmd)}")
    try:
        result = subprocess.run(cmd, capture_output=True, text=True)
        if result.returncode != 0:
            print(f"mpqcli error: {result.stderr}")
            print("WARNING: mpqcli failed. Spell.dbc was still written - pack it manually.")
            return False
        print(result.stdout.strip())
        print(f"MPQ {action}: {mpq_path}")
        return True
    except FileNotFoundError:
        print("WARNING: mpqcli not found. Spell.dbc was still written - pack it manually.")
        print("Download mpqcli.exe from: https://github.com/TheGrayDot/mpqcli/releases")
        return False


# ── Main ────────────────────────────────────────────────────────────────────

def main():
    print("=" * 60)
    print("Alonecraft DBC Build Pipeline")
    print("=" * 60)

    field_names = load_field_names()

    if not os.path.isfile(config.BASE_DBC_PATH):
        sys.exit(f"ERROR: Base DBC not found: {config.BASE_DBC_PATH}")
    records, string_block = read_base_dbc(config.BASE_DBC_PATH)

    overrides = fetch_overrides(field_names)

    if not overrides:
        print("No overrides to apply. Output will match the base DBC.")

    new_strings = []
    added = 0
    modified = 0
    for row in overrides:
        spell_id = int(row["ID"])
        record_bytes = encode_record(row, field_names, string_block, new_strings)
        if spell_id in records:
            modified += 1
        else:
            added += 1
        records[spell_id] = record_bytes

    if new_strings:
        string_block = string_block + b"".join(new_strings)

    print(f"Applied {len(overrides)} override(s): {added} new, {modified} modified")

    output_dir = os.path.join(SCRIPT_DIR, config.OUTPUT_DIR)
    dbc_out = os.path.join(output_dir, "DBFilesClient", "Spell.dbc")
    write_dbc(dbc_out, records, string_block)

    pack_mpq(output_dir)

    print("=" * 60)
    print("Done!")
    print(f"  DBC: {os.path.abspath(dbc_out)}")
    print(f"  MPQ: {os.path.abspath(os.path.join(output_dir, 'patch-4.mpq'))}")
    print("=" * 60)


if __name__ == "__main__":
    main()
