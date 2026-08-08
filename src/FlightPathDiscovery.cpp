/*
 * Talking to a flight master reveals the nodes directly connected to it.
 *
 * Alonecraft runs at 10x XP, so whole zones get skipped -- which means their
 * flight masters are never visited and the taxi network ends up full of holes
 * caused precisely by levelling efficiently.  Rather than granting every node
 * outright, a conversation reveals the *one-hop* neighbours of the node you are
 * standing at, like reading the departure board at a train station.  Reaching a
 * distant continent still takes several hops and the map still fills in
 * gradually; it just stops being unusable because of skipped content.
 *
 * The one-hop set is exactly sTaxiPathSetBySource[node] (DBCStores.cpp:564),
 * keyed source -> destination.  The graph is directed, so `from == node` is the
 * correct set to walk.
 *
 * TWO HOOKS, because neither is sufficient alone:
 *
 *   1. PlayerScript::OnPlayerLearnTaxiNode fires from TaxiHandler.cpp:145 on
 *      genuine first discovery, in any configuration -- but ONLY when the bit
 *      was newly set.  It can never fire for a node the player already knows,
 *      which is every race's starting capital.
 *
 *   2. AllCreatureScript::CanCreatureGossipHello covers that gap: it fires
 *      every time the player opens a flight master, known or not.  It returns
 *      FALSE -- this is a side-effect-only observer.  Returning true would make
 *      ScriptMgr::OnGossipHello skip the core menu entirely and stop every
 *      other AllCreatureScript from running (IsValidBoolScript halts on the
 *      first true).
 *
 * Hook 2 depends on InstantFlightPaths = 2, which makes Unit::PatchValuesUpdate
 * (Unit.cpp:16953) force UNIT_NPC_FLAG_GOSSIP onto every flight master so the
 * instant-flight toggle is reachable.  That flag is what makes the client send
 * CMSG_GOSSIP_HELLO at all.  If that config ever drops to 0 or 1, hook 2 goes
 * silent and only first-discovery propagation survives -- the feature degrades
 * rather than breaking, but the backfill is gone.
 *
 * Deliberately NOT implemented by calling SendDiscoverNewTaxiNode per
 * neighbour: that re-fires OnPlayerLearnTaxiNode, which would turn this into an
 * unbounded breadth-first search across the entire taxi graph on first contact.
 * Bits are set directly instead, which is also why no recursion guard is
 * needed.
 */

#include "Config.h"
#include "Creature.h"
#include "DBCStores.h"
#include "ObjectMgr.h"
#include "Opcodes.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SharedDefines.h"
#include "WorldPacket.h"
#include "WorldSession.h"

namespace
{
    // DBCStores.cpp:631-638 treats this mount id as "Death Knight node", usable
    // by either team, rather than as a real faction mount.
    constexpr uint32 DK_MOUNT_SENTINEL = 32981;

    bool IsEnabled()
    {
        return sConfigMgr->GetOption<bool>("Alonecraft.FlightPathDiscovery.Enable", true);
    }

    // PlayerTaxi's accessors index a fixed 14-word array with no bounds check,
    // so validate the index before touching either mask.  This also enforces
    // membership of sTaxiNodesMask: LoadTaxiMask ANDs the saved mask against it
    // (PlayerTaxi.cpp:99), so a bit set outside it silently vanishes on the
    // next login and the player would "lose" a node they had just been shown.
    bool IsNodeIndexUsable(uint32 nodeId)
    {
        if (!nodeId)
            return false;

        std::size_t field = (nodeId - 1) / 32;
        if (field >= TaxiMaskSize)
            return false;

        uint32 submask = 1 << ((nodeId - 1) % 32);
        return (sTaxiNodesMask[field] & submask) == submask;
    }

    // Mirrors the team test in ObjectMgr::GetNearestTaxiNode (ObjectMgr.cpp:7273).
    // Nothing else filters this: cross-faction bits are not rejected on
    // discovery, they just produce map dots that fail at flight activation.
    bool IsNodeUsableByTeam(TaxiNodesEntry const* node, TeamId teamId)
    {
        return node->MountCreatureID[teamId == TEAM_ALLIANCE ? 1 : 0] != 0
            || node->MountCreatureID[0] == DK_MOUNT_SENTINEL;
    }

    void RevealNeighbours(Player* player, uint32 sourceNode)
    {
        if (!player || !IsEnabled())
            return;

        TaxiPathSetBySource::const_iterator paths = sTaxiPathSetBySource.find(sourceNode);
        if (paths == sTaxiPathSetBySource.end())
            return;

        TeamId teamId = player->GetTeamId(true);
        bool learnedAny = false;

        for (auto const& [destNode, pathEntry] : paths->second)
        {
            if (!pathEntry || !IsNodeIndexUsable(destNode))
                continue;

            if (player->m_taxi.IsTaximaskNodeKnown(destNode))
                continue;

            TaxiNodesEntry const* node = sTaxiNodesStore.LookupEntry(destNode);
            if (!node || !IsNodeUsableByTeam(node, teamId))
                continue;

            if (player->m_taxi.SetTaximaskNode(destNode))
                learnedAny = true;
        }

        if (!learnedAny)
            return;

        // One packet for the whole batch.  SMSG_NEW_TAXI_PATH carries no node
        // id -- it is purely the client's cue to play the discovery sound and
        // flash the UI -- so one per node would stack N sounds for one
        // conversation.  The taxi window itself rebuilds its mask from scratch
        // on open (SendTaxiMenu -> AppendTaximaskTo), so nothing else needs
        // refreshing, and the mask is persisted wholesale by SaveToDB.
        WorldPacket msg(SMSG_NEW_TAXI_PATH, 0);
        player->GetSession()->SendPacket(&msg);
    }
}

// Hook 1: first discovery of a node, in any configuration.
class flight_path_discovery_playerscript : public PlayerScript
{
public:
    flight_path_discovery_playerscript()
        : PlayerScript("flight_path_discovery_playerscript", { PLAYERHOOK_ON_LEARN_TAXI_NODE }) { }

    void OnPlayerLearnTaxiNode(Player const* player, uint32 nodeId) override
    {
        // m_taxi is public and this is the documented shape of the hook; the
        // const is on the pointer, not on the intent.
        RevealNeighbours(const_cast<Player*>(player), nodeId);
    }
};

// Hook 2: backfill for nodes the player already knew.
class flight_path_discovery_creaturescript : public AllCreatureScript
{
public:
    flight_path_discovery_creaturescript() : AllCreatureScript("flight_path_discovery_creaturescript") { }

    bool CanCreatureGossipHello(Player* player, Creature* creature) override
    {
        if (!player || !creature || !creature->HasNpcFlag(UNIT_NPC_FLAG_FLIGHTMASTER))
            return false;

        uint32 curloc = sObjectMgr->GetNearestTaxiNode(*creature, player->GetTeamId(true));

        // If the current node is not yet known, the core is about to learn it
        // in HandleTaxiQueryAvailableNodes and hook 1 will do this work with
        // the correct node.  Acting here too would send a second discovery
        // sound for the same conversation.
        if (IsNodeIndexUsable(curloc) && player->m_taxi.IsTaximaskNodeKnown(curloc))
            RevealNeighbours(player, curloc);

        // Always false: observer only.  See the header comment.
        return false;
    }
};

void AddSC_flight_path_discovery()
{
    new flight_path_discovery_playerscript();
    new flight_path_discovery_creaturescript();
}
