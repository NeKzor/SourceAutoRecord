#include "Tier1.hpp"

#include "Game.hpp"
#include "Interface.hpp"
#include "Offsets.hpp"
#include "SAR.hpp"
#include "Utils.hpp"

void Tier1::Init()
{
    this->g_pCVar = Interface::CreateNew(this, "VEngineCvar0");

    this->RegisterConCommand = this->g_pCVar->Original<_RegisterConCommand>(Offsets::RegisterConCommand);
    this->UnregisterConCommand = this->g_pCVar->Original<_UnregisterConCommand>(Offsets::UnregisterConCommand);
    this->FindCommandBase = this->g_pCVar->Original<_FindCommandBase>(Offsets::FindCommandBase);

    this->m_pConCommandList = reinterpret_cast<ConCommandBase*>((uintptr_t)this->g_pCVar->ThisPtr() + Offsets::m_pConCommandList);

    auto listdemo = reinterpret_cast<ConCommand*>(this->FindCommandBase(this->g_pCVar->ThisPtr(), "listdemo"));
    if (!listdemo) {
        throw std::runtime_error("listdemo command not found");
    }

    this->ConCommand_VTable = listdemo->ConCommandBase_VTable;
    auto callback = (uintptr_t)listdemo->m_fnCompletionCallback + Offsets::AutoCompletionFunc;
    this->AutoCompletionFunc = Memory::Read<_AutoCompletionFunc>(callback);

    auto sv_lan = reinterpret_cast<ConVar*>(this->FindCommandBase(this->g_pCVar->ThisPtr(), "sv_lan"));
    if (!sv_lan) {
        throw std::runtime_error("sv_lan convar not found");
    }

    this->ConVar_VTable = sv_lan->ConCommandBase_VTable;
    this->ConVar_VTable2 = sv_lan->ConVar_VTable;

    auto vtable =
#ifdef _WIN32
        sar.game->Is(SourceGame_HalfLife2Engine)
        ? &this->ConVar_VTable
        : &this->ConVar_VTable2;
#else
        &this->ConVar_VTable;
#endif

    this->Dtor = Memory::VMT<_Dtor>(vtable, Offsets::Dtor);
    this->Create = Memory::VMT<_Create>(vtable, Offsets::Create);

    if (sar.game->Is(SourceGame_Portal2 | SourceGame_ApertureTag)) {
        this->InstallGlobalChangeCallback = this->g_pCVar->Original<_InstallGlobalChangeCallback>(Offsets::InstallGlobalChangeCallback);
        this->RemoveGlobalChangeCallback = this->g_pCVar->Original<_RemoveGlobalChangeCallback>(Offsets::RemoveGlobalChangeCallback);
    }
}
void Tier1::Shutdown()
{
    Interface::Destroy(this->g_pCVar);
}

Tier1* tier1;

CBaseAutoCompleteFileList::CBaseAutoCompleteFileList(const char* cmdname, const char* subdir, const char* extension)
{
    m_pszCommandName = cmdname;
    m_pszSubDir = subdir;
    m_pszExtension = extension;
}
int CBaseAutoCompleteFileList::AutoCompletionFunc(char const* partial, char commands[COMMAND_COMPLETION_MAXITEMS][COMMAND_COMPLETION_ITEM_LENGTH])
{
    return tier1->AutoCompletionFunc(this, partial, commands);
}
