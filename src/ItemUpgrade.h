/*
 * Shared constants and helpers for the item upgrade system.
 *
 * Variant items are generated offline by tools/gen_item_variants.py.  The entry
 * ID encodes its own identity, so nothing needs a lookup table at runtime:
 *
 *     variantEntry = VARIANT_BASE + baseEntry * STRIDE + step
 *
 * Existence is tested with sObjectMgr->GetItemTemplate(), which is a direct
 * vector index (ObjectMgr.cpp:3933) and therefore free.
 *
 * IMPORTANT: steps are NOT contiguous.  The generator drops any step whose
 * stats are no better than the step before it -- these occur where the
 * native-equivalent cap binds across several consecutive levels, and offering
 * them would charge gold for an identical item.  Callers must therefore scan
 * forward for the next existing variant rather than assuming step + 1.
 */

#ifndef ALONECRAFT_ITEM_UPGRADE_H
#define ALONECRAFT_ITEM_UPGRADE_H

#include "Config.h"
#include "ItemTemplate.h"
#include "ObjectMgr.h"
#include "SharedDefines.h"

#include <cmath>
#include <utility>

namespace Alonecraft::ItemUpgrade
{
    constexpr uint32 STRIDE = 20;
    constexpr uint8  MAX_STEP = 16;   // level 1 -> 80 in 5-level steps

    inline uint32 GetEntryBase()
    {
        return sConfigMgr->GetOption<uint32>("Alonecraft.ItemUpgrade.EntryBase", 1000000);
    }

    inline bool IsEnabled()
    {
        return sConfigMgr->GetOption<bool>("Alonecraft.ItemUpgrade.Enable", true);
    }

    // entry -> (base entry, step).  A non-variant entry decodes to step 0, which
    // is what lets an un-upgraded item enter the same code path.
    inline std::pair<uint32, uint8> Decode(uint32 entry)
    {
        uint32 base = GetEntryBase();
        if (entry < base)
            return { entry, 0 };

        uint32 offset = entry - base;
        return { offset / STRIDE, static_cast<uint8>(offset % STRIDE) };
    }

    inline uint32 Encode(uint32 baseEntry, uint8 step)
    {
        return GetEntryBase() + baseEntry * STRIDE + step;
    }

    // The best variant of `baseEntry` the tool is allowed to reach: the one with
    // the HIGHEST RequiredLevel that is still <= targetLevel.
    //
    // Deliberately not an exact-level match.  Steps are not contiguous (see the
    // file header), so an item can simply have no level-60 variant -- Ring of
    // Saviors, for one, generates only 55, 70 and 80.  Under exact matching the
    // level-60 tool was a dead end on that item with no way to tell from the
    // vendor which tool would work, which is what made buying feel like a guess.
    // Taking the highest step at or below the target means every tool the player
    // is high enough to use does the most it legitimately can, and the ceiling
    // they paid for is still respected exactly.
    //
    // This has never been a sequential climb -- one tool has always jumped
    // straight to its level -- but the gaps made it look like one.
    //
    // Step index cannot be computed from the level directly: each item's chain
    // starts at the next multiple of 5 above its own level, so step 1 means
    // level 25 for one item and level 75 for another.  Scanning is cheap --
    // GetItemTemplate is a plain vector index, so this is at most 16 lookups.
    inline ItemTemplate const* FindVariantAtLevel(uint32 baseEntry, uint8 targetLevel,
                                                  uint32& outEntry)
    {
        ItemTemplate const* best = nullptr;
        for (uint8 step = 1; step <= MAX_STEP; ++step)
        {
            uint32 entry = Encode(baseEntry, step);
            ItemTemplate const* proto = sObjectMgr->GetItemTemplate(entry);
            if (!proto || proto->RequiredLevel > targetLevel)
                continue;
            if (!best || proto->RequiredLevel > best->RequiredLevel)
            {
                best = proto;
                outEntry = entry;
            }
        }
        return best;
    }

    // Cost rises super-linearly with target level, mirroring the roughly L^1.9
    // shape of the ScalingStatValues budget curve the stats themselves follow,
    // and is multiplied by rarity so upgrading an epic is not a bargain relative
    // to its power.  Everything is config-scaled because the right absolute
    // numbers depend entirely on the realm's economy.
    inline uint32 GetCost(uint8 targetLevel, uint32 quality)
    {
        static constexpr float qualityMult[] = { 1.0f, 1.0f, 1.0f, 2.0f, 5.4f, 8.0f };
        float mult = qualityMult[std::min<uint32>(quality, 5)];

        float scale = sConfigMgr->GetOption<float>("Alonecraft.ItemUpgrade.CostMultiplier", 1.0f);
        float base = sConfigMgr->GetOption<float>("Alonecraft.ItemUpgrade.BaseCost", 21.0f);

        float copper = base * mult * std::pow(static_cast<float>(targetLevel), 2.6f) * scale;

        // Round to whole silver so prices read cleanly in the confirm dialog.
        return static_cast<uint32>(std::lround(copper / 100.0f)) * 100;
    }
}

#endif // ALONECRAFT_ITEM_UPGRADE_H
