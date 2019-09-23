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
    std::string ID;
    std::string name;
    char currentMap[64];
    bool sameMap;
    Demo demo;
    int startTick;
    void* ghost_entity;
    int CMTime;
    char modelName[64];
    bool isPlaying;
    bool hasFinished;

    void* trail;

public:
    GhostEntity();
    void Reset();
    void Stop();
    GhostEntity* Spawn(bool instantPlay = true, bool playerPos = false, Vector position = { 0, 0, 0 });
    bool IsReady();
    void SetCMTime(float);
    void Think();
    int GetStartDelay();
    void SetStartDelay(int);
    void ChangeModel(const char modelName[64]);
    void SetPosAng(const Vector&, const Vector&);
};
