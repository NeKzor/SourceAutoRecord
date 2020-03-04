#pragma once
#include "Modules/Module.hpp"

#include <functional>
#include <map>
#include <vector>

#include "Utils/Memory.hpp"
#include "Utils/Platform.hpp"

class _Hook;
class Module;

class Interface {
public:
    uintptr_t** baseclass = nullptr;
    uintptr_t* vtable = nullptr;
    int vtableSize = 0;

private:
    bool isHooked = false;
    uintptr_t* copy = nullptr;
    std::map<int, _Hook*> hooks = std::map<int, _Hook*>();

public:
    Interface(void* baseclass);
    Interface(uintptr_t baseclass);
    ~Interface();

private:
    void AllocVtable();

public:
    void EnableHooks();
    void DisableHooks();

    inline bool IsHookable() { return this->copy != nullptr; }

    template <typename T = uintptr_t>
    T Original(int index, bool readJmp = false) const
    {
        if (readJmp) {
            auto source = this->vtable[index] + 1;
            auto rel = *reinterpret_cast<uintptr_t*>(source);
            return (T)(source + rel + sizeof(rel));
        }
        return (T)this->vtable[index];
    }
    template <typename T = uintptr_t>
    T Hooked(int index)
    {
        return IsHookable() ? (T)this->copy[index + 1] : (T) nullptr;
    }
    template <typename T = uintptr_t>
    T Current(int index)
    {
        return (T)(*this->baseclass)[index];
    }

    bool Unhook(int index);

    inline void* ThisPtr() const
    {
        return reinterpret_cast<void*>(this->baseclass);
    }

    // NEW API

    void Hook(_Hook* hook, int index);

    inline uintptr_t ThisUPtr()
    {
        return reinterpret_cast<uintptr_t>(this->baseclass);
    }

    static Interface* CreateNew(void* ptr);
    static Interface* CreateNew(uintptr_t ptr);
    static Interface* CreateNew(const Module* mod, const char* interfaceSymbol);

    static Interface* Hookable(Module* mod, void* ptr);
    static Interface* Hookable(Module* mod, uintptr_t ptr);
    static Interface* Hookable(Module* mod, const char* interfaceSymbol);

    static void Temp(void* ptr, std::function<void(const Interface* temp)> callback);
    static void Temp(uintptr_t ptr, std::function<void(const Interface* temp)> callback);
    static void Temp(const Module* mod, const char* interfaceSymbol, std::function<void(const Interface* temp)> callback);

    static void Destroy(Interface* ptr);

    static void* This(const Module* mod, const char* interfaceSymbol);

    template <typename T = void*>
    static T This(const Module* mod, const char* interfaceSymbol)
    {
        return (T)Interface::This(mod, interfaceSymbol);
    }
};
