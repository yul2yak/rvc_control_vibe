#include "simulator_service.hpp"

namespace rvc::simulator
{

SimulatorService::SimulatorService()
    : force_right_strategy_(domain::TurnDirection::Right)
    , cleaning_service_(
          sensor_reader_,
          motion_actuator_,
          cleaning_actuator_,
          timer_,
          switchable_turn_strategy_)
{
    switchable_turn_strategy_.SetActive(prefer_left_strategy_);
}

void SimulatorService::Reset()
{
    std::lock_guard<std::mutex> lock(mutex_);
    sensor_reader_.Reset();
    motion_actuator_.Clear();
    cleaning_actuator_.Clear();
    timer_.CancelBoost();
}

void SimulatorService::Start(const std::string& turn_strategy)
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (turn_strategy == "force_right")
    {
        switchable_turn_strategy_.SetActive(force_right_strategy_);
    }
    else
    {
        switchable_turn_strategy_.SetActive(prefer_left_strategy_);
    }

    motion_actuator_.Clear();
    cleaning_actuator_.Clear();
    timer_.CancelBoost();
    cleaning_service_.Start();
}

SimulatorState SimulatorService::Tick(
    const domain::SensorSnapshot& sensors,
    std::chrono::milliseconds timer_advance)
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (timer_advance.count() > 0)
    {
        timer_.Advance(timer_advance);
    }

    sensor_reader_.SetSnapshots({sensors});
    cleaning_service_.Tick();

    SimulatorState state;
    state.motion_state = cleaning_service_.MotionState();
    state.cleaning_mode = cleaning_service_.CleaningModeState();
    state.running = cleaning_service_.IsRunning();

    if (!motion_actuator_.History().empty())
    {
        state.last_motion = motion_actuator_.History().back();
    }
    if (!cleaning_actuator_.History().empty())
    {
        state.last_clean = cleaning_actuator_.History().back();
    }

    return state;
}

SimulatorState SimulatorService::GetState() const
{
    std::lock_guard<std::mutex> lock(mutex_);

    SimulatorState state;
    state.motion_state = cleaning_service_.MotionState();
    state.cleaning_mode = cleaning_service_.CleaningModeState();
    state.running = cleaning_service_.IsRunning();

    if (!motion_actuator_.History().empty())
    {
        state.last_motion = motion_actuator_.History().back();
    }
    if (!cleaning_actuator_.History().empty())
    {
        state.last_clean = cleaning_actuator_.History().back();
    }

    return state;
}

}  // namespace rvc::simulator
