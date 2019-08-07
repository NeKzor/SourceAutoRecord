#pragma once
#include <vector>

#include "Features/Feature.hpp"

#include "Utils/SDK.hpp"
#include "Command.hpp"
#include "Variable.hpp"

#include <vector>

class Ghost : public Feature {

private:

public:
    std::vector<QAngle> positionList;
    std::vector<QAngle> angleList;
    int startTick;
    int endTick;
    void* ghost_entity;
    int CMTime;
    char modelName[64];
    bool isPlaying;

public:
    Ghost();
    void Reset();
    void Start();
    bool IsReady();
};

extern Ghost* ghost;

extern Command sar_ghost_set_demo;
extern Command sar_ghost_set_CM_time;
extern Command sar_ghost_set_prop_model;
extern Variable sar_ghost_enable;
extern Variable sar_ghost_height;
extern Variable sar_ghost_transparency;