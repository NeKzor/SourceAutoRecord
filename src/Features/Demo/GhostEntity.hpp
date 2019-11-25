#pragma once
#include <vector>

#include "Features/Feature.hpp"

#include "Command.hpp"
#include "Features/Demo/Demo.hpp"
#include "SFML/Network.hpp"
#include "Utils/SDK.hpp"
#include "Variable.hpp"

#include <chrono>
#include <vector>

struct DataGhost {
    QAngle position;
    QAngle view_angle;
};

class GhostEntity {

private:
    int tickCount;
    float startDelay;
    std::chrono::steady_clock clock;

public:
    std::vector<Vector> positionList;
    std::vector<Vector> angleList;
    unsigned int ID;
    std::string name;
    std::string currentMap;
    bool sameMap;
    Demo demo;
    int startTick;
    void* ghost_entity;
    int CMTime;
    char modelName[64];
    bool isPlaying;
    bool hasFinished;

    DataGhost oldPos;
    DataGhost newPos;
    Vector currentPos;
    std::chrono::time_point<std::chrono::steady_clock> lastUpdate;
    long long loopTime;

    //void* trail;

public:
    GhostEntity();
    void Reset();
    void Stop();
    GhostEntity* Spawn(bool instantPlay, Vector position);
    bool IsReady();
    void SetCMTime(float);
    void Think();
    int GetTickCount();
    int GetStartDelay();
    void SetStartDelay(int);
    void ChangeModel(std::string modelName);
    void SetPosAng(const Vector&, const Vector&);
    void Lerp(DataGhost& oldPosition, DataGhost& targetPosition, float time);
};
