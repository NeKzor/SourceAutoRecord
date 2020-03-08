#include "EngineDemoRecorder.hpp"

#include <stdexcept>

#include "Features/Rebinder.hpp"
#include "Features/Session.hpp"
#include "Features/Timer/Timer.hpp"

#include "Console.hpp"
#include "Engine.hpp"
#include "Server.hpp"

#include "Command.hpp"
#include "Offsets.hpp"
#include "Utils.hpp"

int EngineDemoRecorder::GetTick()
{
    return this->GetRecordingTick(this->s_ClientDemoRecorder->ThisPtr());
}

// CDemoRecorder::SetSignonState
DETOUR(SetSignonState3, int state)
{
    //SIGNONSTATE_FULL is set twice during first CM load. Using SINGONSTATE_SPAWN for demo number increase instead
    if (state == SIGNONSTATE_SPAWN) {
        if (demorecorder->isRecordingDemo || *demorecorder->m_bRecording || sar_record_at_increment.GetBool()) {
            demorecorder->lastDemoNumber++;
        }
    }
    if (state == SIGNONSTATE_FULL) {
        //autorecording in different session (save deletion)
        if (demorecorder->isRecordingDemo) {
            *demorecorder->m_bRecording = true;
        }

        if (*demorecorder->m_bRecording) {
            demorecorder->isRecordingDemo = true;
            *demorecorder->m_nDemoNumber = demorecorder->lastDemoNumber;
            demorecorder->currentDemo = std::string(demorecorder->m_szDemoBaseName);

            if (*demorecorder->m_nDemoNumber > 1) {
                demorecorder->currentDemo += std::string("_") + std::to_string(*demorecorder->m_nDemoNumber);
            }
        }
    }

    return SetSignonState3(thisptr, state);
}

// CDemoRecorder::StopRecording
DETOUR(StopRecording)
{
    // This function does:
    //   m_bRecording = false
    //   m_nDemoNumber = 0
    auto result = StopRecording(thisptr);

    if (demorecorder->isRecordingDemo && sar_autorecord.GetBool() && !demorecorder->requestedStop) {
        *demorecorder->m_nDemoNumber = demorecorder->lastDemoNumber;
        *demorecorder->m_bRecording = true;
    } else if (sar_record_at_increment.GetBool()) {
        *demorecorder->m_nDemoNumber = demorecorder->lastDemoNumber;
        demorecorder->isRecordingDemo = false;
    } else {
        demorecorder->isRecordingDemo = false;
        demorecorder->lastDemoNumber = 1;
    }

    return result;
}

DETOUR_COMMAND(stop)
{
    demorecorder->requestedStop = true;
    stop_callback(args);
    demorecorder->requestedStop = false;
}

void EngineDemoRecorder::Init()
{
    if (!engine->Loaded()) {
        throw std::runtime_error("engine module is required to be loaded");
    }

    auto disconnect = engine->cl->Original(Offsets::Disconnect);
    auto demorecorder = Memory::DerefDeref<uintptr_t>(disconnect + Offsets::demorecorder);

    this->s_ClientDemoRecorder = Interface::Hookable(this, demorecorder);
    this->s_ClientDemoRecorder->Hook(&hkSetSignonState3, Offsets::SetSignonState);
    this->s_ClientDemoRecorder->Hook(&hkStopRecording, Offsets::StopRecording);

    this->GetRecordingTick = s_ClientDemoRecorder->Original<_GetRecordingTick>(Offsets::GetRecordingTick);
    this->m_szDemoBaseName = reinterpret_cast<char*>(demorecorder + Offsets::m_szDemoBaseName);
    this->m_nDemoNumber = reinterpret_cast<int*>(demorecorder + Offsets::m_nDemoNumber);
    this->m_bRecording = reinterpret_cast<bool*>(demorecorder + Offsets::m_bRecording);

    engine->net_time = Memory::Deref<double*>((uintptr_t)this->GetRecordingTick + Offsets::net_time);

    stop_hook.Register(this);
}
void EngineDemoRecorder::Shutdown()
{
    Interface::Destroy(this->s_ClientDemoRecorder);
}

EngineDemoRecorder* demorecorder;
