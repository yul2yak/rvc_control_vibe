#pragma once

#include "domain/motion_command.hpp"

namespace rvc::ports
{

class ICleaningActuator
{
public:
    virtual ~ICleaningActuator() = default;

    virtual void Execute(domain::CleaningCommand command) = 0;
};

}  // namespace rvc::ports
