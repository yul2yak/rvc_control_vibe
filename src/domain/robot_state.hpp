#pragma once

namespace rvc::domain
{

enum class RobotMotionState
{
    Idle,
    MovingForward,
    Stopping,
    Turning,
    BackingUp
};

}  // namespace rvc::domain
