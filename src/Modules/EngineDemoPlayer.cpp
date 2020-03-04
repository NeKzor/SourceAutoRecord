#include "EngineDemoPlayer.hpp"

#include "Features/Demo/Demo.hpp"
#include "Features/Demo/DemoParser.hpp"

#include "Console.hpp"
#include "Engine.hpp"

#include "Interface.hpp"
#include "Offsets.hpp"
#include "SAR.hpp"
#include "Utils.hpp"

int EngineDemoPlayer::GetTick()
{
    return this->GetPlaybackTick(this->s_ClientDemoPlayer->ThisPtr());
}
bool EngineDemoPlayer::IsPlaying()
{
    return this->IsPlayingBack(this->s_ClientDemoPlayer->ThisPtr());
}

// CDemoRecorder::StartPlayback
DETOUR(StartPlayback, const char* filename, bool bAsTimeDemo)
{
    auto result = StartPlayback(thisptr, filename, bAsTimeDemo);

    if (result) {
        DemoParser parser;
        Demo demo;
        auto dir = std::string(engine->GetGameDirectory()) + std::string("/")
            + std::string(demoplayer->DemoName);
        if (parser.Parse(dir, &demo)) {
            parser.Adjust(&demo);
            console->Print("Client:   %s\n", demo.clientName);
            console->Print("Map:      %s\n", demo.mapName);
            console->Print("Ticks:    %i\n", demo.playbackTicks);
            console->Print("Time:     %.3f\n", demo.playbackTime);
            console->Print("Tickrate: %.3f\n", demo.Tickrate());
        } else {
            console->Print("Could not parse \"%s\"!\n", demoplayer->DemoName);
        }
    }
    return result;
}

void EngineDemoPlayer::Init()
{
    if (!engine->Loaded()) {
        throw std::runtime_error("engine module is required to be loaded");
    }

    auto disconnect = engine->cl->Original(Offsets::Disconnect);
    auto demoplayer = Memory::DerefDeref<uintptr_t>(disconnect + Offsets::demoplayer);

    this->s_ClientDemoPlayer = Interface::Hookable(this, demoplayer);
    this->s_ClientDemoPlayer->Hook(&hkStartPlayback, Offsets::StartPlayback);

    this->GetPlaybackTick = s_ClientDemoPlayer->Original<_GetPlaybackTick>(Offsets::GetPlaybackTick);
    this->IsPlayingBack = s_ClientDemoPlayer->Original<_IsPlayingBack>(Offsets::IsPlayingBack);
    this->DemoName = reinterpret_cast<char*>(demoplayer + Offsets::m_szFileName);
}
void EngineDemoPlayer::Shutdown()
{
    Interface::Destroy(this->s_ClientDemoPlayer);
}

EngineDemoPlayer* demoplayer;
