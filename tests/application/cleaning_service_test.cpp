#include <chrono>
#include <gtest/gtest.h>

#include "adapters/in_memory/fake_timer.hpp"
#include "adapters/in_memory/in_memory_cleaning_actuator.hpp"
#include "adapters/in_memory/in_memory_motion_actuator.hpp"
#include "adapters/in_memory/in_memory_sensor_reader.hpp"
#include "adapters/in_memory/prefer_left_turn_strategy.hpp"
#include "application/cleaning_service.hpp"
#include "domain/motion_command.hpp"
#include "tests/application/test_fakes.hpp"

using namespace rvc;
using namespace std::chrono_literals;

namespace
{

domain::SensorSnapshot Clear()
{
    return {};
}

domain::SensorSnapshot FrontBlocked()
{
    domain::SensorSnapshot sensors;
    sensors.front_blocked = true;
    return sensors;
}

domain::SensorSnapshot AllBlocked()
{
    domain::SensorSnapshot sensors;
    sensors.front_blocked = true;
    sensors.left_blocked = true;
    sensors.right_blocked = true;
    return sensors;
}

}  // namespace

class CleaningServiceTest : public ::testing::Test
{
protected:
    adapters::InMemorySensorReader sensor_reader_;
    adapters::InMemoryMotionActuator motion_actuator_;
    adapters::InMemoryCleaningActuator cleaning_actuator_;
    adapters::FakeTimer timer_;
    adapters::PreferLeftTurnStrategy turn_strategy_;
};

TEST_F(CleaningServiceTest, StartAndTick_MovesForwardWithNormalCleaning)
{
    application::CleaningService service(
        sensor_reader_, motion_actuator_, cleaning_actuator_, timer_, turn_strategy_);

    sensor_reader_.SetSnapshots({Clear()});
    service.Start();
    service.Tick();

    ASSERT_EQ(motion_actuator_.History().size(), 1u);
    EXPECT_EQ(motion_actuator_.History().front(), domain::MotionCommand::MoveForward);
    ASSERT_EQ(cleaning_actuator_.History().size(), 1u);
    EXPECT_EQ(cleaning_actuator_.History().front(), domain::CleaningCommand::SetNormal);
}

TEST_F(CleaningServiceTest, FrontObstacle_SequenceStopTurnForward)
{
    application::CleaningService service(
        sensor_reader_, motion_actuator_, cleaning_actuator_, timer_, turn_strategy_);

    sensor_reader_.SetSnapshots({
        FrontBlocked(),
        FrontBlocked(),
        Clear(),
    });

    service.Start();
    service.Tick();
    service.Tick();
    service.Tick();

    ASSERT_EQ(motion_actuator_.History().size(), 3u);
    EXPECT_EQ(motion_actuator_.History().at(0), domain::MotionCommand::Stop);
    EXPECT_EQ(motion_actuator_.History().at(1), domain::MotionCommand::TurnLeft);
    EXPECT_EQ(motion_actuator_.History().at(2), domain::MotionCommand::MoveForward);
}

TEST_F(CleaningServiceTest, DeadEnd_SequenceBackUpThenTurn)
{
    application::CleaningService service(
        sensor_reader_, motion_actuator_, cleaning_actuator_, timer_, turn_strategy_);

    sensor_reader_.SetSnapshots({
        AllBlocked(),
        AllBlocked(),
    });

    service.Start();
    service.Tick();
    service.Tick();

    ASSERT_EQ(motion_actuator_.History().size(), 2u);
    EXPECT_EQ(motion_actuator_.History().at(0), domain::MotionCommand::MoveBackward);
    EXPECT_EQ(motion_actuator_.History().at(1), domain::MotionCommand::TurnLeft);
}

TEST_F(CleaningServiceTest, DustDetected_StartsBoostTimer)
{
    application::CleaningService service(
        sensor_reader_, motion_actuator_, cleaning_actuator_, timer_, turn_strategy_, 3s);

    domain::SensorSnapshot dusty;
    dusty.dust_detected = true;
    sensor_reader_.SetSnapshots({dusty});

    service.Start();
    service.Tick();

    ASSERT_EQ(cleaning_actuator_.History().size(), 1u);
    EXPECT_EQ(cleaning_actuator_.History().front(), domain::CleaningCommand::SetBoost);
    EXPECT_FALSE(timer_.IsBoostExpired());

    timer_.Advance(3s);
    sensor_reader_.SetSnapshots({Clear(), Clear()});
    sensor_reader_.Reset();
    service.Tick();

    ASSERT_GE(cleaning_actuator_.History().size(), 2u);
    EXPECT_EQ(cleaning_actuator_.History().back(), domain::CleaningCommand::SetNormal);
}

TEST_F(CleaningServiceTest, FixedRightTurnStrategy_TurnsRight)
{
    tests::FixedTurnStrategy right_strategy(domain::TurnDirection::Right);
    application::CleaningService service(
        sensor_reader_, motion_actuator_, cleaning_actuator_, timer_, right_strategy);

    sensor_reader_.SetSnapshots({
        FrontBlocked(),
        FrontBlocked(),
    });

    service.Start();
    service.Tick();
    service.Tick();

    EXPECT_EQ(motion_actuator_.History().at(1), domain::MotionCommand::TurnRight);
}
