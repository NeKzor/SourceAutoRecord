#pragma once
#include "Module.hpp"

#include "Interface.hpp"
#include "Offsets.hpp"
#include "Utils.hpp"
#include "Variable.hpp"

#include <chrono>

#ifdef _WIN32
#define AirMove_Mid_Offset 679
#define AirMove_Signature "F3 0F 10 50 40"
#define AirMove_Continue_Offset 5
#define AirMove_Skip_Offset 142
#endif

class Server : public Module {
public:
    Interface* g_GameMovement = nullptr;
    Interface* g_ServerGameDLL = nullptr;

    using _UTIL_PlayerByIndex = void*(__cdecl*)(int index);
    using _GetAllServerClasses = ServerClass* (*)();
    using _IsRestoring = bool(*)();
    using _CreateEntityByName = void*(__stdcall*)(const char *);
    using _DispatchSpawn = void(__stdcall*)(void*);
    using _SetKeyValueChar = bool(__stdcall*)(void*, const char*, const char*);
    using _SetKeyValueFloat = bool(__stdcall*)(void*, const char*, float);
    using _SetKeyValueVector = bool(__stdcall*)(void*, const char*, const Vector&);

    _UTIL_PlayerByIndex UTIL_PlayerByIndex = nullptr;
    _GetAllServerClasses GetAllServerClasses = nullptr;
    _IsRestoring IsRestoring = nullptr;
    _CreateEntityByName CreateEntityByName = nullptr;
    _DispatchSpawn DispatchSpawn = nullptr;
    _SetKeyValueChar SetKeyValueChar = nullptr;
    _SetKeyValueFloat SetKeyValueFloat = nullptr;
    _SetKeyValueVector SetKeyValueVector = nullptr;

    CGlobalVars* gpGlobals = nullptr;
    CEntInfo* m_EntPtrArray = nullptr;

private:
    bool jumpedLastTime = false;
    float savedVerticalVelocity = 0.0f;
    bool callFromCheckJumpButton = false;
    bool paused = false;
    int pauseTick;

public:
    DECL_M(GetPortals, int);
    DECL_M(GetAbsOrigin, Vector);
    DECL_M(GetAbsAngles, QAngle);
    DECL_M(GetLocalVelocity, Vector);
    DECL_M(GetFlags, int);
    DECL_M(GetEFlags, int);
    DECL_M(GetMaxSpeed, float);
    DECL_M(GetGravity, float);
    DECL_M(GetViewOffset, Vector);
    DECL_M(GetEntityName, char*);
    DECL_M(GetEntityClassName, char*);

    void* GetPlayer(int index);
    bool IsPlayer(void* entity);
    bool AllowsMovementChanges();
    int GetSplitScreenPlayerSlot(void* entity);

public:
    // CGameMovement::ProcessMovement
    DECL_DETOUR(ProcessMovement, void* pPlayer, CMoveData* pMove);

    // CGameMovement::CheckJumpButton
    DECL_DETOUR_T(bool, CheckJumpButton);

    static _CheckJumpButton CheckJumpButtonBase;

    // CGameMovement::PlayerMove
    DECL_DETOUR(PlayerMove);

    // CGameMovement::FinishGravity
    DECL_DETOUR(FinishGravity);

    // CGameMovement::AirMove
    DECL_DETOUR_B(AirMove);

#ifdef _WIN32
    // CGameMovement::AirMove
    static uintptr_t AirMove_Skip;
    static uintptr_t AirMove_Continue;
    DECL_DETOUR_MID_MH(AirMove_Mid);
#endif

// CServerGameDLL::GameFrame
#ifdef _WIN32
    DECL_DETOUR_STD(void, GameFrame, bool simulating);
#else
    DECL_DETOUR(GameFrame, bool simulating);
#endif

    bool Init() override;
    void Shutdown() override;
    const char* Name() override { return MODULE("server"); }
};

extern Server* server;

extern Variable sv_cheats;
extern Variable sv_footsteps;
extern Variable sv_alternateticks;
extern Variable sv_bonus_challenge;
extern Variable sv_accelerate;
extern Variable sv_airaccelerate;
extern Variable sv_friction;
extern Variable sv_maxspeed;
extern Variable sv_stopspeed;
extern Variable sv_maxvelocity;
extern Variable sv_gravity;

extern Variable sar_record_at;
extern Variable sar_record_at_demo_name;
extern Variable sar_record_at_increment;
extern Variable sar_pause;
extern Variable sar_pause_at;
extern Variable sar_pause_for;
