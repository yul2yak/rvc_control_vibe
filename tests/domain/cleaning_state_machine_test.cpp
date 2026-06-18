#include <gtest/gtest.h>

#include "domain/cleaning_state_machine.hpp"

using namespace rvc::domain;

namespace
{

SensorSnapshot ClearPath()
{
    return {};
}

SensorSnapshot FrontBlockedOnly()
{
    SensorSnapshot sensors;
    sensors.front_blocked = true;
    return sensors;
}

SensorSnapshot AllSidesBlocked()
{
    SensorSnapshot sensors;
    sensors.front_blocked = true;
    sensors.left_blocked = true;
    sensors.right_blocked = true;
    return sensors;
}

TickContext MakeContext(const SensorSnapshot& sensors, TurnDirection turn = TurnDirection::Left)
{
    TickContext context;
    context.sensors = sensors;
    context.turn_direction = turn;
    return context;
}

}  // namespace

TEST(청소상태머신테스트, 전진중_장애물없음_직진청소유지)
{
    CleaningStateMachine machine;
    machine.Start();

    const TickResult result = machine.Tick(MakeContext(ClearPath()));

    EXPECT_EQ(machine.MotionState(), RobotMotionState::MovingForward);
    EXPECT_EQ(machine.CleaningModeState(), CleaningMode::Normal);
    EXPECT_EQ(result.motion, MotionCommand::MoveForward);
    EXPECT_EQ(result.clean, CleaningCommand::SetNormal);
}

TEST(청소상태머신테스트, 전진중_전방장애물_정지후회전)
{
    CleaningStateMachine machine;
    machine.Start();

    const TickResult stop_result = machine.Tick(MakeContext(FrontBlockedOnly()));
    EXPECT_EQ(machine.MotionState(), RobotMotionState::Stopping);
    EXPECT_EQ(stop_result.motion, MotionCommand::Stop);
    EXPECT_EQ(stop_result.clean, CleaningCommand::SetOff);

    const TickResult turn_result = machine.Tick(MakeContext(FrontBlockedOnly()));
    EXPECT_EQ(machine.MotionState(), RobotMotionState::Turning);
    EXPECT_EQ(turn_result.motion, MotionCommand::TurnLeft);
}

TEST(청소상태머신테스트, 회전중_전방확보_직진청소재개)
{
    CleaningStateMachine machine;
    machine.Start();

    machine.Tick(MakeContext(FrontBlockedOnly()));
    machine.Tick(MakeContext(FrontBlockedOnly()));

    const TickResult result = machine.Tick(MakeContext(ClearPath()));

    EXPECT_EQ(machine.MotionState(), RobotMotionState::MovingForward);
    EXPECT_EQ(machine.CleaningModeState(), CleaningMode::Normal);
    EXPECT_EQ(result.motion, MotionCommand::MoveForward);
    EXPECT_EQ(result.clean, CleaningCommand::SetNormal);
}

TEST(청소상태머신테스트, 전진중_삼방막힘_후진후회전)
{
    CleaningStateMachine machine;
    machine.Start();

    const TickResult back_result = machine.Tick(MakeContext(AllSidesBlocked()));
    EXPECT_EQ(machine.MotionState(), RobotMotionState::BackingUp);
    EXPECT_EQ(back_result.motion, MotionCommand::MoveBackward);
    EXPECT_EQ(back_result.clean, CleaningCommand::SetOff);

    const TickResult turn_result = machine.Tick(MakeContext(AllSidesBlocked()));
    EXPECT_EQ(machine.MotionState(), RobotMotionState::Turning);
    EXPECT_EQ(turn_result.motion, MotionCommand::TurnLeft);
}

TEST(청소상태머신테스트, 일반청소중_먼지감지_부스트전환)
{
    CleaningStateMachine machine;
    machine.Start();

    SensorSnapshot dusty_path;
    dusty_path.dust_detected = true;

    const TickResult result = machine.Tick(MakeContext(dusty_path));

    EXPECT_EQ(machine.CleaningModeState(), CleaningMode::Boost);
    EXPECT_EQ(result.clean, CleaningCommand::SetBoost);
}

TEST(청소상태머신테스트, 부스트중_타이머만료_일반청소복귀)
{
    CleaningStateMachine machine;
    machine.Start();

    SensorSnapshot dusty_path;
    dusty_path.dust_detected = true;
    machine.Tick(MakeContext(dusty_path));

    TickContext expired_context = MakeContext(ClearPath());
    expired_context.boost_timer_expired = true;

    const TickResult result = machine.Tick(expired_context);

    EXPECT_EQ(machine.CleaningModeState(), CleaningMode::Normal);
    EXPECT_EQ(result.clean, CleaningCommand::SetNormal);
}

TEST(청소상태머신테스트, 전진중_전방장애물과먼지_정지하며먼지흡입)
{
    CleaningStateMachine machine;
    machine.Start();

    SensorSnapshot sensors = FrontBlockedOnly();
    sensors.dust_detected = true;

    const TickResult result = machine.Tick(MakeContext(sensors));

    EXPECT_EQ(machine.MotionState(), RobotMotionState::Stopping);
    EXPECT_EQ(result.motion, MotionCommand::Stop);
    EXPECT_EQ(machine.CleaningModeState(), CleaningMode::Boost);
    EXPECT_EQ(result.clean, CleaningCommand::SetBoost);
}

TEST(청소상태머신테스트, 회전중_먼지칸_부스트유지)
{
    CleaningStateMachine machine;
    machine.Start();

    SensorSnapshot blocked_with_dust = FrontBlockedOnly();
    blocked_with_dust.dust_detected = true;

    machine.Tick(MakeContext(blocked_with_dust));
    machine.Tick(MakeContext(blocked_with_dust));

    const TickResult result = machine.Tick(MakeContext(blocked_with_dust));

    EXPECT_EQ(machine.MotionState(), RobotMotionState::Turning);
    EXPECT_EQ(result.motion, MotionCommand::TurnLeft);
    EXPECT_EQ(machine.CleaningModeState(), CleaningMode::Boost);
    EXPECT_EQ(result.clean, CleaningCommand::SetBoost);
}

TEST(청소상태머신테스트, 정지후_우회전선택_우회전실행)
{
    CleaningStateMachine machine;
    machine.Start();

    machine.Tick(MakeContext(FrontBlockedOnly()));

    TickContext context = MakeContext(FrontBlockedOnly(), TurnDirection::Right);
    const TickResult result = machine.Tick(context);

    EXPECT_EQ(machine.MotionState(), RobotMotionState::Turning);
    EXPECT_EQ(result.motion, MotionCommand::TurnRight);
    EXPECT_EQ(machine.ActiveTurnDirection(), TurnDirection::Right);
}
