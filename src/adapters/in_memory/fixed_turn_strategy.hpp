#pragma once

#include "domain/motion_command.hpp"
#include "domain/sensor_snapshot.hpp"
#include "ports/turn_strategy.hpp"

namespace rvc::adapters
{

class FixedTurnStrategy : public ports::ITurnStrategy
{
public:
    explicit FixedTurnStrategy(domain::TurnDirection direction);

    domain::TurnDirection Choose(const domain::SensorSnapshot& sensors) const override;

private:
    domain::TurnDirection direction_;
};

}  // namespace rvc::adapters
