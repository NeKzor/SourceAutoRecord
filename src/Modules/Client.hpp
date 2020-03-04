#pragma once
#include <cstdint>

#include "Module.hpp"

#include "Command.hpp"
#include "Interface.hpp"
#include "Offsets.hpp"
#include "Utils.hpp"
#include "Variable.hpp"

class Client : public Module {
private:
    Interface* g_ClientDLL = nullptr;
    Interface* g_pClientMode = nullptr;
    Interface* g_pClientMode2 = nullptr;
    Interface* g_HUDChallengeStats = nullptr;
    Interface* s_EntityList = nullptr;
    Interface* g_Input = nullptr;

public:
    using _GetClientEntity = void*(__rescall*)(void* thisptr, int entnum);
    using _KeyDown = int(__cdecl*)(void* b, const char* c);
    using _KeyUp = int(__cdecl*)(void* b, const char* c);
    using _GetAllClasses = ClientClass* (*)();

    _GetClientEntity GetClientEntity = nullptr;
    _KeyDown KeyDown = nullptr;
    _KeyUp KeyUp = nullptr;
    _GetAllClasses GetAllClasses = nullptr;

    void* in_jump = nullptr;

public:
    ENTPROP(GetAbsOrigin, Vector, C_m_vecAbsOrigin);
    ENTPROP(GetAbsAngles, QAngle, C_m_angAbsRotation);
    ENTPROP(GetLocalVelocity, Vector, C_m_vecVelocity);
    ENTPROP(GetViewOffset, Vector, C_m_vecViewOffset);

    void* GetPlayer(int index);
    void CalcButtonBits(int nSlot, int& bits, int in_button, int in_ignore, kbutton_t* button, bool reset);

    Client()
        : Module(MODULE("client"))
    {
        this->isHookable = true;
    }

    void Init() override;
    void Shutdown() override;
};

extern Client* client;

extern Variable cl_showpos;
extern Variable cl_sidespeed;
extern Variable cl_forwardspeed;
extern Variable in_forceuser;
extern Variable cl_fov;
