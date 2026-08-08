# Path to the base DBC files — includes all pre-tooling manual changes (4.45).
# These live in base/ next to this script and are checked into git.
# IMPORTANT: Do NOT point at the build output directory (C:/Build/bin/...),
# or stale changes will accumulate across rebuilds.
import os as _os
_BASE_DIR = _os.path.join(_os.path.dirname(_os.path.abspath(__file__)), "base")
BASE_DBC_PATH = _os.path.join(_BASE_DIR, "Spell.dbc")
BASE_TALENT_DBC_PATH = _os.path.join(_BASE_DIR, "Talent.dbc")
BASE_SHAPESHIFT_DBC_PATH = _os.path.join(_BASE_DIR, "SpellShapeshiftForm.dbc")
BASE_SPELLVISUAL_DBC_PATH = _os.path.join(_BASE_DIR, "SpellVisual.dbc")
# Item.dbc is patched from the `item_dbc` MySQL table -- the same table the
# server reads (DBCStores.cpp:338), so one source feeds both sides.  The client
# needs it too: the bag icon is resolved client-side via
# Item.dbc -> DisplayInfoID -> ItemDisplayInfo.dbc -> InventoryIcon, so an item
# the client's Item.dbc has never heard of renders as a red '?' in bags even
# though it displays correctly on the character (that path uses the displayid
# from the item query response instead).
BASE_ITEM_DBC_PATH = _os.path.join(_BASE_DIR, "Item.dbc")
# The far end of that chain.  Custom items need a display row of their own when
# no retail item already wears the icon we want -- the upgrade tools point at
# recoloured enchanting-reagent icons that exist only in patch-4.mpq.  Extracted
# from enUS/patch-enUS-3.MPQ, not locale-enUS.MPQ: the locale archive still
# carries the launch-day table (47829 rows) and the patch chain replaces it
# wholesale (57986 rows, max id 68742).
BASE_ITEMDISPLAYINFO_DBC_PATH = _os.path.join(_BASE_DIR, "ItemDisplayInfo.dbc")
# Synthetic "unique-equipped" families for the item upgrade system.  Uniqueness
# is enforced by entry ID (Player.cpp:13978), so a variant of a unique trinket
# would otherwise be equippable alongside its base -- two of the same trinket.
# A shared ItemLimitCategory covers the whole family.  The client is sent the
# category id (ItemHandler.cpp:535) and renders its name, so it needs this file.
BASE_ITEMLIMITCATEGORY_DBC_PATH = _os.path.join(_BASE_DIR, "ItemLimitCategory.dbc")
BASE_ITEMRANDOMPROPERTIES_DBC_PATH = _os.path.join(_BASE_DIR, "ItemRandomProperties.dbc")
BASE_SPELLITEMENCHANTMENT_DBC_PATH = _os.path.join(_BASE_DIR, "SpellItemEnchantment.dbc")

# Index DBCs used by the talent-calculator export (tools/export_talents.py).
# build_dbc.py never writes these -- an override changes a spell's
# DurationIndex, never the duration table itself -- so one copy is correct for
# both the base and the live source.
BASE_SPELLICON_DBC_PATH = _os.path.join(_BASE_DIR, "SpellIcon.dbc")
BASE_SPELLDURATION_DBC_PATH = _os.path.join(_BASE_DIR, "SpellDuration.dbc")
BASE_SPELLRADIUS_DBC_PATH = _os.path.join(_BASE_DIR, "SpellRadius.dbc")
BASE_SPELLRANGE_DBC_PATH = _os.path.join(_BASE_DIR, "SpellRange.dbc")
BASE_SPELLCASTTIMES_DBC_PATH = _os.path.join(_BASE_DIR, "SpellCastTimes.dbc")

# Pristine retail 3.3.5a DBCs, extracted from the client's enUS locale MPQ
# chain.  BASE_DBC_PATH above already contains pre-tooling manual edits, so it
# under-reports what Alonecraft actually changed; these are the honest
# baseline for the calculator's "modified" badge.
_RETAIL_DIR = _os.path.join(_BASE_DIR, "retail")
RETAIL_DBC_PATH = _os.path.join(_RETAIL_DIR, "Spell.dbc")
RETAIL_TALENT_DBC_PATH = _os.path.join(_RETAIL_DIR, "Talent.dbc")

# MySQL connection (same as AzerothCore)
MYSQL_HOST = "127.0.0.1"
MYSQL_USER = "acore"
MYSQL_PASS = "acore"
MYSQL_DB = "acore_world"

# Output directory (relative to this script)
OUTPUT_DIR = "./output"
BASE_TALENTTAB_DBC_PATH = _os.path.join(_BASE_DIR, "TalentTab.dbc")
