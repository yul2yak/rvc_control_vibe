#pragma once

#include <chrono>

#include "domain/cleaning_state_machine.hpp"
#include "ports/cleaning_actuator.hpp"
#include "ports/motion_actuator.hpp"
#include "ports/sensor_reader.hpp"
#include "ports/timer.hpp"
#include "ports/turn_strategy.hpp"

namespace rvc::application
{

class CleaningService
{
public:
    CleaningService(
        ports::ISensorReader& sensor_reader,
        ports::IMotionActuator& motion_actuator,
        ports::ICleaningActuator& cleaning_actuator,
        ports::ITimer& timer,
        ports::ITurnStrategy& turn_strategy,
        std::chrono::milliseconds boost_duration = std::chrono::seconds(3));

    void Start();
    void Tick();
    bool IsRunning() const;

    domain::RobotMotionState MotionState() const;
    domain::CleaningMode CleaningModeState() const;

private:
    ports::ISensorReader& sensor_reader_;
    ports::IMotionActuator& motion_actuator_;
    ports::ICleaningActuator& cleaning_actuator_;
    ports::ITimer& timer_;
    ports::ITurnStrategy& turn_strategy_;
    std::chrono::milliseconds boost_duration_;
    domain::CleaningStateMachine state_machine_;
    bool running_;
};

}  // namespace rvc::application
