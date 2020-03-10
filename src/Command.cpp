#include "Command.hpp"

#include <cstring>
#include <stdexcept>

#include "Modules/Console.hpp"
#include "Modules/Module.hpp"
#include "Modules/Tier1.hpp"

#include "Game.hpp"
#include "SAR.hpp"

std::vector<CommandBase*>& CommandBase::GetList()
{
    static std::vector<CommandBase*> list;
    return list;
}

CommandBase::CommandBase(int version)
    : ptr(nullptr)
    , version(version)
    , isRegistered(false)
    , isReference(false)
{
}
CommandBase::~CommandBase()
{
    if (!this->isReference) {
        sdelete(this->ptr)
    }
}
int CommandBase::RegisterAll()
{
    auto result = 0;
    for (const auto& command : CommandBase::GetList()) {
        if (command->version != SourceGame_Unknown && !sar.game->Is(command->version)) {
            continue;
        }
        command->Register();
        ++result;
    }
    return result;
}
void CommandBase::UnregisterAll()
{
    for (const auto& command : CommandBase::GetList()) {
        command->Unregister();
    }
}

Command::Command(const char* name)
    : CommandBase(SourceGame_Unknown)
{
    this->ptr = reinterpret_cast<ConCommand*>(tier1->FindCommandBase(tier1->g_pCVar->ThisPtr(), name));
    if (!this->ptr) {
        throw std::runtime_error("command " + std::string(name) + " not found");
    }

    this->isReference = true;
}
Command::Command(const char* pName, _CommandCallback callback, const char* pHelpString, int version, int flags,
    _CommandCompletionCallback completionFunc)
    : CommandBase(version)
{
    this->ptr = new ConCommand(pName, callback, pHelpString, flags, completionFunc);

    CommandBase::GetList().push_back(this);
}
void Command::Register()
{
    if (!this->isRegistered) {
        this->ptr->ConCommandBase_VTable = tier1->ConCommand_VTable;
        tier1->RegisterConCommand(tier1->g_pCVar->ThisPtr(), this->ptr);
        tier1->m_pConCommandList = this->ptr;
    }
    this->isRegistered = true;
}
void Command::Unregister()
{
    if (this->isRegistered) {
        tier1->UnregisterConCommand(tier1->g_pCVar->ThisPtr(), this->ptr);
    }
    this->isRegistered = false;
}
bool Command::ActivateAutoCompleteFile(const char* name, _CommandCompletionCallback callback)
{
    auto cc = Command(name);
    if (!!cc) {
        auto ptr = cc.ThisPtr();
        ptr->m_bHasCompletionCallback = true;
        ptr->m_fnCompletionCallback = callback;
        return true;
    }
    return false;
}
bool Command::DectivateAutoCompleteFile(const char* name)
{
    auto cc = Command(name);
    if (!!cc) {
        auto ptr = cc.ThisPtr();
        ptr->m_bHasCompletionCallback = false;
        ptr->m_fnCompletionCallback = nullptr;
        return true;
    }
    return false;
}

CommandHook::CommandHook(const char* target, _CommandCallback* original, _CommandCallback detour)
    : target(target)
    , original(original)
    , detour(detour)
{
}
void CommandHook::Register(Module* mod)
{
    if (!this->isRegistered) {
        mod->cmdHooks.push_back(this);
    } else {
        console->Warning("CommandHook %s already registered!\n", this->target);
    }
}
void CommandHook::Unregister()
{
    if (this->isRegistered) {
        this->isRegistered = false;
    } else {
        console->Warning("CommandHook %s already unregistered!\n", this->target);
    }
}
void CommandHook::Hook()
{
    auto cmd = Command(this->target);
    if (!!cmd) {
        *this->original = cmd.ThisPtr()->m_fnCommandCallback;
        cmd.ThisPtr()->m_fnCommandCallback = this->detour;
    } else {
        console->Warning("CommandHook failed to find command %s!\n", this->target);
    }
}
void CommandHook::Unhook()
{
    auto cmd = Command(this->target);
    if (!!cmd) {
        cmd.ThisPtr()->m_fnCommandCallback = *this->original;
    } else {
        console->Warning("CommandHook failed to find command %s!\n", this->target);
    }
}
