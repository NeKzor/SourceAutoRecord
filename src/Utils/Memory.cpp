#include "Memory.hpp"

#include <cstring>
#include <memory>
#include <vector>

#ifdef _WIN32
#include <tchar.h>
#include <windows.h>
// Last
#include <psapi.h>
#else
#include <cstdint>
#include <dlfcn.h>
#include <link.h>
#include <sys/uio.h>
#include <unistd.h>
#endif

#define INRANGE(x, a, b) (x >= a && x <= b)
#define getBits(x) (INRANGE((x & (~0x20)), 'A', 'F') ? ((x & (~0x20)) - 'A' + 0xA) : (INRANGE(x, '0', '9') ? x - '0' : 0))
#define getByte(x) (getBits(x[0]) << 4 | getBits(x[1]))

bool Memory::TryGetModule(const char* moduleName, Memory::ModuleInfo* info)
{
    static auto moduleList = std::vector<Memory::ModuleInfo>();

    if (moduleList.empty()) {
#ifdef _WIN32
        HMODULE hMods[1024];
        HANDLE pHandle = GetCurrentProcess();
        DWORD cbNeeded;
        if (EnumProcessModules(pHandle, hMods, sizeof(hMods), &cbNeeded)) {
            for (unsigned i = 0; i < (cbNeeded / sizeof(HMODULE)); ++i) {
                char buffer[MAX_PATH];
                if (!GetModuleFileName(hMods[i], buffer, sizeof(buffer))) {
                    continue;
                }

                auto modinfo = MODULEINFO();
                if (!GetModuleInformation(pHandle, hMods[i], &modinfo, sizeof(modinfo))) {
                    continue;
                }

                auto mod = ModuleInfo();

                auto temp = std::string(buffer);
                auto index = temp.find_last_of("\\/");
                temp = temp.substr(index + 1, temp.length() - index);

                std::snprintf(mod.name, sizeof(mod.name), "%s", temp.c_str());
                mod.base = (uintptr_t)modinfo.lpBaseOfDll;
                mod.size = (uintptr_t)modinfo.SizeOfImage;
                std::snprintf(mod.path, sizeof(mod.path), "%s", buffer);

                moduleList.push_back(mod);
            }
        }

#else
        dl_iterate_phdr([](struct dl_phdr_info* info, size_t, void*) {
            auto mod = Memory::ModuleInfo();

            auto temp = std::string(info->dlpi_name);
            auto index = temp.find_last_of("\\/");
            temp = temp.substr(index + 1, temp.length() - index);
            std::snprintf(mod.name, sizeof(mod.name), "%s", temp.c_str());

            mod.base = info->dlpi_addr + info->dlpi_phdr[0].p_paddr;
            mod.size = info->dlpi_phdr[0].p_memsz;
            std::strncpy(mod.path, info->dlpi_name, sizeof(mod.path));

            moduleList.push_back(mod);
            return 0;
        },
            nullptr);
#endif
    }

    for (Memory::ModuleInfo& item : moduleList) {
        if (!std::strcmp(item.name, moduleName)) {
            if (info) {
                *info = item;
            }
            return true;
        }
    }

    return false;
}
const char* Memory::GetModulePath(const char* moduleName)
{
    auto info = Memory::ModuleInfo();
    return (Memory::TryGetModule(moduleName, &info)) ? std::string(info.path).c_str() : nullptr;
}
void* Memory::GetModuleHandleByName(const char* moduleName)
{
    auto info = Memory::ModuleInfo();
#ifdef _WIN32
    return (Memory::TryGetModule(moduleName, &info)) ? GetModuleHandleA(info.path) : nullptr;
#else
    return (TryGetModule(moduleName, &info)) ? dlopen(info.path, RTLD_NOLOAD | RTLD_NOW) : nullptr;
#endif
}
void Memory::CloseModuleHandle(void* moduleHandle)
{
#ifndef _WIN32
    dlclose(moduleHandle);
#endif
}
std::string Memory::GetProcessName()
{
#ifdef _WIN32
    char temp[MAX_PATH];
    GetModuleFileName(NULL, temp, sizeof(temp));
#else
    char link[32];
    char temp[MAX_PATH] = { 0 };
    std::sprintf(link, "/proc/%d/exe", getpid());
    readlink(link, temp, sizeof(temp));
#endif

    auto proc = std::string(temp);
    auto index = proc.find_last_of("\\/");
    proc = proc.substr(index + 1, proc.length() - index);

    return proc;
}

uintptr_t Memory::FindAddress(const uintptr_t start, const uintptr_t end, const char* target)
{
    const char* pattern = target;
    uintptr_t result = 0;

    for (auto position = start; position < end; ++position) {
        if (!*pattern)
            return result;

        auto match = *reinterpret_cast<const uint8_t*>(pattern);
        auto byte = *reinterpret_cast<const uint8_t*>(position);

        if (match == '\?' || byte == getByte(pattern)) {
            if (!result)
                result = position;

            if (!pattern[2])
                return result;

            pattern += (match != '\?') ? 3 : 2;
        } else {
            pattern = target;
            result = 0;
        }
    }
    return 0;
}
uintptr_t Memory::Scan(const char* moduleName, const char* pattern, int offset)
{
    uintptr_t result = 0;

    auto info = Memory::ModuleInfo();
    if (Memory::TryGetModule(moduleName, &info)) {
        auto start = uintptr_t(info.base);
        auto end = start + info.size;
        result = Memory::FindAddress(start, end, pattern);
        if (result) {
            result += offset;
        }
    }
    return result;
}
std::vector<uintptr_t> Memory::MultiScan(const char* moduleName, const char* pattern, int offset)
{
    std::vector<uintptr_t> result;
    auto length = std::strlen(pattern);

    auto info = Memory::ModuleInfo();
    if (Memory::TryGetModule(moduleName, &info)) {
        auto start = uintptr_t(info.base);
        auto end = start + info.size;
        auto addr = uintptr_t();
        while (true) {
            auto addr = Memory::FindAddress(start, end, pattern);
            if (addr) {
                result.push_back(addr + offset);
                start = addr + length;
            } else {
                break;
            }
        }
    }
    return result;
}

std::vector<uintptr_t> Memory::Scan(const char* moduleName, const Memory::Pattern* pattern)
{
    std::vector<uintptr_t> result;

    auto info = Memory::ModuleInfo();
    if (Memory::TryGetModule(moduleName, &info)) {
        auto start = uintptr_t(info.base);
        auto end = start + info.size;
        auto addr = Memory::FindAddress(start, end, pattern->signature);
        if (addr) {
            for (auto const& offset : pattern->offsets) {
                result.push_back(addr + offset);
            }
        }
    }
    return result;
}
std::vector<std::vector<uintptr_t>> Memory::MultiScan(const char* moduleName, const Memory::Patterns* patterns)
{
    auto results = std::vector<std::vector<uintptr_t>>();

    auto info = Memory::ModuleInfo();
    if (Memory::TryGetModule(moduleName, &info)) {
        auto moduleStart = uintptr_t(info.base);
        auto moduleEnd = moduleStart + info.size;

        for (const auto& pattern : *patterns) {
            auto length = std::strlen(pattern->signature);
            auto start = moduleStart;

            while (true) {
                auto addr = Memory::FindAddress(start, moduleEnd, pattern->signature);
                if (addr) {
                    auto result = std::vector<uintptr_t>();
                    for (const auto& offset : pattern->offsets) {
                        result.push_back(addr + offset);
                    }
                    results.push_back(result);
                    start = addr + length;
                } else {
                    break;
                }
            }
        }
    }
    return results;
}
