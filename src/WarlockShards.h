/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license:
 * https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE-AGPL3
 */

#ifndef WARLOCK_SHARDS_H
#define WARLOCK_SHARDS_H

#include "AlonecraftTestLog.h"
#include "Player.h"
#include <algorithm>

// ---------------------------------------------------------------------------
//  Soul Shard helpers shared by every Warlock*.cpp file
// ---------------------------------------------------------------------------
//
//  Soul Shards remain item 6265.  Alonecraft only raises
//  item_template.stackable from 1 to 32 (see woa_2026_08_01_00.sql) so a full
//  hoard occupies one bag slot instead of thirty-two.
//
//  item_template.maxcount was already 32, so the core refuses to store shard
//  #33 on every creation path via Player::CanStoreNewItem.  The cap is
//  therefore free -- but Player::AddItem chat-spams "You don't have any space
//  in your bags." when storage fails, so every add goes through
//  AddSoulShards() which checks the count first.
//
//  Keeping the item (rather than an aura counter) means every existing
//  shard-consuming spell keeps working untouched through its Spell.dbc
//  Reagent1 = 6265 field -- Soul Fire, Ritual of Souls, Healthstones,
//  Enslave Demon, demon summons, and so on.
//
//  NAMESPACE: these MUST stay namespaced.  mod-playerbots defines its own
//  global `uint32 GetSoulShardCount(Player*)` in
//  modules/mod-playerbots/src/Ai/Class/Warlock/WarlockTriggers.cpp, and at
//  global scope the two collide -- worldserver fails to link with
//  LNK2005 / LNK1169 (multiply defined symbols).
// ---------------------------------------------------------------------------

namespace Alonecraft::Warlock
{

constexpr uint32 SOUL_SHARD_ITEM = 6265;
constexpr uint32 SOUL_SHARD_MAX  = 32;

/// Number of Soul Shards the player is carrying (bags only, not bank).
inline uint32 GetSoulShardCount(Player* player)
{
    return player ? player->GetItemCount(SOUL_SHARD_ITEM) : 0;
}

/// Grants up to `count` Soul Shards, never exceeding SOUL_SHARD_MAX.
/// Returns the number actually granted (0 if already capped or out of space).
///
/// `reason` is a test-harness tag identifying the caller; it is the single
/// choke point every shard gain in the module passes through, so the trace
/// answers "where did that shard come from" without instrumenting each talent.
inline uint32 AddSoulShards(Player* player, uint32 count = 1, char const* reason = "?")
{
    if (!player || !count)
        return 0;

    uint32 const have = GetSoulShardCount(player);
    if (have >= SOUL_SHARD_MAX)
    {
        ACTEST("WARL.SHARD", "gain BLOCKED reason={} player={} have={} cap={}",
            reason, player->GetName(), have, SOUL_SHARD_MAX);
        return 0;
    }

    count = std::min(count, SOUL_SHARD_MAX - have);
    uint32 const granted = player->AddItem(SOUL_SHARD_ITEM, count) ? count : 0;

    ACTEST("WARL.SHARD", "gain reason={} player={} asked={} granted={} before={} after={}",
        reason, player->GetName(), count, granted, have, GetSoulShardCount(player));

    return granted;
}

/// Destroys up to `count` Soul Shards. Returns the number actually consumed.
inline uint32 ConsumeSoulShards(Player* player, uint32 count, char const* reason = "?")
{
    if (!player || !count)
        return 0;

    uint32 const have = GetSoulShardCount(player);
    count = std::min(count, have);
    if (!count)
    {
        ACTEST("WARL.SHARD", "spend FAILED reason={} player={} have=0", reason, player->GetName());
        return 0;
    }

    player->DestroyItemCount(SOUL_SHARD_ITEM, count, true);

    ACTEST("WARL.SHARD", "spend reason={} player={} count={} before={} after={}",
        reason, player->GetName(), count, have, GetSoulShardCount(player));

    return count;
}

} // namespace Alonecraft::Warlock

#endif // WARLOCK_SHARDS_H
