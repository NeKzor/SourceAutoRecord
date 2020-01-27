#include "TimerInterface.hpp"

#include "SpeedrunTimer.hpp"
#include "TimerAction.hpp"

TimerInterface::TimerInterface()
    : version()
    , total(0)
    , ipt(0)
    , action(TimerAction::DoNothing)
    , actionId(0)
{
    std::snprintf(this->version, sizeof(this->version), "SAR_TIMER_00%i", SAR_TIMER_PUB_INTERFACE_VERSION);
}
void TimerInterface::Reset()
{
    this->actionId = 0;
}
void TimerInterface::SetIntervalPerTick(const float* ipt)
{
    this->ipt = *ipt;
}
void TimerInterface::Update(SpeedrunTimer* timer)
{
    this->total = timer->GetTotal();
}
void TimerInterface::SetAction(TimerAction action)
{
    this->action = action;
    ++this->actionId;
}
