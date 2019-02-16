#pragma once
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

struct SetAngleData {
    QAngle currentAngle = { 0, 0, 0 };
    QAngle targetAngle = { 0, 0, 0 };
    float speedInterpolation = 0;
};

class TasTools : public Feature {
public:
    char className[32];
    char propName[32];
    int propOffset;
    PropType propType;
    SetAngleData data[MAX_SPLITSCREEN_PLAYERS];

private:
    Vector acceleration;
    Vector prevVelocity;
    int prevTick;

public:
    TasTools();
    void AimAtPoint(void* player, float x, float y, float z);
    Vector GetVelocityAngles(void* player);
    Vector GetAcceleration(void* player);
    void* GetPlayerInfo();
    void SetAngle(void *pPlayer);
};

extern TasTools* tasTools;

extern Command sar_tas_aim_at_point;
extern Command sar_tas_set_prop;
extern Command sar_tas_addang;
extern Command sar_tas_setang;
