#include "Features/Demo/Ghost.hpp"

#include "Features/Demo/Demo.hpp"
#include "Features/Demo/DemoParser.hpp"
#include "Modules/Engine.hpp"
#include "Modules/Server.hpp"

Ghost* ghost;

Variable sar_ghost_autostart("sar_ghost_autostart", 0, "Start automatically the ghost playback when loading a map.\n");

Ghost::Ghost()
    : positionList()
    , isReady(false)
    , ghost_entity(nullptr)
    , startTick()
    , CMTime(0)
    , endTick(0)
{
    this->hasLoaded = true;
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
        ghost->isReady = true;
    } else {
        console->Print("Could not parse \"%s\"!\n", name.c_str());
        ghost->isReady = false;
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