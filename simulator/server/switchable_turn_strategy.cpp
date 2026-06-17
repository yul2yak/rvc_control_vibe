#include "switchable_turn_strategy.hpp"

namespace rvc::simulator
{

void SwitchableTurnStrategy::SetActive(ports::ITurnStrategy& strategy)
{
    active_strategy_ = &strategy;
}

domain::TurnDirection SwitchableTurnStrategy::Choose(const domain::SensorSnapshot& sensors) const
{
    if (active_strategy_ == nullptr)
    {
        return domain::TurnDirection::Left;
    }

    return active_strategy_->Choose(sensors);
}

}  // namespace rvc::simulator
