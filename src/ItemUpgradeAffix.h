/*
 * Random-property re-tiering for upgraded items.  See ItemUpgradeAffix.cpp for
 * why this is a lookup table rather than a name match against core's data.
 */

#ifndef ALONECRAFT_ITEM_UPGRADE_AFFIX_H
#define ALONECRAFT_ITEM_UPGRADE_AFFIX_H

#include "Define.h"

namespace Alonecraft::ItemUpgrade
{
    // Reads alonecraft_random_affix and alonecraft_random_tier.  Safe to call
    // again; both maps are rebuilt from scratch.
    void LoadAffixTiers();

    // The tier of the same affix belonging to `newGroup`, or `randomPropId`
    // unchanged when there is nothing better to say.  Negative ids (random
    // suffixes) are returned untouched -- those already rescale themselves.
    int32 RetierRandomProperty(int32 randomPropId, int32 newGroup);
}

#endif // ALONECRAFT_ITEM_UPGRADE_AFFIX_H
