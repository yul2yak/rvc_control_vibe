#pragma once

#include <chrono>
#include <mutex>
#include <string>

#include "adapters/in_memory/fake_timer.hpp"
#include "adapters/in_memory/fixed_turn_strategy.hpp"
#include "adapters/in_memory/in_memory_cleaning_actuator.hpp"
#include "adapters/in_memory/in_memory_motion_actuator.hpp"
#include "adapters/in_memory/in_memory_sensor_reader.hpp"
#include "adapters/in_memory/prefer_left_turn_strategy.hpp"
#include "application/cleaning_service.hpp"
#include "domain/motion_command.hpp"
#include "domain/robot_state.hpp"
#include "domain/sensor_snapshot.hpp"
#include "switchable_turn_strategy.hpp"

namespace rvc::simulator
{

struct SimulatorState
{
    domain::RobotMotionState motion_state = domain::RobotMotionState::Idle;
    domain::CleaningMode cleaning_mode = domain::CleaningMode::Off;
    domain::MotionCommand last_motion = domain::MotionCommand::None;
    domain::CleaningCommand last_clean = domain::CleaningCommand::None;
    bool running = false;
};

class SimulatorService
{
public:
    SimulatorService();

    void Reset();
    void Start(const std::string& turn_strategy = "prefer_left");
    SimulatorState Tick(
        const domain::SensorSnapshot& sensors,
        std::chrono::milliseconds timer_advance = std::chrono::milliseconds{0});
    SimulatorState GetState() const;

private:
    mutable std::mutex mutex_;
    adapters::InMemorySensorReader sensor_reader_;
    adapters::InMemoryMotionActuator motion_actuator_;
    adapters::InMemoryCleaningActuator cleaning_actuator_;
    adapters::FakeTimer timer_;
    adapters::PreferLeftTurnStrategy prefer_left_strategy_;
    adapters::FixedTurnStrategy force_right_strategy_;
    SwitchableTurnStrategy switchable_turn_strategy_;
    application::CleaningService cleaning_service_;
};

}  // namespace rvc::simulator
