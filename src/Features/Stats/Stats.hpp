#pragma once
#include <vector>

#include "JumpStats.hpp"
#include "StepStats.hpp"
#include "VelocityStats.hpp"

#include "Features/Feature.hpp"
#include "Features/Hud/Hud.hpp"

#include "Variable.hpp"

struct PlayerStats {
    JumpStats* jumps;
    StepStats* steps;
    VelocityStats* velocity;

    PlayerStats()
        : jumps(new JumpStats())
        , steps(new StepStats())
        , velocity(new VelocityStats())
    {
    }
    void Reset()
    {
        this->jumps->Reset();
        this->steps->Reset();
        this->velocity->Reset();
    }
    ~PlayerStats()
    {
        sdelete(this->jumps);
        sdelete(this->steps);
        sdelete(this->velocity);
    }
};

class Stats : public Feature {
private:
    std::vector<PlayerStats*> playerStats;

public:
    Stats();
    ~Stats();

    PlayerStats* Get(int nSlot);
    void ResetAll();
};

extern Stats* stats;

extern Variable sar_stats_jumps_xy;
extern Variable sar_stats_velocity_peak_xy;
extern Variable sar_stats_auto_reset;
