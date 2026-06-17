#include "scenario_definitions.hpp"

#include <sstream>

namespace rvc::simulator
{

namespace
{

domain::SensorSnapshot ClearSensors()
{
    return {};
}

domain::SensorSnapshot FrontBlockedSensors()
{
    domain::SensorSnapshot sensors;
    sensors.front_blocked = true;
    return sensors;
}

domain::SensorSnapshot AllSidesBlockedSensors()
{
    domain::SensorSnapshot sensors;
    sensors.front_blocked = true;
    sensors.left_blocked = true;
    sensors.right_blocked = true;
    return sensors;
}

domain::SensorSnapshot DustySensors()
{
    domain::SensorSnapshot sensors;
    sensors.dust_detected = true;
    return sensors;
}

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

std::string TurnStrategyToString(ScenarioTurnStrategy strategy)
{
    return strategy == ScenarioTurnStrategy::ForceRight ? "force_right" : "prefer_left";
}

std::string SensorsToJson(const domain::SensorSnapshot& sensors)
{
    std::ostringstream json;
    json << "{"
         << "\"front_blocked\":" << (sensors.front_blocked ? "true" : "false") << ","
         << "\"left_blocked\":" << (sensors.left_blocked ? "true" : "false") << ","
         << "\"right_blocked\":" << (sensors.right_blocked ? "true" : "false") << ","
         << "\"dust_detected\":" << (sensors.dust_detected ? "true" : "false")
         << "}";
    return json.str();
}

}  // namespace

const std::vector<ScenarioDefinition>& GetScenarioDefinitions()
{
    static const std::vector<ScenarioDefinition> definitions = {
        {
            "forward_cleaning",
            "직진 청소",
            ScenarioTurnStrategy::PreferLeft,
            {
                {ClearSensors(), domain::MotionCommand::MoveForward, domain::CleaningCommand::SetNormal},
                {ClearSensors(), domain::MotionCommand::MoveForward, domain::CleaningCommand::SetNormal},
                {ClearSensors(), domain::MotionCommand::MoveForward, domain::CleaningCommand::SetNormal},
            },
        },
        {
            "front_obstacle",
            "전방 장애물 회피",
            ScenarioTurnStrategy::PreferLeft,
            {
                {FrontBlockedSensors(), domain::MotionCommand::Stop, domain::CleaningCommand::SetOff},
                {FrontBlockedSensors(), domain::MotionCommand::TurnLeft, domain::CleaningCommand::SetOff},
                {ClearSensors(), domain::MotionCommand::MoveForward, domain::CleaningCommand::SetNormal},
            },
        },
        {
            "all_sides_blocked",
            "전/좌/우 막힘",
            ScenarioTurnStrategy::PreferLeft,
            {
                {AllSidesBlockedSensors(), domain::MotionCommand::MoveBackward, domain::CleaningCommand::SetOff},
                {AllSidesBlockedSensors(), domain::MotionCommand::TurnLeft, domain::CleaningCommand::SetOff},
                {ClearSensors(), domain::MotionCommand::MoveForward, domain::CleaningCommand::SetNormal},
            },
        },
        {
            "dust_boost",
            "먼지 흡입 및 부스트",
            ScenarioTurnStrategy::PreferLeft,
            {
                {ClearSensors(), domain::MotionCommand::MoveForward, domain::CleaningCommand::SetNormal},
                {DustySensors(), domain::MotionCommand::MoveForward, domain::CleaningCommand::SetBoost},
                {ClearSensors(), domain::MotionCommand::MoveForward, domain::CleaningCommand::SetNormal,
                 std::chrono::seconds(3)},
            },
        },
        {
            "right_turn_obstacle",
            "우회전 전략",
            ScenarioTurnStrategy::ForceRight,
            {
                {FrontBlockedSensors(), domain::MotionCommand::Stop, domain::CleaningCommand::SetOff},
                {FrontBlockedSensors(), domain::MotionCommand::TurnRight, domain::CleaningCommand::SetOff},
            },
        },
    };

    return definitions;
}

std::string ScenarioDefinitionsToJson()
{
    std::ostringstream json;
    json << "{\"scenarios\":[";

    const std::vector<ScenarioDefinition>& definitions = GetScenarioDefinitions();
    for (std::size_t i = 0; i < definitions.size(); ++i)
    {
        if (i > 0)
        {
            json << ",";
        }

        const ScenarioDefinition& scenario = definitions[i];
        json << "{"
             << "\"id\":\"" << scenario.id << "\","
             << "\"name\":\"" << scenario.name << "\","
             << "\"turn_strategy\":\"" << TurnStrategyToString(scenario.turn_strategy) << "\","
             << "\"steps\":[";

        for (std::size_t j = 0; j < scenario.steps.size(); ++j)
        {
            if (j > 0)
            {
                json << ",";
            }

            const ScenarioStepDefinition& step = scenario.steps[j];
            json << "{"
                 << "\"sensors\":" << SensorsToJson(step.sensors) << ","
                 << "\"expected_motion\":\"" << MotionToString(step.expected_motion) << "\","
                 << "\"expected_clean\":\"" << CleanToString(step.expected_clean) << "\","
                 << "\"timer_advance_ms\":" << step.timer_advance.count()
                 << "}";
        }

        json << "]}";
    }

    json << "]}";
    return json.str();
}

}  // namespace rvc::simulator
