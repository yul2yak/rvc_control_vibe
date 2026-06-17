#pragma once

#include "domain/motion_command.hpp"
#include "domain/sensor_snapshot.hpp"
#include "ports/turn_strategy.hpp"

namespace rvc::simulator
{

class SwitchableTurnStrategy : public ports::ITurnStrategy
{
public:
    void SetActive(ports::ITurnStrategy& strategy);
    domain::TurnDirection Choose(const domain::SensorSnapshot& sensors) const override;

private:
    ports::ITurnStrategy* active_strategy_ = nullptr;
};

}  // namespace rvc::simulator
