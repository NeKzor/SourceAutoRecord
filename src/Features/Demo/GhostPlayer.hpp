#pragma once

#include "GhostEntity.hpp"
#include "Modules/Engine.hpp"

class GhostPlayer : public Feature {

private:

public:
    bool enabled;
    bool isNetworking;
    std::vector<GhostEntity*> ghost;

public:
    GhostPlayer();
    void Run();
    bool IsReady();
    void StopAll();
    void StopByID(sf::Uint32 &ID);
    GhostEntity* GetFirstGhost();
    GhostEntity* GetGhostFromID(sf::Uint32& ID);
	void AddGhost(GhostEntity* ghost);
    void ResetGhost();
    void ResetCoord();
    void SetPosAng(sf::Uint32& ID, Vector position, Vector angle);
    void SetStartTick(int);
    int GetStartTick();
    void SetCoordList(std::vector<Vector> pos, std::vector<Vector> ang);
    bool IsNetworking();
};


extern GhostPlayer* ghostPlayer;

extern Command sar_ghost_set_demo;
extern Command sar_ghost_set_prop_model;
extern Command sar_ghost_time_offset;
extern Command sar_ghost_enable;
extern Variable sar_ghost_height;
extern Variable sar_ghost_transparency;
