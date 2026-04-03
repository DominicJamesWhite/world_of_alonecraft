/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license:
 * https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE-AGPL3
 *
 * Global encounter-scaling hooks that apply across all instances:
 *   G1 — Mind-Control / Charm / Possess scaling
 *         At N players, at most N-1 may be MC'd simultaneously.
 *         At N=1 every MC is cancelled instantly.
 */

#include "ScaledEncounters.h"
#include "ScriptMgr.h"
#include "SpellAuraDefines.h"
#include "SpellAuraEffects.h"
#include "SpellInfo.h"
#include "Map.h"

// ---------------------------------------------------------------------------
//  G1 — Mind-Control / Charm / Possess aura scaling
// ---------------------------------------------------------------------------

/// Returns true if this aura type is a hostile mind-control / charm / possess.
static bool IsMindControlAura(Aura const* aura)
{
    SpellInfo const* info = aura->GetSpellInfo();
    if (!info)
        return false;

    for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
    {
        switch (info->Effects[i].ApplyAuraName)
        {
            case SPELL_AURA_MOD_POSSESS:   // 2
            case SPELL_AURA_MOD_CHARM:     // 6
            case SPELL_AURA_AOE_CHARM:     // 177
                return true;
            default:
                break;
        }
    }
    return false;
}

/// Counts how many *players* in the map are currently charmed / MC'd.
static uint32 CountMCdPlayers(Map* map)
{
    uint32 count = 0;
    Map::PlayerList const& players = map->GetPlayers();
    for (auto const& ref : players)
    {
        Player* p = ref.GetSource();
        if (p && !p->IsGameMaster() && p->IsCharmed())
            ++count;
    }
    return count;
}

class ScaledEncounters_MCGuard : public UnitScript
{
public:
    ScaledEncounters_MCGuard()
        : UnitScript("ScaledEncounters_MCGuard",
                      /*addToScripts=*/true,
                      { UNITHOOK_ON_AURA_APPLY })
    { }

    void OnAuraApply(Unit* target, Aura* aura) override
    {
        // Only care about hostile MC applied to players inside an instance.
        if (!target || !target->IsPlayer())
            return;

        if (!IsMindControlAura(aura))
            return;

        Map* map = target->GetMap();
        if (!map || !map->IsDungeon())
            return;

        uint32 N = GetGroupSize(map);
        // At most N-1 players may be MC'd.  At N=1 that means zero.
        uint32 maxMC = (N > 0) ? N - 1 : 0;

        // CountMCdPlayers already includes *this* player (aura was just applied).
        if (CountMCdPlayers(map) > maxMC)
        {
            // Remove the aura that was just applied.
            aura->Remove();
        }
    }
};

// ---------------------------------------------------------------------------
//  Registration
// ---------------------------------------------------------------------------

void AddSC_scaled_encounters_global()
{
    new ScaledEncounters_MCGuard();
}
