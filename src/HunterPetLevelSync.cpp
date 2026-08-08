/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license:
 * https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE-AGPL3
 */

#include "Pet.h"
#include "PetDefines.h"
#include "Player.h"
#include "ScriptMgr.h"

// ---------------------------------------------------------------------------
//  Hunter pets are always the owner's level
// ---------------------------------------------------------------------------
//
//  Alonecraft puts its 10x on quests and nothing else, which leaves pet XP in a
//  bad place: it only ever comes from kills, so a hunter levelling on quests
//  would watch the pet fall further behind every zone.  Rather than give pets
//  their own multiplier to keep a second XP bar in step with the first, the
//  second bar is retired -- the pet is simply held at the owner's level.
//
//  What the core already does:
//      SUMMON_PET   Pet.cpp:2392  GivePetLevel(owner->GetLevel())  -- exact
//      HUNTER_PET   Pet.cpp:2396  clamped into [owner - 5, owner]  -- not exact
//  so only HUNTER_PET needs anything here.
//
//  Two hooks, because no single one covers every way a pet's level is set:
//
//    OnPetAddToWorld (Pet.cpp:118) covers summon-from-stable, login, and -- the
//    one Pet::SynchronizeLevelWithOwner misses entirely -- a fresh tame, which
//    sets the level straight from the tamed creature at SpellEffects.cpp:3167
//    and never syncs.
//
//    OnPlayerLevelChanged (Player.cpp:2545) covers the owner levelling up.  It
//    fires *after* the core's SynchronizeLevelWithOwner at Player.cpp:2519, so
//    it re-applies on top of the owner-5 clamp rather than fighting it.
//
//  Pet::GivePetXP is left alone: it returns at Pet.cpp:903 once the pet is at
//  the owner's level, so it becomes dead code on its own.  Rate.XP.Pet = 0 in
//  worldserver.overrides.conf makes that belt-and-braces (Pet.cpp:891-894).
namespace
{
    // Pet::GivePetLevel returns early when the level already matches, so the
    // stale experience of a pet that levelled under the old rules would sit on
    // the bar forever.  Zero it here rather than there.
    void SyncPetToOwnerLevel(Pet* pet, Player* owner)
    {
        if (!pet || !owner || pet->getPetType() != HUNTER_PET)
            return;

        uint8 const ownerLevel = owner->GetLevel();

        if (pet->GetLevel() != ownerLevel)
        {
            pet->GivePetLevel(ownerLevel);

            // No-op while the pet is still loading (Pet.cpp:510 guards on
            // m_loading), which is exactly what we want -- that path re-syncs
            // on every load anyway.  It matters for a fresh tame.
            pet->SavePetToDB(PET_SAVE_AS_CURRENT);
        }

        pet->SetUInt32Value(UNIT_FIELD_PETEXPERIENCE, 0);
    }
}

class woa_hunter_pet_level_sync_pet : public PetScript
{
public:
    woa_hunter_pet_level_sync_pet()
        : PetScript("woa_hunter_pet_level_sync_pet", { PETHOOK_ON_PET_ADD_TO_WORLD }) { }

    void OnPetAddToWorld(Pet* pet) override
    {
        if (!pet)
            return;

        SyncPetToOwnerLevel(pet, pet->GetOwner());
    }
};

class woa_hunter_pet_level_sync_player : public PlayerScript
{
public:
    woa_hunter_pet_level_sync_player()
        : PlayerScript("woa_hunter_pet_level_sync_player", { PLAYERHOOK_ON_LEVEL_CHANGED }) { }

    void OnPlayerLevelChanged(Player* player, uint8 /*oldLevel*/) override
    {
        if (!player)
            return;

        SyncPetToOwnerLevel(player->GetPet(), player);
    }
};

void AddSC_hunter_pet_level_sync()
{
    new woa_hunter_pet_level_sync_pet();
    new woa_hunter_pet_level_sync_player();
}
