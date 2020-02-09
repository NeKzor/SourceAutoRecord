#pragma once
#include "Modules/Tier1.hpp"

#include "Utils/SDK.hpp"

class Variable {
private:
    ConVar* ptr;

    int originalFlags;

    union {
        FnChangeCallback_t originalFnChangeCallback;
        int originalSize;
    };

public:
    int version;
    bool isRegistered;
    bool isReference;

public:
    static std::vector<Variable*>& GetList();

public:
    Variable();
    ~Variable();
    Variable(const char* name);
    Variable(const char* name, const char* value, const char* helpstr, int flags = FCVAR_NEVER_AS_STRING);
    Variable(const char* name, const char* value, float min, const char* helpstr, int flags = FCVAR_NEVER_AS_STRING);
    Variable(const char* name, const char* value, float min, float max, const char* helpstr, int flags = FCVAR_NEVER_AS_STRING);

    void Create(const char* name, const char* value, int flags = 0, const char* helpstr = "", bool hasmin = false, float min = 0,
        bool hasmax = false, float max = 0);
    void Realloc();

    ConVar* ThisPtr();
    ConVar2* ThisPtr2();

    bool GetBool();
    int GetInt();
    float GetFloat();
    const char* GetString();
    const int GetFlags();

    void SetValue(const char* value);
    void SetValue(float value);
    void SetValue(int value);

    void SetFlags(int value);
    void AddFlag(int value);
    void RemoveFlag(int value);

    void Unlock(bool asCheat = true);
    void Lock();

    void DisableChange();
    void EnableChange();

    void UniqueFor(int version);
    void Register();
    void Unregister();

    bool operator!();

    static int RegisterAll();
    static void UnregisterAll();
    static Variable* Find(const char* name);
};
