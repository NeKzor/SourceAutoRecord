#include "Features/Demo/Ghost.hpp"

#include "Features/Demo/Demo.hpp"
#include "Features/Demo/DemoParser.hpp"
#include "Features/Session.hpp"
#include "Modules/Engine.hpp"
#include "Modules/Server.hpp"

Ghost* ghost;

Variable sar_ghost_enable("sar_ghost_enable", 0, "Start automatically the ghost playback when loading a map.\n");
Variable sar_ghost_height("sar_ghost_height", "16", -256, "Height of the ghost.\n");
Variable sar_ghost_transparency("sar_ghost_transparency", "255", 0, 256, "Transparency of the ghost.\n");

Ghost::Ghost()
    : positionList()
    , angleList()
    , ghost_entity(nullptr)
    , startTick()
    , CMTime(0)
    , endTick(0)
    , modelName("models/props/food_can/food_can_open.mdl")
    , isPlaying(false)
    , mapSpawning(false)
    , inMap(false)
    , tickCount(0)
{
    this->hasLoaded = true;
}

void Ghost::Reset()
{
    this->ghost_entity = nullptr;
    this->isPlaying = false;
    this->tickCount = 0;
    console->Print("Ghost reset.\n");
}

void Ghost::Start()
{
    this->ghost_entity = server->CreateEntityByName("prop_dynamic_override");
    server->SetKeyValueChar(this->ghost_entity, "model", this->modelName);
    server->SetKeyValueChar(this->ghost_entity, "origin", "0 0 0");
    server->SetKeyValueChar(this->ghost_entity, "angles", "0 0 0");

    if (sar_ghost_transparency.GetFloat() <= 254) {
        server->SetKeyValueChar(this->ghost_entity, "rendermode", "1");
        server->SetKeyValueFloat(this->ghost_entity, "renderamt", sar_ghost_transparency.GetFloat());
    } else {
        server->SetKeyValueChar(this->ghost_entity, "rendermode", "0");
    }

    server->DispatchSpawn(this->ghost_entity);
    this->isPlaying = true;
    this->tickCount = 0;

    if (this->ghost_entity != nullptr) {
        console->Print("Say 'Hi !' to Jonnil !\n");
    }
}

bool Ghost::IsReady()
{
    if (this->CMTime > 0 && this->positionList.size() > 0 && sar_ghost_enable.GetBool()) {
        return true;
    }
    return false;
}

void Ghost::SetCMTime(float playbackTime)
{
    float time = playbackTime * 60;
    int ticks = std::round(time);
    this->CMTime = ticks;
    console->Print("Final time of the demo : %f\n", playbackTime);
}

void Ghost::Think()
{
    auto tick = session->GetTick();
    if ((engine->GetMaxClients() == 1 && tick == (this->startTick + (this->CMTime - this->endTick))) || (engine->GetMaxClients() > 1 && tick == this->startTick)) {
        this->Start();
    }

    if (this->isPlaying) {
        Vector position = this->positionList.at(this->tickCount);
        position.z += sar_ghost_height.GetFloat();
        server->SetKeyValueVector(this->ghost_entity, "origin", position);
        server->SetKeyValueVector(this->ghost_entity, "angles", this->angleList.at(this->tickCount));

        if (engine->GetMaxClients() == 1) {
            if (tick % 2 == 0) {
                this->tickCount++;
            }
        } else {
            this->tickCount++;
        }
    }
    if (this->tickCount == this->positionList.size()) {
        console->Print("Ghost has finished.\n");
        this->Reset();
    }
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
        ghost->endTick = demo.playbackTicks;
        ghost->SetCMTime(demo.playbackTime);
        console->Print("Ghost sucessfully created !\n");
    } else {
        console->Print("Could not parse \"%s\"!\n", name.c_str());
    }
}

CON_COMMAND(sar_ghost_set_prop_model, "Set the prop model. Example : models/props/metal_box.mdl\n")
{
    if (args.ArgC() <= 1) {
        return console->Print(sar_ghost_set_prop_model.ThisPtr()->m_pszHelpString);
    }
    std::strncpy(ghost->modelName, args[1], sizeof(ghost->modelName));
}
