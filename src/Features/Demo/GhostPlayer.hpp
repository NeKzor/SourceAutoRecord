#pragma once

#include "GhostEntity.hpp"
#include "Modules/Engine.hpp"

class GhostPlayer : public Feature {

private:
    GhostEntity* ghost;

public:
    GhostPlayer();
    void Run();
    bool IsReady();
    void ResetGhost();
    void ResetCoord();
    GhostEntity* GetGhost();
    void SetStartTick(int);
    int GetStartTick();
    void SetCoordList(std::vector<Vector> pos, std::vector<Vector> ang);
};


extern GhostPlayer* ghostPlayer;

extern Command sar_ghost_set_demo;
extern Command sar_ghost_set_prop_model;
extern Command sar_ghost_time_offset;
extern Variable sar_ghost_enable;
extern Variable sar_ghost_height;
extern Variable sar_ghost_transparency;
