#include "domain/cleaning_state_machine.hpp"

namespace rvc::domain
{

CleaningStateMachine::CleaningStateMachine()
    : motion_state_(RobotMotionState::Idle)
    , cleaning_mode_(CleaningMode::Off)
    , active_turn_direction_(TurnDirection::Left)
{
}

RobotMotionState CleaningStateMachine::MotionState() const
{
    return motion_state_;
}

CleaningMode CleaningStateMachine::CleaningModeState() const
{
    return cleaning_mode_;
}

TurnDirection CleaningStateMachine::ActiveTurnDirection() const
{
    return active_turn_direction_;
}

void CleaningStateMachine::Start()
{
    motion_state_ = RobotMotionState::MovingForward;
    cleaning_mode_ = CleaningMode::Normal;
}

bool CleaningStateMachine::AllSidesBlocked(const SensorSnapshot& sensors) const
{
    return sensors.front_blocked && sensors.left_blocked && sensors.right_blocked;
}

bool CleaningStateMachine::FrontBlocked(const SensorSnapshot& sensors) const
{
    return sensors.front_blocked;
}

MotionCommand CleaningStateMachine::TurnMotionCommand() const
{
    return active_turn_direction_ == TurnDirection::Left ? MotionCommand::TurnLeft
                                                         : MotionCommand::TurnRight;
}

TickResult CleaningStateMachine::Tick(const TickContext& context)
{
    switch (motion_state_)
    {
    case RobotMotionState::Idle:
        return {};
    case RobotMotionState::MovingForward:
        return HandleMovingForward(context);
    case RobotMotionState::Stopping:
        return HandleStopping(context);
    case RobotMotionState::BackingUp:
        return HandleBackingUp(context);
    case RobotMotionState::Turning:
        return HandleTurning(context);
    }

    return {};
}

TickResult CleaningStateMachine::HandleMovingForward(const TickContext& context)
{
    const SensorSnapshot& sensors = context.sensors;

    if (AllSidesBlocked(sensors))
    {
        motion_state_ = RobotMotionState::BackingUp;
        cleaning_mode_ = CleaningMode::Off;

        TickResult result;
        result.next_motion_state = motion_state_;
        result.next_cleaning_mode = cleaning_mode_;
        result.motion = MotionCommand::MoveBackward;
        result.clean = CleaningCommand::SetOff;
        return result;
    }

    if (FrontBlocked(sensors))
    {
        motion_state_ = RobotMotionState::Stopping;
        cleaning_mode_ = CleaningMode::Off;

        TickResult result;
        result.next_motion_state = motion_state_;
        result.next_cleaning_mode = cleaning_mode_;
        result.motion = MotionCommand::Stop;
        result.clean = CleaningCommand::SetOff;
        return result;
    }

    TickResult result;
    result.next_motion_state = RobotMotionState::MovingForward;
    result.motion = MotionCommand::MoveForward;

    if (cleaning_mode_ == CleaningMode::Boost)
    {
        if (context.boost_timer_expired)
        {
            cleaning_mode_ = CleaningMode::Normal;
            result.clean = CleaningCommand::SetNormal;
        }
        else
        {
            result.clean = CleaningCommand::SetBoost;
        }
        result.next_cleaning_mode = cleaning_mode_;
        return result;
    }

    return ApplyCleaningBoost(context, result);
}

TickResult CleaningStateMachine::HandleStopping(const TickContext& context)
{
    active_turn_direction_ = context.turn_direction;
    motion_state_ = RobotMotionState::Turning;

    TickResult result;
    result.next_motion_state = motion_state_;
    result.next_cleaning_mode = cleaning_mode_;
    result.motion = TurnMotionCommand();
    result.clean = CleaningCommand::SetOff;
    return result;
}

TickResult CleaningStateMachine::HandleBackingUp(const TickContext& context)
{
    active_turn_direction_ = context.turn_direction;
    motion_state_ = RobotMotionState::Turning;

    TickResult result;
    result.next_motion_state = motion_state_;
    result.next_cleaning_mode = cleaning_mode_;
    result.motion = TurnMotionCommand();
    result.clean = CleaningCommand::SetOff;
    return result;
}

TickResult CleaningStateMachine::HandleTurning(const TickContext& context)
{
    if (!FrontBlocked(context.sensors))
    {
        motion_state_ = RobotMotionState::MovingForward;
        cleaning_mode_ = CleaningMode::Normal;

        TickResult result;
        result.next_motion_state = motion_state_;
        result.next_cleaning_mode = cleaning_mode_;
        result.motion = MotionCommand::MoveForward;
        result.clean = CleaningCommand::SetNormal;
        return result;
    }

    TickResult result;
    result.next_motion_state = RobotMotionState::Turning;
    result.next_cleaning_mode = cleaning_mode_;
    result.motion = TurnMotionCommand();
    result.clean = CleaningCommand::SetOff;
    return result;
}

TickResult CleaningStateMachine::ApplyCleaningBoost(const TickContext& context, TickResult result)
{
    cleaning_mode_ = CleaningMode::Normal;
    result.next_cleaning_mode = cleaning_mode_;
    result.clean = CleaningCommand::SetNormal;

    if (context.sensors.dust_detected)
    {
        cleaning_mode_ = CleaningMode::Boost;
        result.next_cleaning_mode = cleaning_mode_;
        result.clean = CleaningCommand::SetBoost;
    }

    return result;
}

}  // namespace rvc::domain
