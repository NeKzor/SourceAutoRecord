#include "SAR.hpp"

#include <cstring>

#include "Features.hpp"
#include "Modules.hpp"

#include "Cheats.hpp"
#include "Command.hpp"
#include "Game.hpp"
#include "Interface.hpp"
#include "Variable.hpp"

SAR sar;
InterfaceReg __sar([]() -> void* { return &sar; }, INTERFACEVERSION_ISERVERPLUGINCALLBACKS);

SAR::SAR()
    : vtable(pluginVtable)
    , modules(new Modules())
    , features(new Features())
    , cheats(new Cheats())
    , plugin(new Plugin())
    , game(Game::CreateNew())
{
}
bool SAR::Init()
{
    console = new Console();
    if (!console->Load()) {
        return false;
    }

    if (!this->game) {
        console->Warning("SAR: Game not supported!\n");
        return false;
    }

    if (game->Is(SourceGame_Portal2Engine)) {
        this->vtable = pluginVtable2;
    }

    this->game->LoadOffsets();

    try {
        tier1 = new Tier1();
        tier1->Load();

        this->features->AddFeature<>(&config);
        this->features->AddFeature<>(&cvars);
        this->features->AddFeature<>(&rebinder);
        this->game->Is(SourceGame_INFRA)
            ? this->features->AddFeature<>(reinterpret_cast<InfraSession**>(&session))
            : this->features->AddFeature<>(&session);
        this->features->AddFeature<>(&stepCounter);
        this->features->AddFeature<>(&summary);
        this->features->AddFeature<>(&teleporter);
        this->features->AddFeature<>(&tracer);
        this->features->AddFeature<>(&speedrun);
        this->features->AddFeature<>(&stats);
        this->features->AddFeature<>(&cmdQueuer);
        this->features->AddFeature<>(&replayRecorder1);
        this->features->AddFeature<>(&replayRecorder2);
        this->features->AddFeature<>(&replayPlayer1);
        this->features->AddFeature<>(&replayPlayer2);
        this->features->AddFeature<>(&replayProvider);
        this->features->AddFeature<>(&timer);
        this->features->AddFeature<>(&inspector);
        this->features->AddFeature<>(&classDumper);
        this->features->AddFeature<>(&entityList);
        this->features->AddFeature<>(&offsetFinder);
        this->features->AddFeature<>(&autoStrafer);
        this->features->AddFeature<>(&pauseTimer);
        this->features->AddFeature<>(&dataMapDumper);
        this->features->AddFeature<>(&fovChanger);

        this->modules->AddModule<>(&inputSystem);
        this->modules->AddModule<>(&scheme);
        this->modules->AddModule<>(&surface);
        this->modules->AddModule<>(&vgui);
        this->modules->AddModule<>(&engine);
        this->modules->AddModule<>(&demoplayer);
        this->modules->AddModule<>(&demorecorder);
        this->modules->AddModule<>(&client);
        this->modules->AddModule<>(&server);

        this->modules->LoadAll();

        this->cheats->Init();

        this->features->AddFeature<>(&tasTools);

        if (this->game->Is(SourceGame_Portal2 | SourceGame_ApertureTag)) {
            this->features->AddFeature<>(&listener);
            this->features->AddFeature<>(&workshop);
            this->features->AddFeature<>(&imitator);
        }

        if (listener) {
            listener->Init();
        }

        speedrun->LoadRules(this->game);

        config->Load();

        this->HookAll();

        console->PrintActive("Loaded SourceAutoRecord, Version %s\n", SAR_VERSION);
    } catch (std::exception& ex) {
        console->Warning("SAR: %s\n", ex.what());
        return false;
    }

    return true;
}
void SAR::HookAll()
{
    for (auto& mod : this->modules->list) {
        if (mod->CanHook()) {
            for (auto& interfaceHook : mod->interfaces) {
                interfaceHook->EnableHooks();
            }

            for (auto& cmdHook : mod->cmdHooks) {
                cmdHook->Hook();
            }
        } else if (!mod->interfaces.empty()) {
            console->Warning("SAR: Unable to hook functions in non-hookable module %s\n", mod->filename);
        }
    }
}
void SAR::UnhookAll()
{
    for (auto& mod : this->modules->list) {
        for (auto& interfaceHook : mod->interfaces) {
            interfaceHook->DisableHooks();
        }

        for (auto& cmdHook : mod->cmdHooks) {
            cmdHook->Unhook();
        }
    }
}
void SAR::Cleanup()
{
    if (this->modules) {
        this->UnhookAll();
    }
    if (this->cheats) {
        this->cheats->Shutdown();
    }
    if (this->features) {
        this->features->DeleteAll();
    }
    if (this->modules) {
        this->modules->UnloadAll();
    }

    sdelete(this->features);
    sdelete(this->cheats);
    sdelete(this->modules);
    sdelete(this->plugin);
    sdelete(this->game);
    sdelete(tier1);
    sdelete(console);
}
bool SAR::GetPlugin()
{
    auto s_ServerPlugin = reinterpret_cast<uintptr_t>(engine->s_ServerPlugin->ThisPtr());
    auto m_Size = *reinterpret_cast<int*>(s_ServerPlugin + CServerPlugin_m_Size);
    if (m_Size > 0) {
        auto m_Plugins = *reinterpret_cast<uintptr_t*>(s_ServerPlugin + CServerPlugin_m_Plugins);
        for (auto i = 0; i < m_Size; ++i) {
            auto ptr = *reinterpret_cast<CPlugin**>(m_Plugins + sizeof(uintptr_t) * i);
            if (!std::strcmp(ptr->m_szName, SAR_PLUGIN_SIGNATURE)) {
                this->plugin->ptr = ptr;
                this->plugin->index = i;
                return true;
            }
        }
    }
    return false;
}

CON_COMMAND(sar_session, "Prints the current tick of the server since it has loaded.\n")
{
    auto tick = session->GetTick();
    console->Print("Session Tick: %i (%.3f)\n", tick, engine->ToTime(tick));
    if (*demorecorder->m_bRecording) {
        tick = demorecorder->GetTick();
        console->Print("Demo Recorder Tick: %i (%.3f)\n", tick, engine->ToTime(tick));
    }
    if (demoplayer->IsPlaying()) {
        tick = demoplayer->GetTick();
        console->Print("Demo Player Tick: %i (%.3f)\n", tick, engine->ToTime(tick));
    }
}
CON_COMMAND(sar_about, "Prints info about SAR plugin.\n")
{
    console->Print("SourceAutoRecord is a speedrun plugin for Source Engine games.\n");
    console->Print("More information at: %s\n", sar.Website());
    console->Print("Game: %s\n", sar.game->Version());
    console->Print("Version: %s\n", sar.Version());
    console->Print("Build: %s\n", sar.Build());
}
CON_COMMAND(sar_cvars_save, "Saves important SAR cvars.\n")
{
    if (!config->Save()) {
        console->Print("Failed to create config file!\n");
    } else {
        console->Print("Saved important settings to cfg/_sar_cvars.cfg!\n");
    }
}
CON_COMMAND(sar_cvars_load, "Loads important SAR cvars.\n")
{
    if (!config->Load()) {
        console->Print("Config file not found!\n");
    }
}
CON_COMMAND(sar_cvars_dump, "Dumps all cvars to a file.\n")
{
    std::ofstream file("game.cvars", std::ios::out | std::ios::trunc | std::ios::binary);
    auto result = cvars->Dump(file);
    file.close();

    console->Print("Dumped %i cvars to game.cvars!\n", result);
}
CON_COMMAND(sar_cvars_dump_doc, "Dumps all SAR cvars to a file.\n")
{
    std::ofstream file("sar.cvars", std::ios::out | std::ios::trunc | std::ios::binary);
    auto result = cvars->DumpDoc(file);
    file.close();

    console->Print("Dumped %i cvars to sar.cvars!\n", result);
}
CON_COMMAND(sar_cvars_lock, "Restores default flags of unlocked cvars.\n")
{
    cvars->Lock();
}
CON_COMMAND(sar_cvars_unlock, "Unlocks all special cvars.\n")
{
    cvars->Unlock();
}
CON_COMMAND(sar_cvarlist, "Lists all SAR cvars and unlocked engine cvars.\n")
{
    cvars->ListAll();
}
CON_COMMAND(sar_rename, "Changes your name. Usage: sar_rename <name>\n")
{
    if (args.ArgC() != 2) {
        return console->Print(sar_rename.ThisPtr()->m_pszHelpString);
    }

    auto name = Variable("name");
    if (!!name) {
        name.DisableChange();
        name.SetValue(args[1]);
        name.EnableChange();
    }
}
CON_COMMAND(sar_exit, "Removes all function hooks, registered commands and unloads the module.\n")
{
    sar.UnhookAll();

    if (sar.cheats) {
        sar.cheats->Shutdown();
    }
    if (sar.features) {
        sar.features->DeleteAll();
    }

    if (sar.GetPlugin()) {
        // SAR has to unhook CEngine some ticks before unloading the module
        auto unload = std::string("plugin_unload ") + std::to_string(sar.plugin->index);
        engine->SendToCommandBuffer(unload.c_str(), SAFE_UNLOAD_TICK_DELAY);
    }

    if (sar.modules) {
        sar.modules->UnloadAll();
    }

    sdelete(sar.features);
    sdelete(sar.cheats);
    sdelete(sar.modules);
    sdelete(sar.plugin);
    sdelete(sar.game);

    console->Print("Cya :)\n");

    sdelete(tier1);
    sdelete(console);
}

VFUNC(bool, Load, CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory)
{
    return sar.Init();
}
VFUNC(void, Unload)
{
    sar.Cleanup();
}
VFUNC(void, Pause)
{
}
VFUNC(void, UnPause)
{
}
VFUNC(const char*, GetPluginDescription)
{
    return SAR_PLUGIN_SIGNATURE;
}
VFUNC(void, LevelInit, char const* pMapName)
{
}
VFUNC(void, ServerActivate, void* pEdictList, int edictCount, int clientMax)
{
}
VFUNC(void, GameFrame, bool simulating)
{
}
VFUNC(void, LevelShutdown)
{
}
VFUNC(void, ClientFullyConnect, void* pEdict)
{
}
VFUNC(void, ClientActive, void* pEntity)
{
}
VFUNC(void, ClientDisconnect, void* pEntity)
{
}
VFUNC(void, ClientPutInServer, void* pEntity, char const* playername)
{
}
VFUNC(void, SetCommandClient, int index)
{
}
VFUNC(void, ClientSettingsChanged, void* pEdict)
{
}
VFUNC(int, ClientConnect, bool* bAllowConnect, void* pEntity, const char* pszName, const char* pszAddress, char* reject, int maxrejectlen)
{
    return 0;
}
VFUNC(int, ClientCommand, void* pEntity, const void*& args)
{
    return 0;
}
VFUNC(int, NetworkIDValidated, const char* pszUserName, const char* pszNetworkID)
{
    return 0;
}
VFUNC(void, OnQueryCvarValueFinished, int iCookie, void* pPlayerEntity, int eStatus, const char* pCvarName, const char* pCvarValue)
{
}
VFUNC(void, OnEdictAllocated, void* edict)
{
}
VFUNC(void, OnEdictFreed, const void* edict)
{
}

void* SAR::pluginVtable[20] = {
    Load,
    Unload,
    Pause,
    UnPause,
    GetPluginDescription,
    LevelInit,
    ServerActivate,
    GameFrame,
    LevelShutdown,
    ClientActive,
    ClientDisconnect,
    ClientPutInServer,
    SetCommandClient,
    ClientSettingsChanged,
    ClientConnect,
    ClientCommand,
    NetworkIDValidated,
    OnQueryCvarValueFinished,
    OnEdictAllocated,
    OnEdictFreed,
};

void* SAR::pluginVtable2[21] = {
    Load,
    Unload,
    Pause,
    UnPause,
    GetPluginDescription,
    LevelInit,
    ServerActivate,
    GameFrame,
    LevelShutdown,
    ClientFullyConnect,
    ClientActive,
    ClientDisconnect,
    ClientPutInServer,
    SetCommandClient,
    ClientSettingsChanged,
    ClientConnect,
    ClientCommand,
    NetworkIDValidated,
    OnQueryCvarValueFinished,
    OnEdictAllocated,
    OnEdictFreed,
};
