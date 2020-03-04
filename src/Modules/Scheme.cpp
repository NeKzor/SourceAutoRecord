#include "Scheme.hpp"

#include "Game.hpp"
#include "Interface.hpp"
#include "Offsets.hpp"
#include "Utils.hpp"

unsigned long Scheme::GetDefaultFont()
{
    return this->GetFont(this->g_pScheme->ThisPtr(), "DefaultFixedOutline", 0);
}
void Scheme::Init()
{
    Interface::Temp(this, "VGUI_Scheme0", [this](const Interface* g_pVGuiSchemeManager) {
        using _GetIScheme = uintptr_t(__rescall*)(void* thisptr, unsigned long scheme);
        auto GetIScheme = g_pVGuiSchemeManager->Original<_GetIScheme>(Offsets::GetIScheme);

        // Default scheme is 1
        this->g_pScheme = Interface::CreateNew(GetIScheme(g_pVGuiSchemeManager->ThisPtr(), 1));
        this->GetFont = this->g_pScheme->Original<_GetFont>(Offsets::GetFont);
    });
}
void Scheme::Shutdown()
{
    Interface::Destroy(this->g_pScheme);
}

Scheme* scheme;
