#include "adapters/in_memory/in_memory_cleaning_actuator.hpp"

namespace rvc::adapters
{

void InMemoryCleaningActuator::Execute(domain::CleaningCommand command)
{
    history_.push_back(command);
}

const std::vector<domain::CleaningCommand>& InMemoryCleaningActuator::History() const
{
    return history_;
}

void InMemoryCleaningActuator::Clear()
{
    history_.clear();
}

}  // namespace rvc::adapters
