#pragma once

namespace rvc::domain
{

enum class MotionCommand
{
    None,
    MoveForward,
    Stop,
    TurnLeft,
    TurnRight,
    MoveBackward
};

enum class TurnDirection
{
    Left,
    Right
};

enum class CleaningCommand
{
    None,
    SetOff,
    SetNormal,
    SetBoost
};

}  // namespace rvc::domain
