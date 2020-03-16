#include "Cheats.hpp"

#include <cstring>

#include "Features/Cvars.hpp"

#include "Modules/Client.hpp"
#include "Modules/Console.hpp"
#include "Modules/Engine.hpp"
#include "Modules/Server.hpp"

#include "Game.hpp"
#include "Offsets.hpp"

Variable sar_autorecord("sar_autorecord", "0",
    "Enables automatic demo recording.\n");
Variable sar_autojump("sar_autojump", "0",
    "Enables automatic jumping on the server.\n");
Variable sar_jumpboost("sar_jumpboost", "0", 0,
    "Enables special game movement on the server.\n"
    "0 = Default,\n"
    "1 = Orange Box Engine,\n"
    "2 = Pre-OBE.\n",
    SourceGame_Portal2Engine);
Variable sar_aircontrol("sar_aircontrol", "0", 0,
    "Enables more air-control on the server.\n",
    SourceGame_Portal2Engine);
Variable sar_duckjump("sar_duckjump", "0",
    "Allows duck-jumping even when fully crouched, similar to prevent_crouch_jump.\n",
    SourceGame_Portal2Engine);
Variable sar_disable_challenge_stats_hud("sar_disable_challenge_stats_hud", "0",
    "Disables opening the challenge mode stats HUD.\n",
    SourceGame_Portal2);
Variable sar_disable_steam_pause("sar_disable_steam_pause", "0",
    "Prevents pauses from steam overlay.\n",
    SourceGame_Portal2Game);
Variable sar_disable_no_focus_sleep("sar_disable_no_focus_sleep", "0",
    "Does not yield the CPU when game is not focused.\n",
    SourceGame_Portal2Engine);

Variable sv_laser_cube_autoaim;
Variable ui_loadingscreen_transition_time;
Variable ui_loadingscreen_fadein_time;
Variable ui_loadingscreen_mintransition_time;
Variable hide_gun_when_holding;

// TSP only
void IN_BhopDown(const CCommand& args) { client->KeyDown(client->in_jump, (args.ArgC() > 1) ? args[1] : nullptr); }
void IN_BhopUp(const CCommand& args) { client->KeyUp(client->in_jump, (args.ArgC() > 1) ? args[1] : nullptr); }

Command startbhop("+bhop", IN_BhopDown, "Client sends a key-down event for the in_jump state.\n", SourceGame_TheStanleyParable);
Command endbhop("-bhop", IN_BhopUp, "Client sends a key-up event for the in_jump state.\n", SourceGame_TheStanleyParable);

CON_COMMAND_U(sar_anti_anti_cheat, "Sets sv_cheats to 1.\n", SourceGame_TheStanleyParable)
{
    sv_cheats.ThisPtr()->m_nValue = 1;
}

// TSP & TBG only
DECLARE_AUTOCOMPLETION_FUNCTION(map, "maps", bsp);
DECLARE_AUTOCOMPLETION_FUNCTION(changelevel, "maps", bsp);
DECLARE_AUTOCOMPLETION_FUNCTION(changelevel2, "maps", bsp);

// P2, INFRA and HL2 only
#ifdef _WIN32
#define TRACE_SHUTDOWN_PATTERN "6A 00 68 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? "
#define TRACE_SHUTDOWN_OFFSET1 3
#define TRACE_SHUTDOWN_OFFSET2 10
#else
#define TRACE_SHUTDOWN_PATTERN "C7 44 24 04 00 00 00 00 C7 04 24 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? C7"
#define TRACE_SHUTDOWN_OFFSET1 11
#define TRACE_SHUTDOWN_OFFSET2 10
#endif
CON_COMMAND_U(sar_delete_alias_cmds, "Deletes all alias commands.\n", SourceGame_Portal2Game | SourceGame_HalfLife2Engine)
{
    using _Cmd_Shutdown = int (*)();
    static _Cmd_Shutdown Cmd_Shutdown = nullptr;

    if (!Cmd_Shutdown) {
        auto result = Memory::MultiScan(engine->filename, TRACE_SHUTDOWN_PATTERN, TRACE_SHUTDOWN_OFFSET1);
        if (!result.empty()) {
            for (auto const& addr : result) {
                if (!std::strcmp(*reinterpret_cast<char**>(addr), "Cmd_Shutdown()")) {
                    Cmd_Shutdown = Memory::Read<_Cmd_Shutdown>(addr + TRACE_SHUTDOWN_OFFSET2);
                    break;
                }
            }
        }
    }

    if (Cmd_Shutdown) {
        Cmd_Shutdown();
    } else {
        console->Print("Unable to find Cmd_Shutdown() function!\n");
    }
}

void Cheats::Init()
{
    if (sar.game->Is(SourceGame_Portal2Game)) {
        sv_laser_cube_autoaim = Variable("sv_laser_cube_autoaim");
        ui_loadingscreen_transition_time = Variable("ui_loadingscreen_transition_time");
        ui_loadingscreen_fadein_time = Variable("ui_loadingscreen_fadein_time");
        ui_loadingscreen_mintransition_time = Variable("ui_loadingscreen_mintransition_time");
        hide_gun_when_holding = Variable("hide_gun_when_holding");
    } else if (sar.game->Is(SourceGame_TheStanleyParable | SourceGame_TheBeginnersGuide)) {
        Command::ActivateAutoCompleteFile("map", map_CompletionFunc);
        Command::ActivateAutoCompleteFile("changelevel", changelevel_CompletionFunc);
        Command::ActivateAutoCompleteFile("changelevel2", changelevel2_CompletionFunc);
    }

    if (cvars) {
        cvars->Unlock();
    }

    CommandBase::RegisterAll();
}
void Cheats::Shutdown()
{
    if (sar.game->Is(SourceGame_TheStanleyParable | SourceGame_TheBeginnersGuide)) {
        Command::DectivateAutoCompleteFile("map");
        Command::DectivateAutoCompleteFile("changelevel");
        Command::DectivateAutoCompleteFile("changelevel2");
    }

    if (cvars) {
        cvars->Lock();
    }

    CommandBase::UnregisterAll();
}
