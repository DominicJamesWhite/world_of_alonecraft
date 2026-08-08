/*
 * Alonecraft's additions to the class trainer gossip menu.
 *
 * TWO FEATURES, ONE SCRIPT, AND THAT IS DELIBERATE.  ScriptMgr::OnGossipHello
 * runs every AllCreatureScript through IsValidBoolScript, which STOPS AT THE
 * FIRST ONE THAT RETURNS TRUE (ScriptMgrMacros.h:23-36).  Returning true is
 * also mandatory for anything that appends a gossip item, because the core menu
 * path in HandleGossipHelloOpcode is skipped once a script claims the hello.
 * So two independently registered scripts both adding a trainer item cannot
 * coexist: whichever the registry happens to visit first wins and the other
 * silently never appears, with the winner decided by script id.  Any future
 * trainer gossip option belongs in this file, appended in AppendItems, rather
 * than in a new AllCreatureScript.
 *
 * ---------------------------------------------------------------------------
 * 1. Purchase an Additional Talent Specialization
 *
 * Retail's dual-spec option is DB data (gossip_menu_option MenuID 0 / OptionID
 * 16, OptionType 18) and is hard-gated on GetSpecsCount() == 1 in both the
 * visibility filter (PlayerGossip.cpp:93) and the handler (PlayerGossip.cpp:339),
 * so it can only ever take a player from 1 spec to 2.  It keeps doing that job.
 * This appends a second option for every spec after the second, up to
 * MAX_TALENT_SPECS, gated exactly like dual spec.
 *
 * ---------------------------------------------------------------------------
 * 2. Toggle experience gain
 *
 * Alonecraft runs at 10x XP, which means a player who wants to actually play a
 * zone outgrows it in one quest hub.  This turns the 10x rate into a choice
 * rather than a mandate.  Free and freely reversible: it is a pacing tool, not
 * a challenge mode, so charging for it would only punish someone who changes
 * their mind halfway through a zone.
 *
 * The core already does all the work -- PLAYER_FLAGS_NO_XP_GAIN is checked at
 * the top of Player::GiveXP (Player.cpp:2369) and persists through
 * characters.playerFlags -- so this is purely a way to reach the flag without
 * walking to one of the two Behsten/Slahtz NPCs in the capitals.  Modelled on
 * core's npc_experience (npcs_special.cpp:1963).
 *
 * KNOWN TRADE-OFF: with XP off, a quest turn-in awards neither XP nor the
 * max-level gold conversion.  GiveXP returns before doing anything, and the
 * gold path in RewardQuest keys on player level rather than on the flag.
 * Core's own npc_experience behaves identically.  The confirmation text says so.
 *
 * ---------------------------------------------------------------------------
 * A new gossip_menu_option OptionType is not an option for either feature:
 * PrepareGossipMenu's switch rejects unknown types in its default: branch
 * (PlayerGossip.cpp:151-154), so DB-driven custom options are impossible.
 */

#include "Chat.h"
#include "Config.h"
#include "Creature.h"
#include "MultiSpec.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "ScriptedGossip.h"
#include "SharedDefines.h"
#include "StringFormat.h"
#include "Trainer.h"
#include "World.h"

#include <algorithm>

namespace
{
    // DB-built gossip items always carry sender = 0 (PlayerGossip.cpp:205), so
    // any non-zero sender unambiguously identifies one of ours.  The two
    // features must not share a sender, or the select handler cannot tell them
    // apart.
    constexpr uint32 EXTRA_SPEC_SENDER = GOSSIP_SENDER_MAIN + 900;
    constexpr uint32 EXTRA_SPEC_ACTION = GOSSIP_ACTION_INFO_DEF + 1;

    constexpr uint32 XP_TOGGLE_SENDER = GOSSIP_SENDER_MAIN + 901;
    constexpr uint32 XP_TOGGLE_ACTION_OFF = GOSSIP_ACTION_INFO_DEF + 1;
    constexpr uint32 XP_TOGGLE_ACTION_ON = GOSSIP_ACTION_INFO_DEF + 2;

    constexpr uint32 DEFAULT_EXTRA_SPEC_COST = 10000000; // 1000 gold, same as dual spec

    // ── Extra specialization ────────────────────────────────────────────────

    bool ExtraSpecEnabled()
    {
        return sConfigMgr->GetOption<bool>("Alonecraft.ExtraSpec.Enable", true);
    }

    uint32 GetExtraSpecCost()
    {
        return sConfigMgr->GetOption<uint32>("Alonecraft.ExtraSpec.Cost", DEFAULT_EXTRA_SPEC_COST);
    }

    uint8 GetMaxSpecs()
    {
        uint32 configured = sConfigMgr->GetOption<uint32>("Alonecraft.ExtraSpec.MaxSpecs", MAX_TALENT_SPECS);
        return static_cast<uint8>(std::min<uint32>(configured, MAX_TALENT_SPECS));
    }

    // Same conditions the core applies to the dual-spec option, except that the
    // player must already have two specs rather than one.
    bool IsExtraSpecEligible(Player* player, Creature* creature)
    {
        if (!ExtraSpecEnabled() || !player || !creature)
            return false;

        if (player->GetSpecsCount() < 2 || player->GetSpecsCount() >= GetMaxSpecs())
            return false;

        if (player->GetLevel() < sWorld->getIntConfig(CONFIG_MIN_DUALSPEC_LEVEL))
            return false;

        // Implies "class trainer for this player's class, level >= 10".
        return creature->CanResetTalents(player);
    }

    // ── XP toggle ───────────────────────────────────────────────────────────

    bool XpToggleEnabled()
    {
        return sConfigMgr->GetOption<bool>("Alonecraft.XpToggle.Enable", true);
    }

    uint32 GetXpToggleCost()
    {
        return sConfigMgr->GetOption<uint32>("Alonecraft.XpToggle.Cost", 0);
    }

    // Deliberately NOT creature->CanResetTalents(player), which the extra-spec
    // option uses: that implies level >= 10, and pacing an early zone is
    // precisely the case this feature exists for.  So the trainer check is done
    // directly, with no level floor.
    bool IsClassTrainerFor(Player* player, Creature* creature)
    {
        Trainer::Trainer const* trainer = sObjectMgr->GetTrainer(creature->GetEntry());
        return trainer
            && trainer->GetTrainerType() == Trainer::Type::Class
            && trainer->IsTrainerValidForPlayer(player);
    }

    bool IsXpToggleEligible(Player* player, Creature* creature)
    {
        if (!XpToggleEnabled() || !player || !creature)
            return false;

        // Nothing to toggle once there is no more XP to gain.  A character who
        // switched XP off keeps the flag; it just stops being offered here.
        if (player->GetLevel() >= sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL))
            return false;

        return IsClassTrainerFor(player, creature);
    }

    void AddExtraSpecItem(Player* player)
    {
        uint32 cost = GetExtraSpecCost();
        uint8 nextSpec = player->GetSpecsCount() + 1;

        AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG,
            "Purchase an Additional Talent Specialization",
            EXTRA_SPEC_SENDER, EXTRA_SPEC_ACTION,
            Acore::StringFormat("Are you sure you wish to purchase talent specialization {} for {} gold?",
                nextSpec, cost / GOLD),
            cost, false);
    }

    void AddXpToggleItem(Player* player)
    {
        uint32 cost = GetXpToggleCost();

        if (!player->HasPlayerFlag(PLAYER_FLAGS_NO_XP_GAIN))
        {
            AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1,
                "I no longer wish to gain experience.",
                XP_TOGGLE_SENDER, XP_TOGGLE_ACTION_OFF,
                "Stop gaining experience? Quests will award neither experience "
                "nor gold in its place while this is active. You can turn it "
                "back on here at any time.",
                cost, false);
        }
        else
        {
            AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1,
                "I wish to start gaining experience again.",
                XP_TOGGLE_SENDER, XP_TOGGLE_ACTION_ON,
                "Resume gaining experience?",
                cost, false);
        }
    }

    // The core only verifies and deducts BoxMoney inside Player::OnGossipSelect,
    // which never runs for a script-handled item, so every one of ours has to
    // charge by hand.  Returns false if the player cannot pay.
    bool ChargePlayer(Player* player, Creature* creature, uint32 cost)
    {
        if (!cost)
            return true;

        if (!player->HasEnoughMoney(static_cast<int32>(cost)))
        {
            player->SendBuyError(BUY_ERR_NOT_ENOUGHT_MONEY, creature, 0, 0);
            return false;
        }

        player->ModifyMoney(-static_cast<int32>(cost));
        return true;
    }
}

class trainer_gossip_creature_script : public AllCreatureScript
{
public:
    trainer_gossip_creature_script() : AllCreatureScript("trainer_gossip_creature_script") { }

    bool CanCreatureGossipHello(Player* player, Creature* creature) override
    {
        bool extraSpec = IsExtraSpecEligible(player, creature);
        bool xpToggle = IsXpToggleEligible(player, creature);

        if (!extraSpec && !xpToggle)
            return false; // let the core build the menu as usual

        // Reproduce what NPCHandler::HandleGossipHelloOpcode would have done.
        // PrepareGossipMenu clears the menu itself, so no ClearGossipMenuFor.
        player->PrepareGossipMenu(creature, creature->GetGossipMenuId(), true);

        if (extraSpec)
            AddExtraSpecItem(player);

        if (xpToggle)
            AddXpToggleItem(player);

        player->SendPreparedGossip(creature);
        return true;
    }

    bool CanCreatureGossipSelect(Player* player, Creature* creature, uint32 sender, uint32 action) override
    {
        if (sender == EXTRA_SPEC_SENDER)
            return HandleExtraSpec(player, creature, action);

        if (sender == XP_TOGGLE_SENDER)
            return HandleXpToggle(player, creature, action);

        return false; // not ours — core handles it normally
    }

private:
    static bool HandleExtraSpec(Player* player, Creature* creature, uint32 action)
    {
        if (action != EXTRA_SPEC_ACTION)
            return false;

        // The client can replay a stale menu, so re-check everything.
        if (!IsExtraSpecEligible(player, creature))
        {
            CloseGossipMenuFor(player);
            return true;
        }

        if (!ChargePlayer(player, creature, GetExtraSpecCost()))
        {
            CloseGossipMenuFor(player);
            return true;
        }

        uint8 newCount = player->GetSpecsCount() + 1;
        player->UpdateSpecCount(newCount);

        // UpdateSpecCount does not fire OnPlayerAfterSpecSlotChanged, so tell
        // the MultiSpec addon about the new tab directly.
        Alonecraft::SendSpecState(player);

        ChatHandler(player->GetSession()).PSendSysMessage(
            "You have purchased talent specialization {}. Use /ms {} or .spec {} to switch to it.",
            newCount, newCount, newCount);

        CloseGossipMenuFor(player);
        return true;
    }

    static bool HandleXpToggle(Player* player, Creature* creature, uint32 action)
    {
        if (action != XP_TOGGLE_ACTION_OFF && action != XP_TOGGLE_ACTION_ON)
            return false;

        if (!IsXpToggleEligible(player, creature))
        {
            CloseGossipMenuFor(player);
            return true;
        }

        // Guard against a stale menu whose action disagrees with the flag --
        // otherwise a replayed "turn off" on an already-off character would
        // charge again for nothing.
        bool wantsOff = (action == XP_TOGGLE_ACTION_OFF);
        if (wantsOff == player->HasPlayerFlag(PLAYER_FLAGS_NO_XP_GAIN))
        {
            CloseGossipMenuFor(player);
            return true;
        }

        if (!ChargePlayer(player, creature, GetXpToggleCost()))
        {
            CloseGossipMenuFor(player);
            return true;
        }

        if (wantsOff)
        {
            player->SetPlayerFlag(PLAYER_FLAGS_NO_XP_GAIN);
            ChatHandler(player->GetSession()).PSendSysMessage(
                "You will no longer gain experience. Return to any class trainer to resume.");
        }
        else
        {
            player->RemovePlayerFlag(PLAYER_FLAGS_NO_XP_GAIN);
            ChatHandler(player->GetSession()).PSendSysMessage(
                "You will now gain experience again.");
        }

        CloseGossipMenuFor(player);
        return true;
    }
};

void AddSC_trainer_gossip()
{
    new trainer_gossip_creature_script();
}
