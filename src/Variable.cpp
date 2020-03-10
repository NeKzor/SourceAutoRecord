#include "Variable.hpp"

#include <cstring>

#include "Modules/Tier1.hpp"

#include "Game.hpp"
#include "Offsets.hpp"
#include "SAR.hpp"

Variable::Variable()
    : Variable(SourceGame_Unknown)
{
}
Variable::Variable(int version)
    : CommandBase(version)
    , originalFlags(0)
    , originalFnChangeCallback(nullptr)
{
}
Variable::Variable(const char* name)
    : Variable(SourceGame_Unknown)
{
    this->ptr = reinterpret_cast<ConVar*>(tier1->FindCommandBase(tier1->g_pCVar->ThisPtr(), name));
    this->isReference = true;
}
// Boolean or String
Variable::Variable(const char* name, const char* value, const char* helpstr, int flags, int version)
    : Variable(version)
{
    if (flags != 0)
        this->Create(name, value, flags, helpstr, true, 0, true, 1);
    else
        this->Create(name, value, flags, helpstr);
}
// Float
Variable::Variable(const char* name, const char* value, float min, const char* helpstr, int flags, int version)
    : Variable(version)
{
    this->Create(name, value, flags, helpstr, true, min);
}
// Float
Variable::Variable(const char* name, const char* value, float min, float max, const char* helpstr, int flags, int version)
    : Variable(version)
{
    this->Create(name, value, flags, helpstr, true, min, true, max);
}
void Variable::Create(const char* name, const char* value, int flags, const char* helpstr, bool hasmin, float min, bool hasmax, float max)
{
    this->ptr = new ConVar2(name, value, flags, helpstr, hasmin, min, hasmax, max);

    CommandBase::GetList().push_back(this);
}
void Variable::Register()
{
    if (!this->isRegistered && !this->isReference && this->ptr) {
        this->isRegistered = true;

        auto ptr = this->ThisPtr();
        ptr->ConCommandBase_VTable = tier1->ConVar_VTable;
        ptr->ConVar_VTable = tier1->ConVar_VTable2;

        tier1->Create(ptr,
            ptr->m_pszName,
            ptr->m_pszDefaultValue,
            ptr->m_nFlags,
            ptr->m_pszHelpString,
            ptr->m_bHasMin,
            ptr->m_fMinVal,
            ptr->m_bHasMax,
            ptr->m_fMaxVal,
            nullptr);
    }
}
void Variable::Unregister()
{
    if (this->isRegistered && !this->isReference && this->ptr) {
        this->isRegistered = false;
        tier1->UnregisterConCommand(tier1->g_pCVar->ThisPtr(), this->ptr);
#ifdef _WIN32
        tier1->Dtor(this->ThisPtr(), 0);
#else
        tier1->Dtor(this->ThisPtr());
#endif
        sdelete(this->ptr)
    }
}
void Variable::Unlock(bool asCheat)
{
    if (this->ptr) {
        this->originalFlags = this->ptr->m_nFlags;
        this->RemoveFlag(FCVAR_DEVELOPMENTONLY | FCVAR_HIDDEN);
        if (asCheat) {
            this->AddFlag(FCVAR_CHEAT);
        }

        if (this->isReference) {
            this->GetList().push_back(this);
        }
    }
}
void Variable::Lock()
{
    if (this->ptr) {
        this->SetFlags(this->originalFlags);
        this->SetValue(this->ThisPtr()->m_pszDefaultValue);
    }
}
void Variable::DisableChange()
{
    if (this->ptr) {
        if (sar.game->Is(SourceGame_Portal2Engine)) {
            auto ptr = this->ThisPtr2();
            this->originalSize = ptr->m_fnChangeCallback.m_Size;
            ptr->m_fnChangeCallback.m_Size = 0;
        } else if (sar.game->Is(SourceGame_HalfLife2Engine)) {
            auto ptr = ThisPtr();
            this->originalFnChangeCallback = ptr->m_fnChangeCallback;
            ptr->m_fnChangeCallback = nullptr;
        }
    }
}
void Variable::EnableChange()
{
    if (this->ptr) {
        if (sar.game->Is(SourceGame_Portal2Engine)) {
            this->ThisPtr2()->m_fnChangeCallback.m_Size = this->originalSize;
        } else if (sar.game->Is(SourceGame_HalfLife2Engine)) {
            this->ThisPtr()->m_fnChangeCallback = this->originalFnChangeCallback;
        }
    }
}
Variable Variable::FromString(const char* name, const char* value, const char* helpstr, int version)
{
    return Variable(name, value, helpstr, 0);
}
Variable Variable::FromBoolean(const char* name, const char* value, const char* helpstr, int version)
{
    return Variable(name, value, helpstr, FCVAR_NEVER_AS_STRING);
}
Variable Variable::FromFloat(const char* name, const char* value, float min, const char* helpstr, int version)
{
    return Variable(name, value, helpstr, FCVAR_NEVER_AS_STRING);
}
Variable Variable::FromFloatRange(const char* name, const char* value, float min, float max, const char* helpstr, int version)
{
    return Variable(name, value, helpstr, FCVAR_NEVER_AS_STRING);
}
