/*
 * .spec command — switch between talent specs and manage spec count.
 *
 * Usage:
 *   .spec <1-8>         Switch to the given spec (player-accessible)
 *   .spec preview <1-8> Preview a spec's talents in the talent window
 *   .spec unlock [N]    Set spec count to N, default current+1 (GM only)
 *   .spec count         Show current spec count and active spec
 *   .spec query         Send machine-readable spec state (for addon)
 */

#include "Chat.h"
#include "CommandScript.h"
#include "Player.h"
#include "ScriptMgr.h"

using namespace Acore::ChatCommands;

// Send a machine-readable system message the addon can parse:
// "MULTISPEC:active:count:preview" e.g. "MULTISPEC:3:8:5"
static void SendSpecState(Player* player, ChatHandler* handler)
{
    handler->PSendSysMessage("MULTISPEC:{}:{}:{}",
        player->GetActiveSpec() + 1,
        player->GetSpecsCount(),
        player->GetClientPreviewSpec() + 1);
}

class spec_commandscript : public CommandScript
{
public:
    spec_commandscript() : CommandScript("spec_commandscript") { }

    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable specCommandTable =
        {
            { "preview", HandleSpecPreviewCommand, SEC_PLAYER,     Console::No },
            { "unlock",  HandleSpecUnlockCommand,  SEC_GAMEMASTER, Console::No },
            { "count",   HandleSpecCountCommand,   SEC_PLAYER,     Console::No },
            { "query",   HandleSpecQueryCommand,   SEC_PLAYER,     Console::No },
            { "",        HandleSpecSwitchCommand,   SEC_PLAYER,     Console::No },
        };

        static ChatCommandTable commandTable =
        {
            { "spec", specCommandTable },
        };

        return commandTable;
    }

    // .spec <1-8>
    static bool HandleSpecSwitchCommand(ChatHandler* handler, uint8 specNum)
    {
        Player* player = handler->GetSession()->GetPlayer();
        if (!player)
            return false;

        if (specNum < 1 || specNum > MAX_TALENT_SPECS)
        {
            handler->PSendSysMessage("Spec must be between 1 and {}.", MAX_TALENT_SPECS);
            return true;
        }

        if (specNum > player->GetSpecsCount())
        {
            handler->PSendSysMessage("You only have {} spec(s) unlocked.", player->GetSpecsCount());
            return true;
        }

        uint8 specIndex = specNum - 1; // 0-based internally
        if (specIndex == player->GetActiveSpec())
        {
            handler->PSendSysMessage("Spec {} is already active.", specNum);
            return true;
        }

        player->ActivateSpec(specIndex);
        handler->PSendSysMessage("Switched to spec {}.", specNum);
        SendSpecState(player, handler);
        return true;
    }

    // .spec preview <1-8> — load a spec's talent data into client slot 1
    static bool HandleSpecPreviewCommand(ChatHandler* handler, uint8 specNum)
    {
        Player* player = handler->GetSession()->GetPlayer();
        if (!player)
            return false;

        if (specNum < 1 || specNum > MAX_TALENT_SPECS)
        {
            handler->PSendSysMessage("Spec must be between 1 and {}.", MAX_TALENT_SPECS);
            return true;
        }

        if (specNum > player->GetSpecsCount())
        {
            handler->PSendSysMessage("You only have {} spec(s) unlocked.", player->GetSpecsCount());
            return true;
        }

        uint8 specIndex = specNum - 1;
        player->SetClientPreviewSpec(specIndex);
        player->SendTalentsInfoData(false); // re-send packet with new preview
        SendSpecState(player, handler);
        return true;
    }

    // .spec unlock [N]
    static bool HandleSpecUnlockCommand(ChatHandler* handler, Optional<uint8> count)
    {
        Player* player = handler->GetSession()->GetPlayer();
        if (!player)
            return false;

        uint8 newCount = count.value_or(player->GetSpecsCount() + 1);

        if (newCount < 1 || newCount > MAX_TALENT_SPECS)
        {
            handler->PSendSysMessage("Spec count must be between 1 and {}.", MAX_TALENT_SPECS);
            return true;
        }

        if (newCount == player->GetSpecsCount())
        {
            handler->PSendSysMessage("You already have {} spec(s).", newCount);
            return true;
        }

        player->UpdateSpecCount(newCount);
        handler->PSendSysMessage("Spec count set to {}.", newCount);
        SendSpecState(player, handler);
        return true;
    }

    // .spec count
    static bool HandleSpecCountCommand(ChatHandler* handler)
    {
        Player* player = handler->GetSession()->GetPlayer();
        if (!player)
            return false;

        handler->PSendSysMessage("Active: spec {} of {} unlocked.",
            player->GetActiveSpec() + 1, player->GetSpecsCount());
        return true;
    }

    // .spec query (machine-readable, for addon)
    static bool HandleSpecQueryCommand(ChatHandler* handler)
    {
        Player* player = handler->GetSession()->GetPlayer();
        if (!player)
            return false;

        SendSpecState(player, handler);
        return true;
    }
};

void AddSC_spec_command()
{
    new spec_commandscript();
}
