#pragma once
#include <vector>

#include "Interface.hpp"

#define ENTPROP(name, type, offset) \
    inline type name(void* entity) { return *reinterpret_cast<type*>((uintptr_t)entity + Offsets::offset); }

class Interface;
class CommandHook;

class Module {
private:
    bool isLoaded = false;

protected:
    bool isHookable = false;

public:
    const char* filename = nullptr;
    std::vector<Interface*> interfaces = std::vector<Interface*>();
    std::vector<CommandHook*> cmdHooks = std::vector<CommandHook*>();

public:
    Module(const char* filename);
    inline bool Loaded() { return this->isLoaded; }
    inline bool CanHook() { return this->isHookable; }
    virtual ~Module() = default;
    virtual void Init() = 0;
    virtual void Shutdown() = 0;
    virtual bool Load() final;
    virtual bool Unload() final;
};

class Modules {
public:
    std::vector<Module*> list;

public:
    Modules();
    template <typename T = Module>
    void AddModule(T** modulePtr)
    {
        *modulePtr = new T();
        this->list.push_back(*modulePtr);
    }
    template <typename T = Module>
    void RemoveModule(T** modulePtr)
    {
        this->list.erase(*modulePtr);
    }
    void LoadAll();
    void UnloadAll();
    ~Modules();
};
