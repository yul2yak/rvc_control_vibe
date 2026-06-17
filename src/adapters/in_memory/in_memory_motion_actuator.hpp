#pragma once

#include <vector>

#include "domain/motion_command.hpp"
#include "ports/motion_actuator.hpp"

namespace rvc::adapters
{

class InMemoryMotionActuator : public ports::IMotionActuator
{
public:
    void Execute(domain::MotionCommand command) override;

    const std::vector<domain::MotionCommand>& History() const;
    void Clear();

private:
    std::vector<domain::MotionCommand> history_;
};

}  // namespace rvc::adapters
