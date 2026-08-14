/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license:
 * https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE-AGPL3
 */

#ifndef ALONECRAFT_PET_TALENT_HELPERS_H
#define ALONECRAFT_PET_TALENT_HELPERS_H

#include "Player.h"
#include "SpellAuraEffects.h"
#include "Unit.h"

#include <initializer_list>

// ---------------------------------------------------------------------------
//  Shared plumbing for "talent on the owner, aura on the pet" designs
// ---------------------------------------------------------------------------
//
//  Why these auras have to sit on the pet at all: Unit::ProcSkillsAndReactives
//  (Unit.cpp:6789) only runs for the actor and the victim of an event, so an
//  event caused by the pet never reaches the owner's aura list.  Any "when your
//  pet does X" talent therefore needs its own aura on the pet.
//
//  Delivery is `spell_pet_auras`:
//      talent dummy effect -> AuraEffect::HandleAuraDummy
//                             (SpellAuraEffects.cpp:5171)
//                          -> Unit::AddPetAura   (Unit.cpp:13753)
//                          -> Pet::CastPetAuras  (Pet.cpp:2354)
//  which re-applies on summon and on talent (re)application, so respecs and
//  re-summons need no bookkeeping.
//
//  IMPORTANT: Unit::CastPetAura (Unit.cpp:13770) special-cases ONLY spell 35696
//  for custom base points -- every other pet aura is a bare
//  CastSpell(this, auraId, true).  So no pet aura can be handed an amount at
//  cast time; amounts are computed in DoEffectCalcAmount by reading the owner's
//  talent rank, which is what OwnerTalentAmount below is for.
//
//  This is a deliberate copy of the file-local helpers in
//  WarlockDemonPets.cpp:107-150 rather than a move.  Those statics still work,
//  and lifting them would drag the Demonology pass into an unrelated diff; the
//  Hunter work gets its own copy and the two can be merged later if a third
//  caller appears.

namespace Alonecraft::Pets
{
    // The player that owns the unit this aura is sitting on, or nullptr.
    inline Player* GetPetOwner(Unit* pet)
    {
        if (!pet)
            return nullptr;

        Unit* owner = pet->GetOwner();
        return owner ? owner->ToPlayer() : nullptr;
    }

    // Amount stored on `effIndex` of the highest talent rank the owner actually
    // has.  Ranks are listed low-to-high and walked backwards so the best one
    // wins even if a lower rank's aura somehow lingers.
    inline int32 OwnerTalentAmount(Unit* pet, std::initializer_list<uint32> ranks, uint8 effIndex)
    {
        Player* owner = GetPetOwner(pet);
        if (!owner)
            return 0;

        for (auto itr = std::rbegin(ranks); itr != std::rend(ranks); ++itr)
            if (AuraEffect const* eff = owner->GetAuraEffect(*itr, effIndex))
                return eff->GetAmount();

        return 0;
    }
}

// ---------------------------------------------------------------------------
//  Shared 2-second recalculation heartbeat
// ---------------------------------------------------------------------------
//  Mirrors spell_warl_generic_scaling::CalcPeriodic / HandlePeriodic
//  (spell_warlock.cpp:376-407).  Attaching this to an aura effect that is not
//  periodic in the DBC is legal -- AuraEffect::CalculatePeriodic asks the script
//  -- and saves an effect slot on every custom pet spell.
//
//  Stays a macro rather than a base class: tools/verify_scripts.py scans for
//  each loader's script-name string literal, and a template base would move the
//  PrepareAuraScript expansion out of the file it belongs to.
#define ALONECRAFT_PET_HEARTBEAT(scriptClass)                                       \
    void CalcPeriodic(AuraEffect const* /*aurEff*/, bool& isPeriodic, int32& amplitude) \
    {                                                                               \
        isPeriodic = true;                                                          \
        amplitude  = 2 * IN_MILLISECONDS;                                           \
    }                                                                               \
                                                                                    \
    void HandlePeriodic(AuraEffect const* aurEff)                                   \
    {                                                                               \
        PreventDefaultAction();                                                     \
        GetEffect(aurEff->GetEffIndex())->RecalculateAmount();                       \
    }

#endif // ALONECRAFT_PET_TALENT_HELPERS_H
