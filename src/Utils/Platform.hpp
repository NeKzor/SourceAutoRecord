#pragma once

#define _GAME_PATH(x) #x

#ifdef _WIN32
#define MODULE_EXTENSION ".dll"
// clang-format off
#define GAME_PATH(x) _GAME_PATH(Games/Windows/##x.hpp)
// clang-format on
#define __rescall __thiscall
#define DLL_EXPORT extern "C" __declspec(dllexport)
#define SEEK_DIR_CUR std::ios_base::_Seekdir::_Seekcur
#define VFUNC(type, name, ...) type __fastcall name(void* thisptr, int edx, ##__VA_ARGS__)

#define DETOUR(name, ...)                                                                          \
    using _##name = int(__thiscall*)(void* thisptr, ##__VA_ARGS__);                                \
    _##name name;                                                                                  \
    int __fastcall name##_Hook(void* thisptr, int edx, ##__VA_ARGS__);                             \
    _Hook hk##name(reinterpret_cast<uintptr_t*>(&name), reinterpret_cast<uintptr_t>(name##_Hook)); \
    int __fastcall name##_Hook(void* thisptr, int edx, ##__VA_ARGS__)
#define DETOUR_T(type, name, ...)                                                                  \
    using _##name = type(__thiscall*)(void* thisptr, ##__VA_ARGS__);                               \
    _##name name;                                                                                  \
    type __fastcall name##_Hook(void* thisptr, int edx, ##__VA_ARGS__);                            \
    _Hook hk##name(reinterpret_cast<uintptr_t*>(&name), reinterpret_cast<uintptr_t>(name##_Hook)); \
    type __fastcall name##_Hook(void* thisptr, int edx, ##__VA_ARGS__)
#define DETOUR_B(name, ...)                                                                        \
    using _##name = int(__thiscall*)(void* thisptr, ##__VA_ARGS__);                                \
    _##name name;                                                                                  \
    _##name name##Base;                                                                            \
    int __fastcall name##_Hook(void* thisptr, int edx, ##__VA_ARGS__);                             \
    _Hook hk##name(reinterpret_cast<uintptr_t*>(&name), reinterpret_cast<uintptr_t>(name##_Hook)); \
    int __fastcall name##_Hook(void* thisptr, int edx, ##__VA_ARGS__)
#define DETOUR_BT(type, name, ...)                                                                 \
    using _##name = type(__thiscall*)(void* thisptr, ##__VA_ARGS__);                               \
    _##name name;                                                                                  \
    _##name name##Base;                                                                            \
    type __fastcall name##_Hook(void* thisptr, int edx, ##__VA_ARGS__);                            \
    _Hook hk##name(reinterpret_cast<uintptr_t*>(&name), reinterpret_cast<uintptr_t>(name##_Hook)); \
    type __fastcall name##_Hook(void* thisptr, int edx, ##__VA_ARGS__)
#else
#define MODULE_EXTENSION ".so"
// clang-format off
#define GAME_PATH(x) _GAME_PATH(Games/Linux/x.hpp)
// clang-format on
#define __rescall __attribute__((__cdecl__))
#define __cdecl __attribute__((__cdecl__))
#define __stdcall __attribute__((__stdcall__))
#define __fastcall __attribute__((__fastcall__))
#define DLL_EXPORT extern "C" __attribute__((visibility("default")))
#define SEEK_DIR_CUR std::ios_base::seekdir::_S_cur
#define VFUNC(type, name, ...) type __cdecl name(void* thisptr, ##__VA_ARGS__)

#define DETOUR(name, ...)                                                                          \
    using _##name = int(__cdecl*)(void* thisptr, ##__VA_ARGS__);                                   \
    _##name name;                                                                                  \
    int __cdecl name##_Hook(void* thisptr, ##__VA_ARGS__);                                         \
    _Hook hk##name(reinterpret_cast<uintptr_t*>(&name), reinterpret_cast<uintptr_t>(name##_Hook)); \
    int __cdecl name##_Hook(void* thisptr, ##__VA_ARGS__)
#define DETOUR_T(type, name, ...)                                                                  \
    using _##name = type(__cdecl*)(void* thisptr, ##__VA_ARGS__);                                  \
    _##name name;                                                                                  \
    type __cdecl name##_Hook(void* thisptr, ##__VA_ARGS__);                                        \
    _Hook hk##name(reinterpret_cast<uintptr_t*>(&name), reinterpret_cast<uintptr_t>(name##_Hook)); \
    type __cdecl name##_Hook(void* thisptr, ##__VA_ARGS__)
#define DETOUR_B(name, ...)                                                                        \
    using _##name = int(__cdecl*)(void* thisptr, ##__VA_ARGS__);                                   \
    _##name name;                                                                                  \
    _##name name##Base;                                                                            \
    int __cdecl name##_Hook(void* thisptr, ##__VA_ARGS__);                                         \
    _Hook hk##name(reinterpret_cast<uintptr_t*>(&name), reinterpret_cast<uintptr_t>(name##_Hook)); \
    int __cdecl name##_Hook(void* thisptr, ##__VA_ARGS__)
#define DETOUR_BT(type, name, ...)                                                                 \
    using _##name = type(__cdecl*)(void* thisptr, ##__VA_ARGS__);                                  \
    _##name name;                                                                                  \
    _##name name##Base;                                                                            \
    type __cdecl name##_Hook(void* thisptr, ##__VA_ARGS__);                                        \
    _Hook hk##name(reinterpret_cast<uintptr_t*>(&name), reinterpret_cast<uintptr_t>(name##_Hook)); \
    type __cdecl name##_Hook(void* thisptr, ##__VA_ARGS__)
#endif

#define DETOUR_STD(type, name, ...)                                                                \
    using _##name = type(__stdcall*)(##__VA_ARGS__);                                               \
    _##name name;                                                                                  \
    type __stdcall name##_Hook(##__VA_ARGS__);                                                     \
    _Hook hk##name(reinterpret_cast<uintptr_t*>(&name), reinterpret_cast<uintptr_t>(name##_Hook)); \
    type __stdcall name##_Hook(##__VA_ARGS__)

class _Hook {
public:
    uintptr_t* original;
    uintptr_t detour;

public:
    _Hook(uintptr_t* original, uintptr_t detour)
        : original(original)
        , detour(detour)
    {
    }
};

class _HookBase {
public:
    uintptr_t* original;
    uintptr_t* originalBase;
    uintptr_t detour;

public:
    _HookBase(uintptr_t* original, uintptr_t* originalBase, uintptr_t detour)
        : original(original)
        , originalBase(originalBase)
        , detour(detour)
    {
    }
};
