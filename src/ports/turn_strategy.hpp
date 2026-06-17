#pragma once

#include "domain/motion_command.hpp"
#include "domain/sensor_snapshot.hpp"

namespace rvc::ports
{

class ITurnStrategy
{
public:
    virtual ~ITurnStrategy() = default;

    virtual domain::TurnDirection Choose(const domain::SensorSnapshot& sensors) const = 0;
};

}  // namespace rvc::ports
