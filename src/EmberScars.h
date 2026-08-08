/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license:
 * https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE-AGPL3
 */

#ifndef ALONECRAFT_EMBER_SCARS_H
#define ALONECRAFT_EMBER_SCARS_H

#include "Player.h"

// ---------------------------------------------------------------------------
//  Ember Scars helpers shared by MoltenArmor.cpp and Firebreak.cpp
// ---------------------------------------------------------------------------
//
//  Ember Scars (200023) is the Fire Mage damage-stagger DoT: while Molten
//  Armor is up, half of every hit taken is deferred into it and bled back over
//  10 seconds.  Spell crits clear a stack, and so does Fire Blast via
//  Firebreak.
//
//  Stacks are a counter; the deferred damage itself lives entirely in
//  EFFECT_0's amount.  Clearing a stack therefore has to remove that stack's
//  share of the pool by hand -- see RemoveEmberScarsStacks.
//
//  This lived as two near-identical copies, a file-static in MoltenArmor.cpp
//  and a global in Firebreak.cpp, which is how they drifted: only the
//  MoltenArmor copy honoured Cleansing Flame, and a later fix to the share
//  maths would have had to be made twice.
//
//  NAMESPACE: this MUST stay namespaced, for the same reason as
//  WarlockShards.h.  Firebreak.cpp's copy was a global
//  `void RemoveEmberScarsStack(Player*, uint8)`, and module globals with
//  plausible names risk LNK2005 against mod-playerbots and friends.
// ---------------------------------------------------------------------------

namespace Alonecraft::Mage
{
    // The stacking deferred-damage DoT.
    constexpr uint32 EMBER_SCARS_DOT_ID = 200023;

    // Must match spell 200023's StackAmount.  Used only as the cap when
    // adding stacks -- the removal maths deliberately does not depend on it.
    constexpr uint8 EMBER_SCARS_MAX_STACKS = 5;

    /*
     * Clear stacks, and with them their share of the outstanding damage.
     *
     * Every stack holds an equal share of the CURRENT pool, so clearing k of n
     * stacks leaves exactly (n - k) / n of the damage behind: 1 of 5 removes
     * 20%, 1 of 2 removes 50%, and clearing every stack removes all of it.
     *
     * Dividing by the live stack count rather than by EMBER_SCARS_MAX_STACKS
     * is what makes that hold.  A fixed 1/5 share only lines up while the aura
     * is at full stacks; below that it under-removes, and then the last clear
     * silently wipes whatever is left -- at 2 stacks the first crit would take
     * 20% and the second the remaining 80%.
     *
     * The share is of the pool, not of the hits that filled it.  Two big hits
     * and five small ones can hold the same damage, and in both cases one
     * stack is worth one stack's share of it.
     */
    void RemoveEmberScarsStacks(Player* player, uint8 stacksToRemove = 1);
}

#endif // ALONECRAFT_EMBER_SCARS_H
