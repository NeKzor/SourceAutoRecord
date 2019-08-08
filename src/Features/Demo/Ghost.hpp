#pragma once
#include <vector>

#include "Features/Feature.hpp"

#include "Utils/SDK.hpp"
#include "Command.hpp"
#include "Variable.hpp"

#include <vector>

class Ghost : public Feature {

private:
    int tickCount;

public:
    std::vector<Vector> positionList;
    std::vector<Vector> angleList;
    int startTick;
    int endTick;
    void* ghost_entity;
    int CMTime;
    char modelName[64];
    bool isPlaying;
    bool mapSpawning;
    bool inMap;

public:
    Ghost();
    void Reset();
    void Start();
    bool IsReady();
    void SetCMTime(float);
    void Think();
};

extern Ghost* ghost;

extern Command sar_ghost_set_demo;
extern Command sar_ghost_set_prop_model;
extern Variable sar_ghost_enable;
extern Variable sar_ghost_height;
extern Variable sar_ghost_transparency;