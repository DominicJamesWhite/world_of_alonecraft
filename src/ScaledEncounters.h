/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license:
 * https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE-AGPL3
 */

#ifndef SCALED_ENCOUNTERS_H
#define SCALED_ENCOUNTERS_H

#include "Creature.h"
#include "Map.h"
#include "Player.h"

// ---------------------------------------------------------------------------
//  Helpers shared by every ScaledEncounters_*.cpp file
// ---------------------------------------------------------------------------

/// Returns the number of non-GM players currently in the instance.
inline uint32 GetGroupSize(Map* map)
{
    return map ? map->GetPlayersCountExceptGMs() : 1;
}

/// Overload that takes a WorldObject (creature, GO, etc.).
inline uint32 GetGroupSize(WorldObject* obj)
{
    return obj ? GetGroupSize(obj->GetMap()) : 1;
}

// ---------------------------------------------------------------------------
//  Scaling formulae  (all return uint32, designed > 0)
// ---------------------------------------------------------------------------

/// Scale a count linearly: e.g. ScaleCount(4, 10, 2) = 1  (4 targets meant
/// for 10 players, with 2 present → 1 target, minimum 1).
inline uint32 ScaleCount(uint32 original, uint32 designed, uint32 N)
{
    if (N >= designed)
        return original;
    uint32 scaled = (original * N + designed - 1) / designed; // round up
    return std::max(1u, scaled);
}

/// Same as ScaleCount but allows the result to reach 0 (e.g. "no targets").
inline uint32 ScaleCountZero(uint32 original, uint32 designed, uint32 N)
{
    if (N >= designed)
        return original;
    return (original * N) / designed;
}

/// Scale damage that was meant to be soaked by `designed` players.
/// At fewer players the per-person hit is kept constant.
inline uint32 ScaleSplitDamage(uint32 totalDamage, uint32 designed, uint32 N)
{
    if (N >= designed)
        return totalDamage;
    return totalDamage * N / designed;
}

/// Returns a percentage (0–100) representing how close N is to designed.
inline uint32 ScalePct(uint32 N, uint32 designed)
{
    if (N >= designed)
        return 100;
    return 100 * N / designed;
}

/// Returns true when there are fewer than `threshold` players in the map.
inline bool IsSmallGroup(WorldObject* obj, uint32 threshold = 3)
{
    return GetGroupSize(obj) < threshold;
}

#endif // SCALED_ENCOUNTERS_H
