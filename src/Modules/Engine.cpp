#include "Engine.hpp"

#include <cstring>

#include "Features/Cvars.hpp"
#include "Features/Session.hpp"
#include "Features/Speedrun/SpeedrunTimer.hpp"

#include "Console.hpp"
#include "EngineDemoPlayer.hpp"
#include "EngineDemoRecorder.hpp"
#include "Server.hpp"

#include "Game.hpp"
#include "Interface.hpp"
#include "SAR.hpp"
#include "Utils.hpp"
#include "Variable.hpp"

Variable host_framerate;
Variable net_showmsg;

void Engine::ExecuteCommand(const char* cmd)
{
    this->ClientCmd(this->engineClient->ThisPtr(), cmd);
}
int Engine::GetTick()
{
    return (this->GetMaxClients() < 2) ? *this->tickcount : TIME_TO_TICKS(*this->net_time);
}
float Engine::ToTime(int tick)
{
    return tick * *this->interval_per_tick;
}
int Engine::GetLocalPlayerIndex()
{
    return this->GetLocalPlayer(this->engineClient->ThisPtr());
}
edict_t* Engine::PEntityOfEntIndex(int iEntIndex)
{
    if (iEntIndex >= 0 && iEntIndex < server->gpGlobals->maxEntities) {
        auto pEdict = reinterpret_cast<edict_t*>((uintptr_t)server->gpGlobals->pEdicts + iEntIndex * sizeof(edict_t));
        if (!pEdict->IsFree()) {
            return pEdict;
        }
    }

    return nullptr;
}
QAngle Engine::GetAngles(int nSlot)
{
    auto va = QAngle();
    if (this->GetLocalClient) {
        auto client = this->GetLocalClient(nSlot);
        if (client) {
            va = *reinterpret_cast<QAngle*>((uintptr_t)client + Offsets::viewangles);
        }
    } else {
        this->GetViewAngles(this->engineClient->ThisPtr(), va);
    }
    return va;
}
void Engine::SetAngles(int nSlot, QAngle va)
{
    if (this->GetLocalClient) {
        auto client = this->GetLocalClient(nSlot);
        if (client) {
            auto viewangles = reinterpret_cast<QAngle*>((uintptr_t)client + Offsets::viewangles);
            viewangles->x = Math::AngleNormalize(va.x);
            viewangles->y = Math::AngleNormalize(va.y);
            viewangles->z = Math::AngleNormalize(va.z);
        }
    } else {
        this->SetViewAngles(this->engineClient->ThisPtr(), va);
    }
}
void Engine::SendToCommandBuffer(const char* text, int delay)
{
    if (sar.game->Is(SourceGame_Portal2Engine)) {
#ifdef _WIN32
        auto slot = this->GetActiveSplitScreenPlayerSlot();
#else
        auto slot = this->GetActiveSplitScreenPlayerSlot(nullptr);
#endif
        this->Cbuf_AddText(slot, text, delay);
    } else if (sar.game->Is(SourceGame_HalfLife2Engine)) {
        this->AddText(this->s_CommandBuffer, text, delay);
    }
}
int Engine::PointToScreen(const Vector& point, Vector& screen)
{
#ifdef _WIN32
    return this->ScreenPosition(point, screen);
#else
    return this->ScreenPosition(nullptr, point, screen);
#endif
}
void Engine::SafeUnload(const char* postCommand)
{
    // The exit command will handle everything
    this->ExecuteCommand("sar_exit");

    if (postCommand) {
        this->SendToCommandBuffer(postCommand, SAFE_UNLOAD_TICK_DELAY);
    }
}

// CClientState::Disconnect
DETOUR(Disconnect, bool bShowMainMenu)
{
    session->Ended();
    return Disconnect(thisptr, bShowMainMenu);
}
#ifdef _WIN32
DETOUR(Disconnect2, int unk1, int unk2, int unk3)
{
    session->Ended();
    return Disconnect2(thisptr, unk1, unk2, unk3);
}
DETOUR_COMMAND(connect)
{
    session->Ended();
    connect_callback(args);
}
#else
DETOUR(Disconnect2, int unk, bool bShowMainMenu)
{
    session->Ended();
    return Disconnect2(thisptr, unk, bShowMainMenu);
}
#endif

// CClientState::SetSignonState
DETOUR(SetSignonState, int state, int count, void* unk)
{
    session->Changed(state);
    return SetSignonState(thisptr, state, count, unk);
}
DETOUR(SetSignonState2, int state, int count)
{
    session->Changed(state);
    return SetSignonState2(thisptr, state, count);
}

// CEngine::Frame
DETOUR(Frame)
{
    speedrun->PreUpdate(engine->GetTick(), engine->m_szLevelName);

    if (engine->hoststate->m_currentState != session->prevState) {
        session->Changed();
    }
    session->prevState = engine->hoststate->m_currentState;

    if (engine->hoststate->m_activeGame || std::strlen(engine->m_szLevelName) == 0) {
        speedrun->PostUpdate(engine->GetTick(), engine->m_szLevelName);
    }

    return Frame(thisptr);
}

// CSteam3Client::OnGameOverlayActivated
DETOUR_B(OnGameOverlayActivated, GameOverlayActivated_t* pGameOverlayActivated)
{
    engine->overlayActivated = pGameOverlayActivated->m_bActive;
    return OnGameOverlayActivatedBase(thisptr, pGameOverlayActivated);
}

DETOUR_COMMAND(plugin_load)
{
    // Prevent crash when trying to load SAR twice or try to find the module in
    // the plugin list if the initial search thread failed
    if (args.ArgC() >= 2) {
        auto file = std::string(args[1]);
        if (Utils::EndsWith(file, std::string(MODULE("sar"))) || Utils::EndsWith(file, std::string("sar"))) {
            if (sar.GetPlugin()) {
                sar.plugin->ptr->m_bDisable = true;
                console->PrintActive("SAR: Plugin fully loaded!\n");
            }
            return;
        }
    }

    plugin_load_callback(args);
}
DETOUR_COMMAND(plugin_unload)
{
    if (args.ArgC() >= 2 && sar.GetPlugin() && std::atoi(args[1]) == sar.plugin->index) {
        engine->SafeUnload();
    } else {
        plugin_unload_callback(args);
    }
}
DETOUR_COMMAND(exit)
{
    engine->SafeUnload("exit");
}
DETOUR_COMMAND(quit)
{
    engine->SafeUnload("quit");
}
DETOUR_COMMAND(help)
{
    cvars->PrintHelp(args);
}
DETOUR_COMMAND(gameui_activate)
{
    if (sar_disable_steam_pause.GetBool() && engine->overlayActivated) {
        return;
    }

    gameui_activate_callback(args);
}

void Engine::Init()
{
    this->s_ServerPlugin = Interface::CreateNew(this, "ISERVERPLUGINHELPERS0");

    this->engineClient = Interface::CreateNew(this, "VEngineClient0");
    this->GetScreenSize = this->engineClient->Original<_GetScreenSize>(Offsets::GetScreenSize);
    this->ClientCmd = this->engineClient->Original<_ClientCmd>(Offsets::ClientCmd);
    this->GetLocalPlayer = this->engineClient->Original<_GetLocalPlayer>(Offsets::GetLocalPlayer);
    this->GetViewAngles = this->engineClient->Original<_GetViewAngles>(Offsets::GetViewAngles);
    this->SetViewAngles = this->engineClient->Original<_SetViewAngles>(Offsets::SetViewAngles);
    this->GetMaxClients = this->engineClient->Original<_GetMaxClients>(Offsets::GetMaxClients);
    this->GetGameDirectory = this->engineClient->Original<_GetGameDirectory>(Offsets::GetGameDirectory);

    Memory::Read<_Cbuf_AddText>((uintptr_t)this->ClientCmd + Offsets::Cbuf_AddText, &this->Cbuf_AddText);
    Memory::Deref<void*>((uintptr_t)this->Cbuf_AddText + Offsets::s_CommandBuffer, &this->s_CommandBuffer);

    if (sar.game->Is(SourceGame_Portal2Engine)) {
        Memory::Read((uintptr_t)this->SetViewAngles + Offsets::GetLocalClient, &this->GetLocalClient);

        if (sar.game->Is(SourceGame_Portal2Game)) {
            auto GetSteamAPIContext = this->engineClient->Original<uintptr_t (*)()>(Offsets::GetSteamAPIContext);
            auto OnGameOverlayActivated = reinterpret_cast<_OnGameOverlayActivated*>(GetSteamAPIContext() + Offsets::OnGameOverlayActivated);

            OnGameOverlayActivatedBase = *OnGameOverlayActivated;
            *OnGameOverlayActivated = reinterpret_cast<_OnGameOverlayActivated>(OnGameOverlayActivated_Hook);
        }

        Interface::Temp(this, "VEngineServer0", [this](const Interface* g_VEngineServer) {
            this->ClientCommand = g_VEngineServer->Original<_ClientCommand>(Offsets::ClientCommand);
        });
    }

    void* clPtr = nullptr;
    if (sar.game->Is(SourceGame_Portal2Engine)) {
        typedef void* (*_GetClientState)();
        auto GetClientState = Memory::Read<_GetClientState>((uintptr_t)this->ClientCmd + Offsets::GetClientStateFunction);
        clPtr = GetClientState();

        this->GetActiveSplitScreenPlayerSlot = this->engineClient->Original<_GetActiveSplitScreenPlayerSlot>(Offsets::GetActiveSplitScreenPlayerSlot);
    } else if (sar.game->Is(SourceGame_HalfLife2Engine)) {
        auto ServerCmdKeyValues = this->engineClient->Original(Offsets::ServerCmdKeyValues);
        clPtr = Memory::Deref<void*>(ServerCmdKeyValues + Offsets::cl);

        Memory::Read<_AddText>((uintptr_t)this->Cbuf_AddText + Offsets::AddText, &this->AddText);
    }

    this->cl = Interface::Hookable(this, clPtr);
    if (sar.game->Is(SourceGame_Portal2Engine)) {
        this->cl->Hook(&hkSetSignonState, Offsets::Disconnect - 1);
        this->cl->Hook(&hkDisconnect, Offsets::Disconnect);
    } else if (sar.game->Is(SourceGame_HalfLife2Engine)) {
        this->cl->Hook(&hkSetSignonState2, Offsets::Disconnect - 1);
#ifdef _WIN32
        connect_hook.Register(this);
#else
        this->cl->Hook(&hkDisconnect2, Offsets::Disconnect);
#endif
    }
#if _WIN32
    auto IServerMessageHandler_VMT = Memory::Deref<uintptr_t>((uintptr_t)this->cl->ThisPtr() + IServerMessageHandler_VMT_Offset);
    auto ProcessTick = Memory::Deref<uintptr_t>(IServerMessageHandler_VMT + sizeof(uintptr_t) * Offsets::ProcessTick);
#else
    auto ProcessTick = this->cl->Original(Offsets::ProcessTick);
#endif
    tickcount = Memory::Deref<int*>(ProcessTick + Offsets::tickcount);
    interval_per_tick = Memory::Deref<float*>(ProcessTick + Offsets::interval_per_tick);
    speedrun->SetIntervalPerTick(interval_per_tick);

    auto SetSignonState = this->cl->Original(Offsets::Disconnect - 1);
    auto HostState_OnClientConnected = Memory::Read(SetSignonState + Offsets::HostState_OnClientConnected);
    Memory::Deref<CHostState*>(HostState_OnClientConnected + Offsets::hoststate, &hoststate);

    Interface::Temp(this, "VENGINETOOL0", [this](const Interface* tool) {
        auto GetCurrentMap = tool->Original(Offsets::GetCurrentMap);
        this->m_szLevelName = Memory::Deref<char*>(GetCurrentMap + Offsets::m_szLevelName);

        if (sar.game->Is(SourceGame_HalfLife2Engine) && std::strlen(this->m_szLevelName) != 0) {
            throw std::runtime_error("tried to load the plugin when server is active");
        }

        this->m_bLoadgame = reinterpret_cast<bool*>((uintptr_t)this->m_szLevelName + Offsets::m_bLoadGame);
    });

    Interface::Temp(this, "VENGINE_LAUNCHER_API_VERSION0", [this](const Interface* s_EngineAPI) {
        auto IsRunningSimulation = s_EngineAPI->Original(Offsets::IsRunningSimulation);
        auto engAddr = Memory::DerefDeref<void*>(IsRunningSimulation + Offsets::eng);

        if (this->eng = Interface::Hookable(this, engAddr)) {
            if (this->tickcount && this->hoststate && this->m_szLevelName) {
                this->eng->Hook(&hkFrame, Offsets::Frame);
            }
        }
    });

    if (sar.game->Is(SourceGame_Portal2 | SourceGame_ApertureTag)) {
        this->s_GameEventManager = Interface::CreateNew(this, "GAMEEVENTSMANAGER002");
        this->AddListener = this->s_GameEventManager->Original<_AddListener>(Offsets::AddListener);
        this->RemoveListener = this->s_GameEventManager->Original<_RemoveListener>(Offsets::RemoveListener);

        auto FireEventClientSide = s_GameEventManager->Original(Offsets::FireEventClientSide);
        auto FireEventIntern = Memory::Read(FireEventClientSide + Offsets::FireEventIntern);
        Memory::Read<_ConPrintEvent>(FireEventIntern + Offsets::ConPrintEvent, &this->ConPrintEvent);
    }

    Interface::Temp(this, "VDebugOverlay0", [this](const Interface* debugoverlay) {
        this->ScreenPosition = debugoverlay->Original<_ScreenPosition>(Offsets::ScreenPosition);
    });

    plugin_load_hook.Register(this);
    plugin_unload_hook.Register(this);
    exit_hook.Register(this);
    quit_hook.Register(this);
    help_hook.Register(this);

    if (sar.game->Is(SourceGame_Portal2Game)) {
        gameui_activate_hook.Register(this);
    }

    host_framerate = Variable("host_framerate");
    net_showmsg = Variable("net_showmsg");
}
void Engine::Shutdown()
{
    if (this->engineClient && sar.game->Is(SourceGame_Portal2Game)) {
        auto GetSteamAPIContext = this->engineClient->Original<uintptr_t (*)()>(Offsets::GetSteamAPIContext);
        auto OnGameOverlayActivated = reinterpret_cast<_OnGameOverlayActivated*>(GetSteamAPIContext() + Offsets::OnGameOverlayActivated);
        *OnGameOverlayActivated = OnGameOverlayActivatedBase;
    }

    Interface::Destroy(this->engineClient);
    Interface::Destroy(this->s_ServerPlugin);
    Interface::Destroy(this->cl);
    Interface::Destroy(this->eng);
    Interface::Destroy(this->s_GameEventManager);
}

Engine* engine;
