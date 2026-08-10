/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license: https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE-AGPL3
 */

// ---------------------------------------------------------------------------
//  Hunter: Trap Launcher -- granting the launched trap spells
// ---------------------------------------------------------------------------
//
//  woa_2026_08_10_06.sql creates 19 castable "Trap Launcher: <Trap>" spells,
//  one per existing trap rank, each a 40 yard ground-targeted version of that
//  rank.  This file is the only C++ the feature needs: something has to put
//  them in the player's spellbook.
//
//  Why a script rather than data.  AzerothCore has no spell_learn_spell table
//  (Player.cpp:3294 mentions one, but only in a stale comment inherited from
//  TrinityCore), so "learning X also learns Y" is not expressible in SQL.
//  trainer_spell would work but requires a trainer visit and would not reach
//  the hunters who already know their traps.  A hook does both.
//
//  Two hooks, for two different moments:
//
//    OnPlayerLearnSpell  -- the trainer case.  The launched rank appears the
//                           instant the base rank is trained.
//    OnPlayerLogin       -- the backfill case.  Every existing hunter, and
//                           anyone who acquired a trap by some path that does
//                           not route through OnPlayerLearnSpell (.learn, a
//                           quest reward, a character copy).
//
//  Rank supersession is not handled here.  Player::addSpell walks spell_ranks,
//  and woa_2026_08_10_06.sql declares the launched chains there, so learning
//  rank 4 unlearns rank 3 by itself.  Doing it manually would fight the core.
// ---------------------------------------------------------------------------

#include "ScriptMgr.h"
#include "Player.h"

#include "AlonecraftTestLog.h"

namespace
{
    struct TrapLaunchEntry
    {
        uint32 baseSpell;       // the trap the hunter trains
        uint32 launchedSpell;   // its 40 yard counterpart
    };

    // Kept in the same order as woa_2026_08_10_06.sql allocates the ids, so
    // the two can be diffed by eye.
    constexpr TrapLaunchEntry TrapLaunchers[] =
    {
        // Freezing Trap
        { 1499,  200700 },
        { 14310, 200701 },
        { 14311, 200702 },
        // 27753 is a duplicate of Freezing Trap rank 3 that never made it
        // into spell_ranks.  It summons the same gameobject as 14311, so it
        // maps to the same launcher rather than getting one of its own.
        { 27753, 200702 },
        // Frost Trap (single rank)
        { 13809, 200703 },
        // Immolation Trap
        { 13795, 200704 },
        { 14302, 200705 },
        { 14303, 200706 },
        { 14304, 200707 },
        { 14305, 200708 },
        { 27023, 200709 },
        { 49055, 200710 },
        { 49056, 200711 },
        // Explosive Trap
        { 13813, 200712 },
        { 14316, 200713 },
        { 14317, 200714 },
        { 27025, 200715 },
        { 49066, 200716 },
        { 49067, 200717 },
        // Snake Trap (single rank)
        { 34600, 200718 },
    };

    // Grant the launcher for one freshly-known base trap.
    void GrantLauncherFor(Player* player, uint32 baseSpell)
    {
        for (TrapLaunchEntry const& entry : TrapLaunchers)
        {
            if (entry.baseSpell != baseSpell)
                continue;

            if (player->HasSpell(entry.launchedSpell))
                return;

            player->learnSpell(entry.launchedSpell);
            ACTEST("HUNT.TRAPLAUNCH", "granted base={} launched={} guid={}",
                   baseSpell, entry.launchedSpell, player->GetGUID().ToString());
            return;
        }
    }
}

class woa_hunter_trap_launcher : public PlayerScript
{
public:
    woa_hunter_trap_launcher() : PlayerScript("woa_hunter_trap_launcher",
        { PLAYERHOOK_ON_LOGIN, PLAYERHOOK_ON_LEARN_SPELL }) { }

    void OnPlayerLogin(Player* player) override
    {
        if (!player || player->getClass() != CLASS_HUNTER)
            return;

        for (TrapLaunchEntry const& entry : TrapLaunchers)
            if (player->HasSpell(entry.baseSpell))
                GrantLauncherFor(player, entry.baseSpell);
    }

    void OnPlayerLearnSpell(Player* player, uint32 spellID) override
    {
        if (!player || player->getClass() != CLASS_HUNTER)
            return;

        GrantLauncherFor(player, spellID);
    }
};

void AddSC_hunter_trap_launcher()
{
    new woa_hunter_trap_launcher();
}
