#pragma once
#include "Module.hpp"

#include "Interface.hpp"
#include "Offsets.hpp"
#include "Utils.hpp"
#include "Variable.hpp"

class Server : public Module {
public:
    Interface* g_GameMovement = nullptr;
    Interface* g_ServerGameDLL = nullptr;

    using _UTIL_PlayerByIndex = void*(__cdecl*)(int index);
    using _GetAllServerClasses = ServerClass* (*)();
    using _IsRestoring = bool (*)();

    _UTIL_PlayerByIndex UTIL_PlayerByIndex = nullptr;
    _GetAllServerClasses GetAllServerClasses = nullptr;
    _IsRestoring IsRestoring = nullptr;

    CGlobalVars* gpGlobals = nullptr;
    CEntInfo* m_EntPtrArray = nullptr;

    bool jumpedLastTime = false;
    float savedVerticalVelocity = 0.0f;
    bool callFromCheckJumpButton = false;
    bool paused = false;
    int pauseTick = 0;

public:
    ENTPROP(GetPortals, int, iNumPortalsPlaced);
    ENTPROP(GetAbsOrigin, Vector, S_m_vecAbsOrigin);
    ENTPROP(GetAbsAngles, QAngle, S_m_angAbsRotation);
    ENTPROP(GetLocalVelocity, Vector, S_m_vecVelocity);
    ENTPROP(GetFlags, int, m_fFlags);
    ENTPROP(GetEFlags, int, m_iEFlags);
    ENTPROP(GetMaxSpeed, float, m_flMaxspeed);
    ENTPROP(GetGravity, float, m_flGravity);
    ENTPROP(GetViewOffset, Vector, S_m_vecViewOffset);
    ENTPROP(GetEntityName, char*, m_iName);
    ENTPROP(GetEntityClassName, char*, m_iClassName);

    void* GetPlayer(int index);
    bool IsPlayer(void* entity);
    bool AllowsMovementChanges();
    int GetSplitScreenPlayerSlot(void* entity);

    Server()
        : Module(MODULE("server"))
    {
        this->isHookable = true;
    }

    void Init() override;
    void Shutdown() override;
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

extern Variable sar_pause;
extern Variable sar_pause_at;
extern Variable sar_pause_for;
extern Variable sar_record_at;
extern Variable sar_record_at_demo_name;
extern Variable sar_record_at_increment;
