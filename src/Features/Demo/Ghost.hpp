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
    int startTick;
    int endTick;
    void* ghost_entity;
    int CMTime;
    char modelName[64];

public:
    Ghost();
    void Reset();
    void Start();
    bool isReady();
};

extern Ghost* ghost;

extern Command sar_ghost_set_demo;
extern Command sar_ghost_set_CM_time;
extern Command sar_ghost_set_prop_model;
extern Variable sar_ghost_enable;
extern Variable sar_ghost_height;