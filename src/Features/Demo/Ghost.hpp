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
    bool isReady;
    void* ghost_entity;
    int CMTime;

public:
    Ghost();
};

extern Ghost* ghost;

extern Command sar_ghost_set_demo;
extern Command sar_ghost_set_CM_time;
extern Variable sar_ghost_autostart;