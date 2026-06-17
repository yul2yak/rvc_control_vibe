#include "scenario_runner.hpp"

#include <sstream>

#include "adapters/in_memory/fake_timer.hpp"
#include "adapters/in_memory/fixed_turn_strategy.hpp"
#include "adapters/in_memory/in_memory_cleaning_actuator.hpp"
#include "adapters/in_memory/in_memory_motion_actuator.hpp"
#include "adapters/in_memory/in_memory_sensor_reader.hpp"
#include "adapters/in_memory/prefer_left_turn_strategy.hpp"
#include "application/cleaning_service.hpp"
#include "scenario_definitions.hpp"

namespace rvc::simulator
{

namespace
{

std::string MotionToString(domain::MotionCommand command)
{
    switch (command)
    {
    case domain::MotionCommand::MoveForward:
        return "MoveForward";
    case domain::MotionCommand::Stop:
        return "Stop";
    case domain::MotionCommand::TurnLeft:
        return "TurnLeft";
    case domain::MotionCommand::TurnRight:
        return "TurnRight";
    case domain::MotionCommand::MoveBackward:
        return "MoveBackward";
    default:
        return "None";
    }
}

std::string CleanToString(domain::CleaningCommand command)
{
    switch (command)
    {
    case domain::CleaningCommand::SetNormal:
        return "SetNormal";
    case domain::CleaningCommand::SetBoost:
        return "SetBoost";
    case domain::CleaningCommand::SetOff:
        return "SetOff";
    default:
        return "None";
    }
}

ScenarioResult RunDefinition(
    const ScenarioDefinition& definition,
    ports::ITurnStrategy& turn_strategy)
{
    ScenarioResult result;
    result.id = definition.id;
    result.name = definition.name;
    result.passed = true;

    adapters::InMemorySensorReader sensor_reader;
    adapters::InMemoryMotionActuator motion_actuator;
    adapters::InMemoryCleaningActuator cleaning_actuator;
    adapters::FakeTimer timer;

    application::CleaningService service(
        sensor_reader, motion_actuator, cleaning_actuator, timer, turn_strategy);

    service.Start();

    for (std::size_t i = 0; i < definition.steps.size(); ++i)
    {
        const ScenarioStepDefinition& step = definition.steps[i];

        if (step.timer_advance.count() > 0)
        {
            timer.Advance(step.timer_advance);
        }

        sensor_reader.SetSnapshots({step.sensors});
        service.Tick();

        ScenarioStepResult step_result;
        step_result.step_index = static_cast<int>(i) + 1;
        step_result.expected_motion = MotionToString(step.expected_motion);
        step_result.expected_clean = CleanToString(step.expected_clean);

        const auto& motions = motion_actuator.History();
        const auto& cleans = cleaning_actuator.History();

        if (motions.empty() || cleans.empty())
        {
            step_result.actual_motion = "None";
            step_result.actual_clean = "None";
            step_result.passed = false;
            result.passed = false;
            result.message = "Step " + std::to_string(step_result.step_index) + ": no actuator command";
            result.steps.push_back(step_result);
            continue;
        }

        step_result.actual_motion = MotionToString(motions.back());
        step_result.actual_clean = CleanToString(cleans.back());
        step_result.passed = motions.back() == step.expected_motion
                             && cleans.back() == step.expected_clean;

        if (!step_result.passed)
        {
            result.passed = false;
            std::ostringstream message;
            message << "Step " << step_result.step_index << " mismatch";
            result.message = message.str();
        }

        result.steps.push_back(step_result);
    }

    if (result.passed)
    {
        result.message = "OK";
    }

    return result;
}

}  // namespace

ScenarioRunSummary ScenarioRunner::RunAll() const
{
    ScenarioRunSummary summary;

    adapters::PreferLeftTurnStrategy prefer_left;
    adapters::FixedTurnStrategy force_right(domain::TurnDirection::Right);

    for (const ScenarioDefinition& definition : GetScenarioDefinitions())
    {
        ports::ITurnStrategy& turn_strategy = definition.turn_strategy == ScenarioTurnStrategy::ForceRight
                                                 ? static_cast<ports::ITurnStrategy&>(force_right)
                                                 : static_cast<ports::ITurnStrategy&>(prefer_left);
        summary.scenarios.push_back(RunDefinition(definition, turn_strategy));
    }

    summary.total = static_cast<int>(summary.scenarios.size());
    for (const ScenarioResult& scenario : summary.scenarios)
    {
        if (scenario.passed)
        {
            ++summary.passed;
        }
        else
        {
            ++summary.failed;
        }
    }

    return summary;
}

}  // namespace rvc::simulator
