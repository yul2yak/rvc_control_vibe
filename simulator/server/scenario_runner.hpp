#pragma once

#include <chrono>
#include <string>
#include <vector>

#include "domain/motion_command.hpp"
#include "domain/sensor_snapshot.hpp"

namespace rvc::simulator
{

struct ScenarioStepResult
{
    int step_index = 0;
    std::string expected_motion;
    std::string actual_motion;
    std::string expected_clean;
    std::string actual_clean;
    bool passed = false;
};

struct ScenarioResult
{
    std::string id;
    std::string name;
    bool passed = false;
    std::string message;
    std::vector<ScenarioStepResult> steps;
};

struct ScenarioRunSummary
{
    int total = 0;
    int passed = 0;
    int failed = 0;
    std::vector<ScenarioResult> scenarios;
};

class ScenarioRunner
{
public:
    ScenarioRunSummary RunAll() const;
};

}  // namespace rvc::simulator
