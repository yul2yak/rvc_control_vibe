#include "adapters/in_memory/fixed_turn_strategy.hpp"

namespace rvc::adapters
{

FixedTurnStrategy::FixedTurnStrategy(domain::TurnDirection direction)
    : direction_(direction)
{
}

domain::TurnDirection FixedTurnStrategy::Choose(const domain::SensorSnapshot& sensors) const
{
    (void)sensors;
    return direction_;
}

}  // namespace rvc::adapters
