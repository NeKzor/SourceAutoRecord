#include "Console.hpp"

void Console::Init()
{
    auto tier0 = Memory::GetModuleHandleByName(this->filename);

    this->Msg = Memory::GetSymbolAddress<_Msg>(tier0, MSG_SYMBOL);
    this->ColorMsg = Memory::GetSymbolAddress<_ColorMsg>(tier0, CONCOLORMSG_SYMBOL);
    this->Warning = Memory::GetSymbolAddress<_Warning>(tier0, WARNING_SYMBOL);
    this->DevMsg = Memory::GetSymbolAddress<_DevMsg>(tier0, DEVMSG_SYMBOL);
    this->DevWarning = Memory::GetSymbolAddress<_DevWarning>(tier0, DEVWARNINGMSG_SYMBOL);

    Memory::CloseModuleHandle(tier0);
}
void Console::Shutdown()
{
}

Console* console;
