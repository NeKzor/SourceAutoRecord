#pragma once
#include "TimerAction.hpp"

#include "Utils/SDK.hpp"

#define SAR_TIMER_PUB_INTERFACE_VERSION 1

class SpeedrunTimer;

class TimerInterface {
public:
    char version[16]; // 0
    int total; // 16
    float ipt; // 20
    TimerAction action; // 24
    unsigned int actionId; // 28

public:
    TimerInterface();
    void Reset();
    void SetIntervalPerTick(const float* ipt);
    void Update(SpeedrunTimer* timer);
    void SetAction(TimerAction action);
};

static_assert(sizeof(TimerInterface) == 32);
