#pragma once
#include "Module.hpp"

#include "Interface.hpp"
#include "Utils.hpp"

class Scheme : public Module {
public:
    Interface* g_pScheme = nullptr;

    using _GetFont = unsigned long(__rescall*)(void* thisptr, const char* fontName, bool proportional);
    _GetFont GetFont = nullptr;

public:
    unsigned long GetDefaultFont();

    Scheme()
        : Module(MODULE("vgui2"))
    {
    }

    void Init() override;
    void Shutdown() override;
};

extern Scheme* scheme;
