#include <iostream>
#include <sstream>
#include <string>

#include <httplib.h>

#include "domain/motion_command.hpp"
#include "domain/robot_state.hpp"
#include "scenario_definitions.hpp"
#include "scenario_runner.hpp"
#include "simulator_service.hpp"

using namespace rvc;

namespace
{

std::string MotionStateToString(domain::RobotMotionState state)
{
    switch (state)
    {
    case domain::RobotMotionState::MovingForward:
        return "MovingForward";
    case domain::RobotMotionState::Stopping:
        return "Stopping";
    case domain::RobotMotionState::Turning:
        return "Turning";
    case domain::RobotMotionState::BackingUp:
        return "BackingUp";
    default:
        return "Idle";
    }
}

std::string CleaningModeToString(domain::CleaningMode mode)
{
    switch (mode)
    {
    case domain::CleaningMode::Normal:
        return "Normal";
    case domain::CleaningMode::Boost:
        return "Boost";
    case domain::CleaningMode::Off:
        return "Off";
    default:
        return "Unknown";
    }
}

std::string MotionCommandToString(domain::MotionCommand command)
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

std::string CleaningCommandToString(domain::CleaningCommand command)
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

std::string StateToJson(const simulator::SimulatorState& state)
{
    std::ostringstream json;
    json << "{"
         << "\"running\":" << (state.running ? "true" : "false") << ","
         << "\"motion_state\":\"" << MotionStateToString(state.motion_state) << "\","
         << "\"cleaning_mode\":\"" << CleaningModeToString(state.cleaning_mode) << "\","
         << "\"last_motion\":\"" << MotionCommandToString(state.last_motion) << "\","
         << "\"last_clean\":\"" << CleaningCommandToString(state.last_clean) << "\""
         << "}";
    return json.str();
}

bool ParseBool(const std::string& body, const std::string& key)
{
    const std::string pattern = "\"" + key + "\":";
    const auto pos = body.find(pattern);
    if (pos == std::string::npos)
    {
        return false;
    }

    const auto value_pos = body.find("true", pos);
    return value_pos != std::string::npos && value_pos < pos + pattern.size() + 8;
}

std::string ParseString(const std::string& body, const std::string& key, const std::string& default_value)
{
    const std::string pattern = "\"" + key + "\":\"";
    const auto pos = body.find(pattern);
    if (pos == std::string::npos)
    {
        return default_value;
    }

    const auto start = pos + pattern.size();
    const auto end = body.find('"', start);
    if (end == std::string::npos)
    {
        return default_value;
    }

    return body.substr(start, end - start);
}

int ParseInt(const std::string& body, const std::string& key, int default_value = 0)
{
    const std::string pattern = "\"" + key + "\":";
    const auto pos = body.find(pattern);
    if (pos == std::string::npos)
    {
        return default_value;
    }

    const auto start = pos + pattern.size();
    const auto end = body.find_first_of(",}", start);
    const std::string value = body.substr(start, end - start);

    try
    {
        return std::stoi(value);
    }
    catch (...)
    {
        return default_value;
    }
}

domain::SensorSnapshot ParseSensors(const std::string& body)
{
    domain::SensorSnapshot sensors;
    sensors.front_blocked = ParseBool(body, "front_blocked");
    sensors.left_blocked = ParseBool(body, "left_blocked");
    sensors.right_blocked = ParseBool(body, "right_blocked");
    sensors.dust_detected = ParseBool(body, "dust_detected");
    return sensors;
}

std::string StepResultToJson(const simulator::ScenarioStepResult& step)
{
    std::ostringstream json;
    json << "{"
         << "\"step\":" << step.step_index << ","
         << "\"expected_motion\":\"" << step.expected_motion << "\","
         << "\"actual_motion\":\"" << step.actual_motion << "\","
         << "\"expected_clean\":\"" << step.expected_clean << "\","
         << "\"actual_clean\":\"" << step.actual_clean << "\","
         << "\"passed\":" << (step.passed ? "true" : "false")
         << "}";
    return json.str();
}

std::string ScenarioResultToJson(const simulator::ScenarioResult& scenario)
{
    std::ostringstream json;
    json << "{"
         << "\"id\":\"" << scenario.id << "\","
         << "\"name\":\"" << scenario.name << "\","
         << "\"passed\":" << (scenario.passed ? "true" : "false") << ","
         << "\"message\":\"" << scenario.message << "\","
         << "\"steps\":[";
    for (std::size_t i = 0; i < scenario.steps.size(); ++i)
    {
        if (i > 0)
        {
            json << ",";
        }
        json << StepResultToJson(scenario.steps[i]);
    }
    json << "]}";
    return json.str();
}

std::string SummaryToJson(const simulator::ScenarioRunSummary& summary)
{
    std::ostringstream json;
    json << "{"
         << "\"total\":" << summary.total << ","
         << "\"passed\":" << summary.passed << ","
         << "\"failed\":" << summary.failed << ","
         << "\"scenarios\":[";
    for (std::size_t i = 0; i < summary.scenarios.size(); ++i)
    {
        if (i > 0)
        {
            json << ",";
        }
        json << ScenarioResultToJson(summary.scenarios[i]);
    }
    json << "]}";
    return json.str();
}

}  // namespace

int main()
{
    simulator::SimulatorService service;
    httplib::Server server;

    server.set_mount_point("/", RVC_WEB_ROOT);

    server.Post("/api/reset", [&](const httplib::Request&, httplib::Response& res)
    {
        service.Reset();
        res.set_content("{\"ok\":true}", "application/json");
    });

    server.Post("/api/start", [&](const httplib::Request& req, httplib::Response& res)
    {
        const std::string turn_strategy = ParseString(req.body, "turn_strategy", "prefer_left");
        service.Start(turn_strategy);
        res.set_content(StateToJson(service.GetState()), "application/json");
    });

    server.Post("/api/tick", [&](const httplib::Request& req, httplib::Response& res)
    {
        const domain::SensorSnapshot sensors = ParseSensors(req.body);
        const int timer_advance_ms = ParseInt(req.body, "timer_advance_ms", 0);
        const simulator::SimulatorState state = service.Tick(
            sensors, std::chrono::milliseconds(timer_advance_ms));
        res.set_content(StateToJson(state), "application/json");
    });

    server.Get("/api/state", [&](const httplib::Request&, httplib::Response& res)
    {
        res.set_content(StateToJson(service.GetState()), "application/json");
    });

    server.Get("/api/scenarios", [&](const httplib::Request&, httplib::Response& res)
    {
        res.set_content(simulator::ScenarioDefinitionsToJson(), "application/json; charset=utf-8");
    });

    server.Post("/api/scenarios/run", [&](const httplib::Request&, httplib::Response& res)
    {
        const simulator::ScenarioRunner runner;
        const simulator::ScenarioRunSummary summary = runner.RunAll();
        res.set_content(SummaryToJson(summary), "application/json");
    });

    const int port = 8080;
    std::cout << "RVC Simulator running at http://localhost:" << port << "\n";
    server.listen("0.0.0.0", port);
    return 0;
}
