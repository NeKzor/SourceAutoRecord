#include "Features/Demo/GhostPlayer.hpp"
#include "Features/Demo/DemoParser.hpp"

GhostPlayer* ghostPlayer;

Variable sar_ghost_height("sar_ghost_height", "16", -256, "Height of the ghost.\n");
Variable sar_ghost_transparency("sar_ghost_transparency", "255", 0, 256, "Transparency of the ghost.\n");

GhostPlayer::GhostPlayer()
    : ghost()
    , enabled(false)
{
    this->hasLoaded = true;
    this->ghost = new GhostEntity();
}

bool GhostPlayer::IsReady()
{
    return this->ghost->IsReady();
}

void GhostPlayer::Run()
{
    this->ghost->Think();
}

void GhostPlayer::Stop()
{
    this->ghost->Stop();
}

void GhostPlayer::ResetGhost()
{
    this->ghost->Reset();
}

void GhostPlayer::ResetCoord()
{
    this->ghost->positionList.clear();
    this->ghost->angleList.clear();
}

GhostEntity* GhostPlayer::GetGhost()
{
    return this->ghost;
}

int GhostPlayer::GetStartTick()
{
    return this->ghost->startTick;
}

void GhostPlayer::SetStartTick(int startTick)
{
    this->ghost->startTick = startTick;
}

void GhostPlayer::SetCoordList(std::vector<Vector> pos, std::vector<Vector> ang)
{
    this->ghost->positionList = pos;
    this->ghost->angleList = ang;
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
        ghostPlayer->GetGhost()->SetCMTime(demo.playbackTime);
        ghostPlayer->GetGhost()->demo = demo;
        console->Print("Ghost sucessfully created ! Final time of the ghost : %f\n", demo.playbackTime);
    } else {
        console->Print("Could not parse \"%s\"!\n", name.c_str());
    }
}

CON_COMMAND(sar_ghost_set_prop_model, "Set the prop model. Example : models/props/metal_box.mdl\n")
{
    if (args.ArgC() <= 1) {
        return console->Print(sar_ghost_set_prop_model.ThisPtr()->m_pszHelpString);
    }
    std::strncpy(ghostPlayer->GetGhost()->modelName, args[1], sizeof(ghostPlayer->GetGhost()->modelName));
}

CON_COMMAND(sar_ghost_time_offset, "In seconds. Start the ghost with a delay. Can be negative of positive.\n")
{
    if (args.ArgC() <= 1) {
        return console->Print(sar_ghost_time_offset.ThisPtr()->m_pszHelpString);
    }
    float delay = static_cast<float>(std::atof(args[1]));
    GhostEntity* ghost = ghostPlayer->GetGhost();

    if (delay < 0) { //Asking for faster ghost
        if (delay * 60 > ghost->positionList.size()) { //Too fast
            console->Print("Time offset is too low.\n");
            return;
        } else { //Ok
            ghost->SetStartDelay(-delay * 60); //Seconds -> ticks ; TODO : Check if this works properly in Coop
            console->Print("Final time of the ghost : %f\n", ghost->demo.playbackTime + delay);
        }
    } else if (delay > 0) { //Asking for slower ghost
        ghost->SetStartDelay(0);
        ghost->SetCMTime(ghost->demo.playbackTime + delay);
        console->Print("Final time of the ghost : %f\n", ghost->demo.playbackTime + delay);
    }
}

CON_COMMAND(sar_ghost_enable, "Start automatically the ghost playback when loading a map.\n")
{
    if (args.ArgC() <= 1) {
        return console->Print(sar_ghost_time_offset.ThisPtr()->m_pszHelpString);
    }
    bool enable = static_cast<bool>(std::atoi(args[1]));
    if (enable) {
        ghostPlayer->enabled = true;
    } else {
        ghostPlayer->enabled = false;
        ghostPlayer->Stop();
	}
}