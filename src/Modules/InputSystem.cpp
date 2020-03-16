#include "InputSystem.hpp"

#include "Cheats.hpp"
#include "Command.hpp"
#include "Interface.hpp"
#include "Module.hpp"
#include "Offsets.hpp"
#include "SAR.hpp"
#include "Utils.hpp"

int InputSystem::GetButton(const char* pString)
{
    return this->StringToButtonCode(this->g_InputSystem->ThisPtr(), pString);
}

// CInputSystem::SleepUntilInput
DETOUR(SleepUntilInput, int nMaxSleepTimeMS)
{
    if (sar_disable_no_focus_sleep.GetBool()) {
        nMaxSleepTimeMS = 0;
    }

    return SleepUntilInput(thisptr, nMaxSleepTimeMS);
}

void InputSystem::Init()
{
    this->g_InputSystem = Interface::Hookable(this, "InputSystemVersion0");
    this->StringToButtonCode = this->g_InputSystem->Original<_StringToButtonCode>(Offsets::StringToButtonCode);

    if (sar.game->Is(SourceGame_Portal2Engine)) {
        this->g_InputSystem->Hook(&hkSleepUntilInput, Offsets::SleepUntilInput);
    }

    auto unbind = Command("unbind");
    auto cc_unbind_callback = (uintptr_t)unbind.ThisPtr()->m_pCommandCallback;
    this->KeySetBinding = Memory::Read<_KeySetBinding>(cc_unbind_callback + Offsets::Key_SetBinding);
}
void InputSystem::Shutdown()
{
    Interface::Destroy(this->g_InputSystem);
}

InputSystem* inputSystem;
