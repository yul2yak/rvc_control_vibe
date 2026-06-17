#pragma once

#include "domain/motion_command.hpp"
#include "domain/sensor_snapshot.hpp"
#include "ports/turn_strategy.hpp"

namespace rvc::tests
{

class FixedTurnStrategy : public ports::ITurnStrategy
{
public:
    explicit FixedTurnStrategy(domain::TurnDirection direction)
        : direction_(direction)
    {
    }

    domain::TurnDirection Choose(const domain::SensorSnapshot& sensors) const override
    {
        (void)sensors;
        return direction_;
    }

private:
    domain::TurnDirection direction_;
};

}  // namespace rvc::tests
