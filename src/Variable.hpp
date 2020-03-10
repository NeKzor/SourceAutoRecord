#pragma once
#include "Modules/Tier1.hpp"

#include "Utils/SDK.hpp"

#include "Command.hpp"
#include "Game.hpp"
#include "Offsets.hpp"

class Variable : public CommandBase {
private:
    int originalFlags;

    union {
        FnChangeCallback_t originalFnChangeCallback;
        int originalSize;
    };

public:
    Variable();
    Variable(int version);
    Variable(const char* name);
    Variable(const char* name, const char* value, const char* helpstr, int flags = FCVAR_NEVER_AS_STRING,
        int version = SourceGame_Unknown);
    Variable(const char* name, const char* value, float min, const char* helpstr, int flags = FCVAR_NEVER_AS_STRING,
        int version = SourceGame_Unknown);
    Variable(const char* name, const char* value, float min, float max, const char* helpstr, int flags = FCVAR_NEVER_AS_STRING,
        int version = SourceGame_Unknown);

    void Create(const char* name, const char* value, int flags = 0, const char* helpstr = "", bool hasmin = false, float min = 0,
        bool hasmax = false, float max = 0);

    inline bool IsCommand() override { return false; }
    void Register() override;
    void Unregister() override;

    inline ConVar* ThisPtr() { return reinterpret_cast<ConVar*>(this->ptr); }
    inline ConVar2* ThisPtr2() { return reinterpret_cast<ConVar2*>(this->ptr); }

    inline bool GetBool() { return !!GetInt(); }
    inline int GetInt() { return this->ThisPtr()->m_nValue; }
    inline float GetFloat() { return this->ThisPtr()->m_fValue; }
    inline const char* GetString() { return this->ThisPtr()->m_pszString; }
    inline const int GetFlags() { return this->ThisPtr()->m_nFlags; }

    inline void SetValue(const char* value)
    {
        Memory::VMT<_InternalSetValue>(this->ptr, Offsets::InternalSetValue)(this->ptr, value);
    }
    inline void SetValue(float value)
    {
        Memory::VMT<_InternalSetFloatValue>(this->ptr, Offsets::InternalSetFloatValue)(this->ptr, value);
    }
    inline void SetValue(int value)
    {
        Memory::VMT<_InternalSetIntValue>(this->ptr, Offsets::InternalSetIntValue)(this->ptr, value);
    }

    inline void SetFlags(int value) { this->ptr->m_nFlags = value; }
    inline void AddFlag(int value) { this->SetFlags(this->GetFlags() | value); }
    inline void RemoveFlag(int value) { this->SetFlags(this->GetFlags() & ~(value)); }

    void Unlock(bool asCheat = true);
    void Lock();

    void DisableChange();
    void EnableChange();

    static Variable FromString(const char* name, const char* value, const char* helpstr,
        int version = SourceGame_Unknown);
    static Variable FromBoolean(const char* name, const char* value, const char* helpstr,
        int version = SourceGame_Unknown);
    static Variable FromFloat(const char* name, const char* value, float min, const char* helpstr,
        int version = SourceGame_Unknown);
    static Variable FromFloatRange(const char* name, const char* value, float min, float max, const char* helpstr,
        int version = SourceGame_Unknown);
};
