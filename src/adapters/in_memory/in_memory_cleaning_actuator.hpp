#pragma once

#include <vector>

#include "domain/motion_command.hpp"
#include "ports/cleaning_actuator.hpp"

namespace rvc::adapters
{

class InMemoryCleaningActuator : public ports::ICleaningActuator
{
public:
    void Execute(domain::CleaningCommand command) override;

    const std::vector<domain::CleaningCommand>& History() const;
    void Clear();

private:
    std::vector<domain::CleaningCommand> history_;
};

}  // namespace rvc::adapters
