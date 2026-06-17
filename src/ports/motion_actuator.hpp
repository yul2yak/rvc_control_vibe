#pragma once

#include "domain/motion_command.hpp"

namespace rvc::ports
{

class IMotionActuator
{
public:
    virtual ~IMotionActuator() = default;

    virtual void Execute(domain::MotionCommand command) = 0;
};

}  // namespace rvc::ports
