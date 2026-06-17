#pragma once

#include "domain/motion_command.hpp"
#include "domain/sensor_snapshot.hpp"
#include "ports/turn_strategy.hpp"

namespace rvc::adapters
{

class PreferLeftTurnStrategy : public ports::ITurnStrategy
{
public:
    domain::TurnDirection Choose(const domain::SensorSnapshot& sensors) const override;
};

}  // namespace rvc::adapters
