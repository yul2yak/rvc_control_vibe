#include "application/cleaning_service.hpp"

namespace rvc::application
{

CleaningService::CleaningService(
    ports::ISensorReader& sensor_reader,
    ports::IMotionActuator& motion_actuator,
    ports::ICleaningActuator& cleaning_actuator,
    ports::ITimer& timer,
    ports::ITurnStrategy& turn_strategy,
    std::chrono::milliseconds boost_duration)
    : sensor_reader_(sensor_reader)
    , motion_actuator_(motion_actuator)
    , cleaning_actuator_(cleaning_actuator)
    , timer_(timer)
    , turn_strategy_(turn_strategy)
    , boost_duration_(boost_duration)
    , running_(false)
{
}

void CleaningService::Start()
{
    state_machine_.Start();
    running_ = true;
    timer_.CancelBoost();
}

void CleaningService::Tick()
{
    if (!running_)
    {
        return;
    }

    const domain::SensorSnapshot sensors = sensor_reader_.Read();

    domain::TickContext context;
    context.sensors = sensors;
    context.turn_direction = turn_strategy_.Choose(sensors);
    context.boost_timer_expired = timer_.IsBoostExpired();

    const domain::TickResult result = state_machine_.Tick(context);

    if (result.motion != domain::MotionCommand::None)
    {
        motion_actuator_.Execute(result.motion);
    }

    if (result.clean != domain::CleaningCommand::None)
    {
        cleaning_actuator_.Execute(result.clean);

        if (result.clean == domain::CleaningCommand::SetBoost)
        {
            timer_.StartBoost(boost_duration_);
        }
        else if (result.clean == domain::CleaningCommand::SetOff
                 || result.clean == domain::CleaningCommand::SetNormal)
        {
            timer_.CancelBoost();
        }
    }
}

bool CleaningService::IsRunning() const
{
    return running_;
}

domain::RobotMotionState CleaningService::MotionState() const
{
    return state_machine_.MotionState();
}

domain::CleaningMode CleaningService::CleaningModeState() const
{
    return state_machine_.CleaningModeState();
}

}  // namespace rvc::application
