#pragma once
#include "Modules/Engine.hpp"

#include "Features/Session.hpp"
#include "Features/Speedrun/SpeedrunTimer.hpp"
#include "Features/Timer.hpp"

#include "Utils.hpp"

class Listener : public IGameEventListener2 {
private:
    bool m_bRegisteredForEvents;
#if _WIN32
    using _ConPrintEvent = int(__stdcall*)(IGameEvent* event);
    _ConPrintEvent ConPrintEvent;
#endif

public:
    Listener()
        : m_bRegisteredForEvents(false)
        , ConPrintEvent(nullptr)
    {
    }
    void Init()
    {
        for (const auto& event : EVENTS) {
            auto result = Engine::AddListener(Engine::s_GameEventManager->ThisPtr(), this, event, true);
            if (result) {
                //console->DevMsg("SAR: Added event listener for %s!\n", event);
            } else {
                console->DevWarning("SAR: Failed to add event listener for %s!\n", event);
            }
        }
#if _WIN32
        this->ConPrintEvent = Memory::Absolute<_ConPrintEvent>(MODULE("engine"), 0x186C20);
#endif
    }
    void Shutdown()
    {
        Engine::RemoveListener(Engine::s_GameEventManager->ThisPtr(), this);
    }
    virtual ~Listener()
    {
        this->Shutdown();
    }
    virtual void FireGameEvent(IGameEvent* event)
    {
        if (!event)
            return;

        if (sar_debug_game_events.GetBool() && this->ConPrintEvent) {
            console->Print("[%i] Event fired: %s\n", Engine::GetSessionTick(), event->GetName());
#if _WIN32
            this->ConPrintEvent(event);
#endif
        }

        if (Engine::GetMaxClients() >= 2) {
            if (!std::strcmp(event->GetName(), "player_spawn_blue") || !std::strcmp(event->GetName(), "player_spawn_orange")) {
                console->Print("Detected cooperative spawn!\n");
                Session::Rebase(*Engine::tickcount);
                Timer::Rebase(*Engine::tickcount);
                speedrun->Unpause(Engine::tickcount);
            }
        }
    }
    virtual int GetEventDebugID()
    {
        return 42;
    }
    void DumpGameEvents()
    {
        auto m_Size = *reinterpret_cast<int*>((uintptr_t)Engine::s_GameEventManager->ThisPtr() + 16);
        console->Print("m_Size = %i\n", m_Size);
        if (m_Size > 0) {
            auto m_GameEvents = *reinterpret_cast<uintptr_t*>((uintptr_t)Engine::s_GameEventManager->ThisPtr() + 124);
            for (int i = 0; i < m_Size; i++) {
                auto name = *reinterpret_cast<char**>(m_GameEvents + 24 * i + 16);
                console->Print("%s\n", name);
            }
        }
    }
};

Listener* listener;
extern Listener* listener;