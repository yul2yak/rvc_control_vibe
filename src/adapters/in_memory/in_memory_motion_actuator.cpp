#include "adapters/in_memory/in_memory_motion_actuator.hpp"

namespace rvc::adapters
{

void InMemoryMotionActuator::Execute(domain::MotionCommand command)
{
    history_.push_back(command);
}

const std::vector<domain::MotionCommand>& InMemoryMotionActuator::History() const
{
    return history_;
}

void InMemoryMotionActuator::Clear()
{
    history_.clear();
}

}  // namespace rvc::adapters
