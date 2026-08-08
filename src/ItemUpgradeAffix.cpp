/*
 * Random-property re-tiering for upgraded items.
 *
 * A vanilla item's "of the Eagle" is an ItemRandomProperty: a FIXED enchantment
 * picked from a per-slot ladder, not a scaling suffix.  GenerateEnchSuffixFactor
 * (ItemEnchantmentMgr.cpp:131) returns 0 for anything without RandomSuffix, so
 * copying the property id onto the upgraded item -- which is all Restore() used
 * to do -- carries the level-20 bonus all the way to level 80.
 *
 * tools/gen_random_property_tiers.py extends each ladder past level 60, where
 * Blizzard's data stops entirely, and assigns every variant the group its target
 * level warrants.  All that is left at runtime is: given the old property and
 * the new item's group, find the tier of the SAME affix in that group.
 *
 * Why two tables rather than reading item_enchantment_template directly:
 * ItemEnchantmentMgr keeps its group map in a file-scope static with no
 * accessor, so matching by affix name at runtime would mean adding one to core.
 * The generator does the name matching offline instead and leaves two integer
 * lookups behind, which is both faster and a module-only change.
 */

#include "DatabaseEnv.h"
#include "ItemUpgradeAffix.h"
#include "Log.h"
#include "ScriptMgr.h"

#include <unordered_map>

namespace
{
    // property id -> affix id, and (affix id, group) -> property id.
    std::unordered_map<uint32, uint32> g_affixOf;
    std::unordered_map<uint64, uint32> g_tier;

    constexpr uint64 TierKey(uint32 affix, uint32 group)
    {
        return (uint64(affix) << 32) | group;
    }
}

namespace Alonecraft::ItemUpgrade
{
    void LoadAffixTiers()
    {
        g_affixOf.clear();
        g_tier.clear();

        if (QueryResult r = WorldDatabase.Query("SELECT property, affix FROM alonecraft_random_affix"))
            do
            {
                Field* f = r->Fetch();
                g_affixOf[f[0].Get<uint32>()] = f[1].Get<uint32>();
            } while (r->NextRow());

        if (QueryResult r = WorldDatabase.Query("SELECT affix, group_id, property FROM alonecraft_random_tier"))
            do
            {
                Field* f = r->Fetch();
                g_tier[TierKey(f[0].Get<uint32>(), f[1].Get<uint32>())] = f[2].Get<uint32>();
            } while (r->NextRow());

        LOG_INFO("server.loading", ">> Loaded {} affix mappings and {} tiers for item upgrades",
                 g_affixOf.size(), g_tier.size());
    }

    int32 RetierRandomProperty(int32 randomPropId, int32 newGroup)
    {
        // Suffixes (negative ids) already rescale off the new template's
        // ItemLevel inside SetItemRandomProperties -- they are the half of the
        // system that was never broken.  Leave them alone.
        // A RandomSuffix item carries its group in RandomSuffix instead, so
        // newGroup is 0 for those -- another reason they fall straight through.
        if (randomPropId <= 0 || newGroup <= 0)
            return randomPropId;

        auto affix = g_affixOf.find(uint32(randomPropId));
        if (affix == g_affixOf.end())
            return randomPropId;

        auto tier = g_tier.find(TierKey(affix->second, uint32(newGroup)));
        if (tier == g_tier.end())
            return randomPropId;   // no tier of this affix here; keep what we had

        return int32(tier->second);
    }
}

// OnStartup, not OnAfterConfigLoad: the world database is guaranteed loaded by
// then, and these two tables are generated data rather than configuration.
class woa_item_upgrade_affix_world : public WorldScript
{
public:
    woa_item_upgrade_affix_world() : WorldScript("woa_item_upgrade_affix_world") { }

    void OnStartup() override
    {
        Alonecraft::ItemUpgrade::LoadAffixTiers();
    }
};

void AddSC_item_upgrade_affix()
{
    new woa_item_upgrade_affix_world();
}
