#include <iostream>
#include <string>
#include <vector>

#include "adapters/in_memory/cleaning_session_adapter.hpp"
#include "adapters/in_memory/fake_timer.hpp"
#include "adapters/in_memory/in_memory_cleaning_actuator.hpp"
#include "adapters/in_memory/in_memory_motion_actuator.hpp"
#include "adapters/in_memory/in_memory_sensor_reader.hpp"
#include "adapters/in_memory/prefer_left_turn_strategy.hpp"
#include "application/cleaning_service.hpp"
#include "domain/motion_command.hpp"
#include "domain/robot_state.hpp"

using namespace rvc;

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

std::string CleaningToString(domain::CleaningCommand command)
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

domain::SensorSnapshot ParseSnapshot(const std::string& token)
{
    domain::SensorSnapshot sensors;

    if (token.find('F') != std::string::npos)
    {
        sensors.front_blocked = true;
    }
    if (token.find('L') != std::string::npos)
    {
        sensors.left_blocked = true;
    }
    if (token.find('R') != std::string::npos)
    {
        sensors.right_blocked = true;
    }
    if (token.find('D') != std::string::npos)
    {
        sensors.dust_detected = true;
    }

    return sensors;
}

void PrintUsage()
{
    std::cout << "Usage: rvc_cli <scenario>\n"
              << "Scenarios:\n"
              << "  forward       - clear path forward\n"
              << "  obstacle      - front obstacle then clear\n"
              << "  deadend       - blocked on all sides then clear\n"
              << "  dust          - dust detected then clear\n"
              << "  custom <ticks>  e.g. custom clear F clear\n"
              << "Tick tokens: clear, F (front), L, R, D (dust), combinations e.g. FLR\n";
}

}  // namespace

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        PrintUsage();
        return 1;
    }

    std::vector<domain::SensorSnapshot> snapshots;
    const std::string scenario = argv[1];

    if (scenario == "forward")
    {
        snapshots = {{}, {}, {}};
    }
    else if (scenario == "obstacle")
    {
        snapshots = {ParseSnapshot("F"), ParseSnapshot("F"), {}};
    }
    else if (scenario == "deadend")
    {
        snapshots = {ParseSnapshot("FLR"), ParseSnapshot("FLR"), {}};
    }
    else if (scenario == "dust")
    {
        snapshots = {ParseSnapshot("D"), {}, {}};
    }
    else if (scenario == "custom")
    {
        for (int i = 2; i < argc; ++i)
        {
            const std::string token = argv[i];
            if (token == "clear")
            {
                snapshots.push_back({});
            }
            else
            {
                snapshots.push_back(ParseSnapshot(token));
            }
        }
    }
    else
    {
        PrintUsage();
        return 1;
    }

    adapters::InMemorySensorReader sensor_reader;
    adapters::InMemoryMotionActuator motion_actuator;
    adapters::InMemoryCleaningActuator cleaning_actuator;
    adapters::FakeTimer timer;
    adapters::PreferLeftTurnStrategy turn_strategy;

    application::CleaningService service(
        sensor_reader, motion_actuator, cleaning_actuator, timer, turn_strategy);
    adapters::CleaningSessionAdapter session(service);

    sensor_reader.SetSnapshots(snapshots);
    session.Start();

    std::cout << "Scenario: " << scenario << "\n";

    for (std::size_t i = 0; i < snapshots.size(); ++i)
    {
        const std::size_t motion_before = motion_actuator.History().size();
        const std::size_t clean_before = cleaning_actuator.History().size();

        session.Tick();

        std::cout << "Tick " << (i + 1) << ":";

        if (motion_actuator.History().size() > motion_before)
        {
            std::cout << " motion=" << MotionToString(motion_actuator.History().back());
        }
        if (cleaning_actuator.History().size() > clean_before)
        {
            std::cout << " clean=" << CleaningToString(cleaning_actuator.History().back());
        }

        std::cout << " state=";

        switch (service.MotionState())
        {
        case domain::RobotMotionState::MovingForward:
            std::cout << "MovingForward";
            break;
        case domain::RobotMotionState::Stopping:
            std::cout << "Stopping";
            break;
        case domain::RobotMotionState::Turning:
            std::cout << "Turning";
            break;
        case domain::RobotMotionState::BackingUp:
            std::cout << "BackingUp";
            break;
        default:
            std::cout << "Idle";
            break;
        }

        std::cout << "\n";
    }

    return 0;
}
