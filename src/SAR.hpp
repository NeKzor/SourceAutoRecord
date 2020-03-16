#pragma once
#include <thread>

#include "Features/Feature.hpp"

#include "Modules/Console.hpp"
#include "Modules/Module.hpp"

#include "Cheats.hpp"
#include "Command.hpp"
#include "Game.hpp"
#include "Interface.hpp"
#include "Plugin.hpp"
#include "Variable.hpp"

#define SAR_VERSION "1.11"
#define SAR_BUILD __TIME__ " " __DATE__
#define SAR_WEB "https://nekzor.github.io/SourceAutoRecord"

#define SAFE_UNLOAD_TICK_DELAY 33

class SAR {
private:
    void* vtable;

public:
    Modules* modules;
    Features* features;
    Cheats* cheats;
    Plugin* plugin;
    Game* game;

public:
    SAR();

    inline const char* Version() { return SAR_VERSION; }
    inline const char* Build() { return SAR_BUILD; }
    inline const char* Website() { return SAR_WEB; }

    bool Init();
    void HookAll();
    void UnhookAll();
    void Cleanup();
    bool GetPlugin();

    static void* pluginVtable[20];
    static void* pluginVtable2[21];
};

extern SAR sar;
