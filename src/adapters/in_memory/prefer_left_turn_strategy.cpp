#include "adapters/in_memory/prefer_left_turn_strategy.hpp"

namespace rvc::adapters
{

domain::TurnDirection PreferLeftTurnStrategy::Choose(const domain::SensorSnapshot& sensors) const
{
    if (!sensors.left_blocked)
    {
        return domain::TurnDirection::Left;
    }

    if (!sensors.right_blocked)
    {
        return domain::TurnDirection::Right;
    }

    return domain::TurnDirection::Left;
}

}  // namespace rvc::adapters
