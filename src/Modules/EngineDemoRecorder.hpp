#pragma once
#include <string>

#include "Module.hpp"

#include "Command.hpp"
#include "Interface.hpp"
#include "Utils.hpp"

class EngineDemoRecorder : public Module {
public:
    Interface* s_ClientDemoRecorder = nullptr;

    using _GetRecordingTick = int(__rescall*)(void* thisptr);
    _GetRecordingTick GetRecordingTick = nullptr;

    char* m_szDemoBaseName = nullptr;
    int* m_nDemoNumber = nullptr;
    bool* m_bRecording = nullptr;

    std::string currentDemo = std::string();
    bool isRecordingDemo = false;
    bool requestedStop = false;
    int lastDemoNumber = 1;

public:
    int GetTick();

    EngineDemoRecorder()
        : Module(MODULE("engine"))
    {
        this->isHookable = true;
    }

    void Init() override;
    void Shutdown() override;
};

extern EngineDemoRecorder* demorecorder;
