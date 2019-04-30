#pragma once
#include <vector>

#include "Features/Feature.hpp"

#include "Utils/SDK.hpp"

#include "Command.hpp"

enum class PropType {
    Integer,
    Boolean,
    Float,
    Handle,
    Vector,
    String,
    Char
};

<<<<<<< HEAD
struct SetAnglesData {
    QAngle currentAngles = { 0, 0, 0 };
    QAngle targetAngles = { 0, 0, 0 };
    float speedInterpolation = 0;
=======
struct TasPlayerData {
    QAngle currentAngles = { 0, 0, 0 };
    QAngle targetAngles = { 0, 0, 0 };
    float speedInterpolation = 0;
    Vector acceleration = { 0, 0, 0 };
    Vector prevVelocity = { 0, 0, 0 };
    int prevTick = 0;
>>>>>>> 1b44ab346acb6d0aa2e8764be26e9196dfc09ec2
};

class TasTools : public Feature {
public:
    char className[32];
    char propName[32];
    int propOffset;
    PropType propType;
<<<<<<< HEAD
    SetAnglesData data[MAX_SPLITSCREEN_PLAYERS];

private:
    Vector acceleration;
    Vector prevVelocity;
    int prevTick;

public:
    TasTools();
    void AimAtPoint(void* player, float x, float y, float z, int doSlerp);
    Vector GetVelocityAngles(void* player);
    Vector GetAcceleration(void* player);
    void* GetPlayerInfo();
    void SetAngles(void* pPlayer);
=======
    std::vector<TasPlayerData*> data;

public:
    TasTools();
    ~TasTools();

    void AimAtPoint(void* player, float x, float y, float z, int doSlerp);
    Vector GetVelocityAngles(void* player);
    Vector GetAcceleration(void* player);
    void* GetPlayerInfo(void* player);
    void SetAngles(void* player);
>>>>>>> 1b44ab346acb6d0aa2e8764be26e9196dfc09ec2
    QAngle Slerp(QAngle a0, QAngle a1, float speedInterpolation);
};

extern TasTools* tasTools;

extern Command sar_tas_aim_at_point;
extern Command sar_tas_set_prop;
extern Command sar_tas_addang;
extern Command sar_tas_setang;
