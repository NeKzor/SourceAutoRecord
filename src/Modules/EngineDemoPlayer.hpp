#pragma once
#include "Module.hpp"

#include "Interface.hpp"
#include "Offsets.hpp"
#include "SAR.hpp"
#include "Utils.hpp"

class EngineDemoPlayer : public Module {
public:
    Interface* s_ClientDemoPlayer = nullptr;

    using _IsPlayingBack = bool(__rescall*)(void* thisptr);
    using _GetPlaybackTick = int(__rescall*)(void* thisptr);

    _IsPlayingBack IsPlayingBack = nullptr;
    _GetPlaybackTick GetPlaybackTick = nullptr;

    char* DemoName = nullptr;

public:
    int GetTick();
    bool IsPlaying();

    EngineDemoPlayer()
        : Module(MODULE("engine"))
    {
        this->isHookable = true;
    }

    void Init() override;
    void Shutdown() override;
};

extern EngineDemoPlayer* demoplayer;
