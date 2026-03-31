# Path to the base DBC files — includes all pre-tooling manual changes (4.45).
# These live in base/ next to this script and are checked into git.
# IMPORTANT: Do NOT point at the build output directory (C:/Build/bin/...),
# or stale changes will accumulate across rebuilds.
import os as _os
_BASE_DIR = _os.path.join(_os.path.dirname(_os.path.abspath(__file__)), "base")
BASE_DBC_PATH = _os.path.join(_BASE_DIR, "Spell.dbc")
BASE_TALENT_DBC_PATH = _os.path.join(_BASE_DIR, "Talent.dbc")

# MySQL connection (same as AzerothCore)
MYSQL_HOST = "127.0.0.1"
MYSQL_USER = "acore"
MYSQL_PASS = "acore"
MYSQL_DB = "acore_world"

# Output directory (relative to this script)
OUTPUT_DIR = "./output"
