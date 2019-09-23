#pragma once
#include <vector>

#include "Features/Feature.hpp"

#include "Command.hpp"
#include "Features/Demo/Demo.hpp"
#include "Utils/SDK.hpp"
#include "Variable.hpp"

#include <vector>

class GhostEntity {

private:
    int tickCount;
    float startDelay;

public:
    std::vector<Vector> positionList;
    std::vector<Vector> angleList;
    Demo demo;
    int startTick;
    void* ghost_entity;
    int CMTime;
    char modelName[64];
    bool isPlaying;
    bool hasFinished;

public:
    GhostEntity();
    void Reset();
    void Stop();
    GhostEntity* Spawn(bool instantPlay = true, bool playerPos = false);
    bool IsReady();
    void SetCMTime(float);
    void Think();
    int GetStartDelay();
    void SetStartDelay(int);
    void ChangeModel(const char modelName[64]);
};
