#include "Interface.hpp"

#include <cstring>
#include <functional>
#include <stdexcept>

#include "Modules/Console.hpp"
#include "Modules/Module.hpp"

#include "Utils/Memory.hpp"
#include "Utils/SDK.hpp"

#define CreateInterfaceInternal_Offset 5
#ifdef _WIN32
#define s_pInterfaceRegs_Offset 6
#else
#define s_pInterfaceRegs_Offset 11
#endif

Interface::Interface(void* baseclass)
    : Interface(reinterpret_cast<uintptr_t>(baseclass))
{
}
Interface::Interface(uintptr_t baseclass)
{
    this->baseclass = reinterpret_cast<uintptr_t**>(baseclass);
    this->vtable = *this->baseclass;
}
Interface::~Interface()
{
    this->DisableHooks();
    if (this->copy) {
        delete[] this->copy;
        this->copy = nullptr;
    }
    this->hooks.clear();
}
void Interface::AllocVtable()
{
    if (!this->copy) {
        this->vtableSize = 0;
        while (this->vtable[this->vtableSize]) {
            ++this->vtableSize;
        }

        this->copy = new uintptr_t[this->vtableSize + 1];
        std::memcpy(this->copy, this->vtable - 1, sizeof(uintptr_t) + this->vtableSize * sizeof(uintptr_t));

        for (auto const& hk : this->hooks) {
            auto index = hk.first;
            auto hook = hk.second;

            if (index >= 0 && index < this->vtableSize) {
                this->copy[index + 1] = hook->detour;
                *hook->original = this->Original(index);
            } else {
                throw std::runtime_error("invalid vtable index value of " + std::to_string(index));
            }
        }
    }
}
void Interface::EnableHooks()
{
    this->AllocVtable();

    if (this->IsHookable() && !this->isHooked) {
        *this->baseclass = this->copy + 1;
        this->isHooked = true;
    }
}
void Interface::DisableHooks()
{
    if (this->IsHookable() && this->isHooked) {
        *this->baseclass = this->vtable;
        this->isHooked = false;
    }
}
bool Interface::Unhook(int index)
{
    if (this->IsHookable() && index >= 0 && index < this->vtableSize) {
        this->copy[index + 1] = this->vtable[index];
        return true;
    }
    return false;
}

void Interface::Hook(VHook* hook, int index)
{
    if (this->IsHookable()) {
        throw std::runtime_error("interface is not hookable");
    }
    if (this->hooks.find(index) != this->hooks.end()) {
        throw std::runtime_error("vtable index already in use");
    }
    this->hooks.insert({ index, hook });
}

Interface* Interface::CreateNew(void* ptr)
{
    return (ptr) ? new Interface(ptr) : throw std::runtime_error("called Interface::CreateNew with nullptr");
}
Interface* Interface::CreateNew(uintptr_t ptr)
{
    return (ptr) ? new Interface(ptr) : throw std::runtime_error("called Interface::CreateNew with nullptr");
}
Interface* Interface::CreateNew(const Module* mod, const char* interfaceSymbol)
{
    auto ptr = Interface::This(mod, interfaceSymbol);
    return (ptr) ? new Interface(ptr) : throw std::runtime_error("Interface::This could not resolve interface");
}

Interface* Interface::Hookable(Module* mod, void* ptr)
{
    if (!ptr) {
        throw std::runtime_error("called Interface::Hookable with nullptr");
    }

    auto res = new Interface(ptr);
    mod->interfaces.push_back(res);
    return res;
}
Interface* Interface::Hookable(Module* mod, uintptr_t ptr)
{
    if (!ptr) {
        throw std::runtime_error("called Interface::Hookable with nullptr");
    }

    auto res = new Interface(ptr);
    mod->interfaces.push_back(res);
    return res;
}
Interface* Interface::Hookable(Module* mod, const char* interfaceSymbol)
{
    auto ptr = Interface::This(mod, interfaceSymbol);
    if (!ptr) {
        throw std::runtime_error("Interface::This could not resolve interface");
    }

    auto res = new Interface(ptr);
    mod->interfaces.push_back(res);
    return res;
}

void Interface::Temp(void* ptr, std::function<void(const Interface* temp)> callback)
{
    if (!ptr) {
        throw std::runtime_error("called Interface::Temp with nullptr");
    }

    auto temp = Interface(ptr);
    try {
        callback(&temp);
    } catch (std::exception& ex) {
        throw ex;
    }
}
void Interface::Temp(uintptr_t ptr, std::function<void(const Interface* temp)> callback)
{
    if (!ptr) {
        throw std::runtime_error("called Interface::Temp with nullptr");
    }

    auto temp = Interface(ptr);
    try {
        callback(&temp);
    } catch (std::exception& ex) {
        throw ex;
    }
}
void Interface::Temp(const Module* mod, const char* interfaceSymbol, std::function<void(const Interface* temp)> callback)
{
    auto ptr = Interface::This(mod, interfaceSymbol);
    if (!ptr) {
        throw std::runtime_error("Interface::This could not resolve interface");
    }

    auto temp = Interface(ptr);
    try {
        callback(&temp);
    } catch (std::exception& ex) {
        throw ex;
    }
}

void Interface::Destroy(Interface* ptr)
{
    if (ptr) {
        delete ptr;
        ptr = nullptr;
    }
}

void* Interface::This(const Module* mod, const char* interfaceSymbol)
{
    auto handle = Memory::GetModuleHandleByName(mod->filename);
    if (!handle) {
        console->DevWarning("SAR: Failed to open module %s!\n", mod->filename);
        return nullptr;
    }

    auto CreateInterface = Memory::GetSymbolAddress<uintptr_t>(handle, "CreateInterface");
    Memory::CloseModuleHandle(handle);

    if (!CreateInterface) {
        console->DevWarning("SAR: Failed to find symbol CreateInterface for %s!\n", mod->filename);
        return nullptr;
    }

#ifdef _WIN32
    auto obe = Memory::Deref<uint8_t>(CreateInterface) == 0xE9; // jmp
#else
    auto obe = false;
#endif

    auto CreateInterfaceInternal = Memory::Read(CreateInterface + (obe ? 1 : CreateInterfaceInternal_Offset));
    auto s_pInterfaceRegs = Memory::DerefDeref<InterfaceReg*>(CreateInterfaceInternal + (obe ? 3 : s_pInterfaceRegs_Offset));

    void* result = nullptr;
    for (auto& current = s_pInterfaceRegs; current; current = current->m_pNext) {
        if (std::strncmp(current->m_pName, interfaceSymbol, std::strlen(interfaceSymbol)) == 0) {
            result = current->m_CreateFn();
            break;
        }
    }

    if (!result) {
        console->DevWarning("SAR: Failed to find interface with symbol %s in %s!\n", interfaceSymbol, mod->filename);
        return nullptr;
    }
    return result;
}
