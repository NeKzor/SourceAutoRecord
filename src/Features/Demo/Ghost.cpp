#include "Features/Demo/Ghost.hpp"

#include "Features/Demo/Demo.hpp"
#include "Features/Demo/DemoParser.hpp"
#include "Modules/Engine.hpp"
#include "Modules/Server.hpp"

Ghost* ghost;

Variable sar_ghost_enable("sar_ghost_enable", 0, "Start automatically the ghost playback when loading a map.\n");
Variable sar_ghost_height("sar_ghost_height", "16", "Height of the ghost.\n");

Ghost::Ghost()
    : positionList()
    , ghost_entity(nullptr)
    , startTick()
    , CMTime(0)
    , endTick(0)
    , modelName("models/props/metal_box.mdl")
{
    this->hasLoaded = true;
}

void Ghost::Reset()
{
    ghost->ghost_entity = nullptr;
    server->inMap = false;
    server->tickCount = 0;
    console->Print("Reset.\n");
}

void Ghost::Start()
{
    ghost->ghost_entity = server->CreateEntityByName("prop_dynamic_override");
    server->SetKeyValueChar(ghost->ghost_entity, "model", this->modelName);
    server->SetKeyValueChar(ghost->ghost_entity, "origin", "0 0 0");
    server->DispatchSpawn(ghost->ghost_entity);
    console->Print("Player replay has been sucessfully spawned.\n");
    server->inMap = true;
    server->tickCount = 0;
}

bool Ghost::isReady()
{
    if (ghost->CMTime > 0 && ghost->positionList.size() > 0 && sar_ghost_enable.GetBool()) {
        return true;
    }
    return false;
}

// Commands

CON_COMMAND_AUTOCOMPLETEFILE(sar_ghost_set_demo, "Set the demo in order to build the ghost.\n", 0, 0, dem)
{
    if (args.ArgC() <= 1) {
        return console->Print(sar_ghost_set_demo.ThisPtr()->m_pszHelpString);
    }

    std::string name;
    if (args[1][0] == '\0') {
        if (engine->demoplayer->DemoName[0] != '\0') {
            name = std::string(engine->demoplayer->DemoName);
        } else {
            return console->Print("No demo was recorded or played back!\n");
        }
    } else {
        name = std::string(args[1]);
    }

    DemoParser parser;
    parser.outputMode = 3;

    Demo demo;
    auto dir = std::string(engine->GetGameDirectory()) + std::string("/") + name;
    if (parser.Parse(dir, &demo)) {
        parser.Adjust(&demo);
    } else {
        console->Print("Could not parse \"%s\"!\n", name.c_str());
    }
}
CON_COMMAND(sar_ghost_set_CM_time, "Due to lack of information from Volvo, I need final CM time. I'm crying.\n")
{
    if (args.ArgC() <= 1) {
        return console->Print(sar_ghost_set_CM_time.ThisPtr()->m_pszHelpString);
    }

    float time = static_cast<float>(std::atof(args[1])) * 60;
    int ticks = std::round(time);
    ghost->CMTime = ticks;
}
CON_COMMAND(sar_ghost_set_prop_model, "Set the prop model. Example : models/props/metal_box.mdl\n")
{
    if (args.ArgC() <= 1) {
        return console->Print(sar_ghost_set_prop_model.ThisPtr()->m_pszHelpString);
    }
    std::strncpy(ghost->modelName, args[1], sizeof(ghost->modelName));
}
