#pragma once
#include "Module.hpp"

#include "EngineDemoPlayer.hpp"
#include "EngineDemoRecorder.hpp"

#include "Command.hpp"
#include "Interface.hpp"
#include "Utils.hpp"
#include "Variable.hpp"

#if _WIN32
#define IServerMessageHandler_VMT_Offset 8
#endif

class Engine : public Module {
public:
    Interface* engineClient = nullptr;
    Interface* cl = nullptr;
    Interface* s_GameEventManager = nullptr;
    Interface* eng = nullptr;
    Interface* debugoverlay = nullptr;
    Interface* s_ServerPlugin = nullptr;

    using _ClientCmd = int(__rescall*)(void* thisptr, const char* szCmdString);
    using _GetLocalPlayer = int(__rescall*)(void* thisptr);
    using _GetViewAngles = int(__rescall*)(void* thisptr, QAngle& va);
    using _SetViewAngles = int(__rescall*)(void* thisptr, QAngle& va);
    using _GetMaxClients = int (*)();
    using _GetGameDirectory = char*(__cdecl*)();
    using _AddListener = bool(__rescall*)(void* thisptr, IGameEventListener2* listener, const char* name, bool serverside);
    using _RemoveListener = bool(__rescall*)(void* thisptr, IGameEventListener2* listener);
    using _Cbuf_AddText = void(__cdecl*)(int slot, const char* pText, int nTickDelay);
    using _AddText = void(__rescall*)(void* thisptr, const char* pText, int nTickDelay);
    using _ClientCommand = int (*)(void* thisptr, void* pEdict, const char* szFmt, ...);
    using _GetLocalClient = int (*)(int index);
#ifdef _WIN32
    using _GetScreenSize = int(__stdcall*)(int& width, int& height);
    using _GetActiveSplitScreenPlayerSlot = int (*)();
    using _ScreenPosition = int(__stdcall*)(const Vector& point, Vector& screen);
    using _ConPrintEvent = int(__stdcall*)(IGameEvent* ev);
#else
    using _GetScreenSize = int(__cdecl*)(void* thisptr, int& width, int& height);
    using _GetActiveSplitScreenPlayerSlot = int (*)(void* thisptr);
    using _ScreenPosition = int(__stdcall*)(void* thisptr, const Vector& point, Vector& screen);
    using _ConPrintEvent = int(__cdecl*)(void* thisptr, IGameEvent* ev);
#endif

    _GetScreenSize GetScreenSize = nullptr;
    _ClientCmd ClientCmd = nullptr;
    _GetLocalPlayer GetLocalPlayer = nullptr;
    _GetViewAngles GetViewAngles = nullptr;
    _SetViewAngles SetViewAngles = nullptr;
    _GetMaxClients GetMaxClients = nullptr;
    _GetGameDirectory GetGameDirectory = nullptr;
    _GetActiveSplitScreenPlayerSlot GetActiveSplitScreenPlayerSlot = nullptr;
    _AddListener AddListener = nullptr;
    _RemoveListener RemoveListener = nullptr;
    _Cbuf_AddText Cbuf_AddText = nullptr;
    _AddText AddText = nullptr;
    _ScreenPosition ScreenPosition = nullptr;
    _ConPrintEvent ConPrintEvent = nullptr;
    _ClientCommand ClientCommand = nullptr;
    _GetLocalClient GetLocalClient = nullptr;

    int* tickcount = nullptr;
    double* net_time = nullptr;
    float* interval_per_tick = nullptr;
    char* m_szLevelName = nullptr;
    bool* m_bLoadgame = nullptr;
    CHostState* hoststate = nullptr;
    void* s_CommandBuffer = nullptr;

    bool overlayActivated = false;

public:
    void ExecuteCommand(const char* cmd);
    int GetTick();
    float ToTime(int tick);
    int GetLocalPlayerIndex();
    edict_t* PEntityOfEntIndex(int iEntIndex);
    QAngle GetAngles(int nSlot);
    void SetAngles(int nSlot, QAngle va);
    void SendToCommandBuffer(const char* text, int delay);
    int PointToScreen(const Vector& point, Vector& screen);
    void SafeUnload(const char* postCommand = nullptr);

    Engine()
        : Module(MODULE("engine"))
    {
        this->isHookable = true;
    }

    void Init() override;
    void Shutdown() override;
};

extern Engine* engine;

extern Variable host_framerate;
extern Variable net_showmsg;

#define TIME_TO_TICKS(dt) ((int)(0.5f + (float)(dt) / *engine->interval_per_tick))
#define GET_SLOT() engine->GetLocalPlayerIndex() - 1
#define IGNORE_DEMO_PLAYER()     \
    if (demoplayer->IsPlaying()) \
        return;

#ifdef _WIN32
#define GET_ACTIVE_SPLITSCREEN_SLOT() engine->GetActiveSplitScreenPlayerSlot()
#else
#define GET_ACTIVE_SPLITSCREEN_SLOT() engine->GetActiveSplitScreenPlayerSlot(nullptr)
#endif
