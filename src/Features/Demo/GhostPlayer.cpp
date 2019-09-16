#include "Features/Demo/GhostPlayer.hpp"
#include "Features/Demo/DemoParser.hpp"

GhostPlayer* ghostPlayer;

Variable sar_ghost_height("sar_ghost_height", "16", -256, "Height of the ghost.\n");
Variable sar_ghost_transparency("sar_ghost_transparency", "255", 0, 256, "Transparency of the ghost.\n");
Variable sar_ghost_trail_lenght("sar_ghost_trail_lenght", "0", 0, "Lenght of the trail. 0 to deactivate the trail.\n");
Variable sar_ghost_trail_transparency("sar_ghost_trail_transparency", "75", 0, 256, "Transparency of the trail.\n");

GhostPlayer::GhostPlayer()
    : ghost()
    , enabled(false)
    , isNetworking(false)
{
    this->hasLoaded = true;
}

bool GhostPlayer::IsReady()
{
    for (auto it : this->ghost) {
        if (!it->IsReady()) {
            return false;
		}
    }
    if (!this->enabled) {
        return false;
	}

    return true;
}

void GhostPlayer::Run()
{
    for (auto it : this->ghost) {
        it->Think();
    }
}

void GhostPlayer::StopAll()
{
    for (auto it : this->ghost) {
        it->Stop();
    }
}

void GhostPlayer::StopByID(std::string ID)
{
    this->GetGhostFromID(ID)->Stop();
}

void GhostPlayer::ResetGhost()
{
    for (auto it : this->ghost) {
        it->Reset();
    }
}

void GhostPlayer::ResetCoord()
{
    for (auto it : this->ghost) {
        it->positionList.clear();
        it->angleList.clear();
    }
}

void GhostPlayer::SetPosAng(std::string ID, Vector position, Vector angle)
{
    this->GetGhostFromID(ID)->SetPosAng(position, angle);
}

GhostEntity* GhostPlayer::GetFirstGhost()
{
    return this->ghost[0];
}

GhostEntity* GhostPlayer::GetGhostFromID(std::string ID)
{
    for (auto it : this->ghost) {
        if (it->ID == ID) {
            return it;
        }
    }
    return nullptr;
}

void GhostPlayer::AddGhost(GhostEntity* ghost)
{
    this->ghost.push_back(ghost);
}

int GhostPlayer::GetStartTick()
{
    return this->GetFirstGhost()->startTick;
}

void GhostPlayer::SetStartTick(int startTick)
{
    this->GetFirstGhost()->startTick = startTick;
}

void GhostPlayer::SetCoordList(std::vector<Vector> pos, std::vector<Vector> ang)
{
    this->GetFirstGhost()->positionList = pos;
    this->GetFirstGhost()->angleList = ang;
}

bool GhostPlayer::IsNetworking()
{
    return this->isNetworking;
}

// Commands

CON_COMMAND_AUTOCOMPLETEFILE(sar_ghost_set_demo, "Set the demo in order to build the ghost.\n", 0, 0, dem)
{
    if (args.ArgC() <= 1) {
        return console->Print(sar_ghost_set_demo.ThisPtr()->m_pszHelpString);
    }
    if (ghostPlayer->ghost.empty()) {
        return console->Print("sar_ghost_enable must be enabled before setting the demo.\n");
    }
    if (ghostPlayer->IsNetworking()) {
        return console->Warning("Can't play ghost with demos when connected to a server !\n");
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
        ghostPlayer->GetFirstGhost()->SetCMTime(demo.playbackTime);
        ghostPlayer->GetFirstGhost()->demo = demo;
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
    if (ghostPlayer->ghost.empty()) {
        return console->Print("sar_ghost_enable must be enabled before setting the demo.\n");
    }

    ghostPlayer->GetFirstGhost()->ChangeModel(args[1]);
    ghostPlayer->ResetGhost();
    ghostPlayer->GetFirstGhost()->Spawn(false, true);
}

CON_COMMAND(sar_ghost_time_offset, "In seconds. Start the ghost with a delay. Can be negative of positive.\n")
{
    if (args.ArgC() <= 1) {
        return console->Print(sar_ghost_time_offset.ThisPtr()->m_pszHelpString);
    }
    if (ghostPlayer->ghost.empty()) {
        return console->Print("sar_ghost_enable must be enabled before setting the demo.\n");
    }

    float delay = static_cast<float>(std::atof(args[1]));
    GhostEntity* ghost = ghostPlayer->GetFirstGhost();

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
        //ghostPlayer->AddGhost(new GhostEntity);
    } else {
        ghostPlayer->enabled = false;
        ghostPlayer->StopAll();
    }
}
