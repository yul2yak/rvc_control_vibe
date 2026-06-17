#pragma once

#include <chrono>
#include <string>
#include <vector>

#include "domain/motion_command.hpp"
#include "domain/sensor_snapshot.hpp"

namespace rvc::simulator
{

enum class ScenarioTurnStrategy
{
    PreferLeft,
    ForceRight
};

struct ScenarioStepDefinition
{
    domain::SensorSnapshot sensors;
    domain::MotionCommand expected_motion = domain::MotionCommand::None;
    domain::CleaningCommand expected_clean = domain::CleaningCommand::None;
    std::chrono::milliseconds timer_advance{0};
};

struct ScenarioDefinition
{
    std::string id;
    std::string name;
    ScenarioTurnStrategy turn_strategy = ScenarioTurnStrategy::PreferLeft;
    std::vector<ScenarioStepDefinition> steps;
};

const std::vector<ScenarioDefinition>& GetScenarioDefinitions();

std::string ScenarioDefinitionsToJson();

}  // namespace rvc::simulator
