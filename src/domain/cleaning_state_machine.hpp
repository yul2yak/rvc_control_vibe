#pragma once

#include "domain/cleaning_mode.hpp"
#include "domain/motion_command.hpp"
#include "domain/robot_state.hpp"
#include "domain/sensor_snapshot.hpp"

namespace rvc::domain
{

struct TickContext
{
    SensorSnapshot sensors;
    TurnDirection turn_direction = TurnDirection::Left;
    bool boost_timer_expired = false;
};

struct TickResult
{
    RobotMotionState next_motion_state = RobotMotionState::Idle;
    CleaningMode next_cleaning_mode = CleaningMode::Off;
    MotionCommand motion = MotionCommand::None;
    CleaningCommand clean = CleaningCommand::None;
};

class CleaningStateMachine
{
public:
    CleaningStateMachine();

    RobotMotionState MotionState() const;
    CleaningMode CleaningModeState() const;
    TurnDirection ActiveTurnDirection() const;

    TickResult Tick(const TickContext& context);
    void Start();

private:
    bool AllSidesBlocked(const SensorSnapshot& sensors) const;
    bool FrontBlocked(const SensorSnapshot& sensors) const;
    MotionCommand TurnMotionCommand() const;
    TickResult HandleMovingForward(const TickContext& context);
    TickResult HandleStopping(const TickContext& context);
    TickResult HandleBackingUp(const TickContext& context);
    TickResult HandleTurning(const TickContext& context);
    TickResult ApplyCleaningBoost(const TickContext& context, TickResult result);

    RobotMotionState motion_state_;
    CleaningMode cleaning_mode_;
    TurnDirection active_turn_direction_;
};

}  // namespace rvc::domain
