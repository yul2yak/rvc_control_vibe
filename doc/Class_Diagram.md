# Class Diagram — RVC Control SW

현재 구현 클래스·인터페이스 관계를 레이어별로 정리합니다.  
네임스페이스: `rvc::domain`, `rvc::ports`, `rvc::application`, `rvc::adapters`, `rvc::simulator`

---

## 레이어 개요

```mermaid
flowchart TB
    subgraph driving [Driving Adapters]
        CLI[rvc_cli main]
        HTTP[main.cpp httplib]
        SimSvc[SimulatorService]
        ScnRun[ScenarioRunner]
    end

    subgraph app [Application]
        CS[CleaningService]
    end

    subgraph domain [Domain]
        SM[CleaningStateMachine]
        Types[Value Types / Enums]
    end

    subgraph ports_layer [Ports]
        P1[ISensorReader]
        P2[IMotionActuator]
        P3[ICleaningActuator]
        P4[ITimer]
        P5[ITurnStrategy]
        P6[ICleaningSession]
    end

    subgraph driven [Driven Adapters]
        Adapters[in_memory adapters]
    end

    CLI --> CS
    HTTP --> SimSvc
    HTTP --> ScnRun
    SimSvc --> CS
    ScnRun --> CS

    CS --> SM
    CS --> P1
    CS --> P2
    CS --> P3
    CS --> P4
    CS --> P5

    P1 -.-> Adapters
    P2 -.-> Adapters
    P3 -.-> Adapters
    P4 -.-> Adapters
    P5 -.-> Adapters
    P6 -.-> Adapters

    SM --> Types
```

---

## Domain — 값 객체·열거형

```mermaid
classDiagram
    class SensorSnapshot {
        +bool front_blocked
        +bool left_blocked
        +bool right_blocked
        +bool dust_detected
    }

    class RobotMotionState {
        <<enumeration>>
        Idle
        MovingForward
        Stopping
        Turning
        BackingUp
    }

    class CleaningMode {
        <<enumeration>>
        Off
        Normal
        Boost
    }

    class MotionCommand {
        <<enumeration>>
        None
        MoveForward
        Stop
        TurnLeft
        TurnRight
        MoveBackward
    }

    class CleaningCommand {
        <<enumeration>>
        None
        SetOff
        SetNormal
        SetBoost
    }

    class TurnDirection {
        <<enumeration>>
        Left
        Right
    }
```

---

## Domain — 상태 머신

```mermaid
classDiagram
    class TickContext {
        +SensorSnapshot sensors
        +TurnDirection turn_direction
        +bool boost_timer_expired
    }

    class TickResult {
        +RobotMotionState next_motion_state
        +CleaningMode next_cleaning_mode
        +MotionCommand motion
        +CleaningCommand clean
    }

    class CleaningStateMachine {
        -RobotMotionState motion_state_
        -CleaningMode cleaning_mode_
        -TurnDirection active_turn_direction_
        +CleaningStateMachine()
        +Start()
        +Tick(TickContext) TickResult
        +MotionState() RobotMotionState
        +CleaningModeState() CleaningMode
        +ActiveTurnDirection() TurnDirection
        -HandleMovingForward(TickContext) TickResult
        -HandleStopping(TickContext) TickResult
        -HandleBackingUp(TickContext) TickResult
        -HandleTurning(TickContext) TickResult
        -ApplyCleaningBoost(TickContext, TickResult) TickResult
        -ResolveCleaningDuringManeuver(TickContext) CleaningCommand
    }

    CleaningStateMachine ..> TickContext : uses
    CleaningStateMachine ..> TickResult : produces
    CleaningStateMachine ..> SensorSnapshot
    CleaningStateMachine ..> RobotMotionState
    CleaningStateMachine ..> CleaningMode
    CleaningStateMachine ..> MotionCommand
    CleaningStateMachine ..> CleaningCommand
```

**상태 전이 요약**

| 현재 상태 | 조건 | 다음 상태 | 대표 명령 |
|-----------|------|-----------|-----------|
| `Idle` | `Start()` | `MovingForward` | — |
| `MovingForward` | 전방 막힘 | `Stopping` | `Stop` |
| `MovingForward` | 삼방 막힘 | `BackingUp` | `MoveBackward` |
| `MovingForward` | 통로 확보 | `MovingForward` | `MoveForward` |
| `Stopping` | — | `Turning` | `TurnLeft`/`TurnRight` |
| `BackingUp` | — | `Turning` | `TurnLeft`/`TurnRight` |
| `Turning` | 전방 확보 | `MovingForward` | `MoveForward` |
| `Turning` | 전방 막힘 | `Turning` | `TurnLeft`/`TurnRight` |

---

## Ports — 인터페이스

```mermaid
classDiagram
    class ISensorReader {
        <<interface>>
        +Read() SensorSnapshot
    }

    class IMotionActuator {
        <<interface>>
        +Execute(MotionCommand)
    }

    class ICleaningActuator {
        <<interface>>
        +Execute(CleaningCommand)
    }

    class ITimer {
        <<interface>>
        +StartBoost(milliseconds)
        +IsBoostExpired() bool
        +CancelBoost()
    }

    class ITurnStrategy {
        <<interface>>
        +Choose(SensorSnapshot) TurnDirection
    }

    class ICleaningSession {
        <<interface>>
        +Start()
        +Tick()
        +IsRunning() bool
    }

    ISensorReader ..> SensorSnapshot
    IMotionActuator ..> MotionCommand
    ICleaningActuator ..> CleaningCommand
    ITurnStrategy ..> SensorSnapshot
    ITurnStrategy ..> TurnDirection
```

---

## Application

```mermaid
classDiagram
    class CleaningService {
        -ISensorReader& sensor_reader_
        -IMotionActuator& motion_actuator_
        -ICleaningActuator& cleaning_actuator_
        -ITimer& timer_
        -ITurnStrategy& turn_strategy_
        -milliseconds boost_duration_
        -CleaningStateMachine state_machine_
        -bool running_
        +CleaningService(...)
        +Start()
        +Tick()
        +IsRunning() bool
        +MotionState() RobotMotionState
        +CleaningModeState() CleaningMode
    }

    CleaningService *-- CleaningStateMachine : owns
    CleaningService --> ISensorReader
    CleaningService --> IMotionActuator
    CleaningService --> ICleaningActuator
    CleaningService --> ITimer
    CleaningService --> ITurnStrategy
```

`CleaningService`는 **유일한 애플리케이션 서비스**이며, 포트를 조합해 1틱 오케스트레이션을 담당합니다.

---

## Driven Adapters (`src/adapters/in_memory/`)

```mermaid
classDiagram
    class InMemorySensorReader {
        -vector~SensorSnapshot~ snapshots_
        -size_t index_
        +SetSnapshots(vector)
        +Reset()
        +Read() SensorSnapshot
    }

    class InMemoryMotionActuator {
        -vector~MotionCommand~ history_
        +Execute(MotionCommand)
        +History() vector
        +Clear()
    }

    class InMemoryCleaningActuator {
        -vector~CleaningCommand~ history_
        +Execute(CleaningCommand)
        +History() vector
        +Clear()
    }

    class FakeTimer {
        -milliseconds boost_end_
        -bool active_
        +StartBoost(milliseconds)
        +IsBoostExpired() bool
        +CancelBoost()
        +Advance(milliseconds)
    }

    class PreferLeftTurnStrategy {
        +Choose(SensorSnapshot) TurnDirection
    }

    class FixedTurnStrategy {
        +Choose(SensorSnapshot) TurnDirection
    }

    class CleaningSessionAdapter {
        -CleaningService& service_
        +Start()
        +Tick()
        +IsRunning() bool
    }

    ISensorReader <|.. InMemorySensorReader
    IMotionActuator <|.. InMemoryMotionActuator
    ICleaningActuator <|.. InMemoryCleaningActuator
    ITimer <|.. FakeTimer
    ITurnStrategy <|.. PreferLeftTurnStrategy
    ITurnStrategy <|.. FixedTurnStrategy
    ICleaningSession <|.. CleaningSessionAdapter
    CleaningSessionAdapter --> CleaningService
```

| 클래스 | 역할 |
|--------|------|
| `InMemorySensorReader` | Tick마다 주입된 센서 스냅샷 반환 |
| `InMemoryMotionActuator` | 동작 명령 기록 (HW 대체) |
| `InMemoryCleaningActuator` | 청소 명령 기록 (HW 대체) |
| `FakeTimer` | 부스트 타이머 시뮬레이션 (`Advance`로 테스트 가속) |
| `PreferLeftTurnStrategy` | 좌측 통로 우선 회전 |
| `FixedTurnStrategy` | 항상 우회전 |
| `CleaningSessionAdapter` | `ICleaningSession` 포트 → `CleaningService` 위임 |

---

## Simulator (`simulator/server/`)

```mermaid
classDiagram
    class SimulatorState {
        +RobotMotionState motion_state
        +CleaningMode cleaning_mode
        +MotionCommand last_motion
        +CleaningCommand last_clean
        +bool running
    }

    class SimulatorService {
        -mutex mutex_
        -InMemorySensorReader sensor_reader_
        -InMemoryMotionActuator motion_actuator_
        -InMemoryCleaningActuator cleaning_actuator_
        -FakeTimer timer_
        -PreferLeftTurnStrategy prefer_left_strategy_
        -FixedTurnStrategy force_right_strategy_
        -SwitchableTurnStrategy switchable_turn_strategy_
        -CleaningService cleaning_service_
        +Reset()
        +Start(turn_strategy)
        +Tick(sensors, timer_advance) SimulatorState
        +GetState() SimulatorState
    }

    class SwitchableTurnStrategy {
        -ITurnStrategy* active_strategy_
        +SetActive(ITurnStrategy&)
        +Choose(SensorSnapshot) TurnDirection
    }

    class ScenarioDefinition {
        +string id
        +string name
        +ScenarioTurnStrategy turn_strategy
        +vector~ScenarioStepDefinition~ steps
    }

    class ScenarioStepDefinition {
        +SensorSnapshot sensors
        +MotionCommand expected_motion
        +CleaningCommand expected_clean
        +milliseconds timer_advance
    }

    class ScenarioRunner {
        +RunAll() ScenarioRunSummary
    }

    class ScenarioResult {
        +string id
        +string name
        +bool passed
        +string message
        +vector~ScenarioStepResult~ steps
    }

    SimulatorService *-- CleaningService
    SimulatorService *-- InMemorySensorReader
    SimulatorService *-- InMemoryMotionActuator
    SimulatorService *-- InMemoryCleaningActuator
    SimulatorService *-- FakeTimer
    SimulatorService *-- SwitchableTurnStrategy
    SwitchableTurnStrategy --> ITurnStrategy
    ScenarioRunner ..> ScenarioDefinition
    ScenarioRunner ..> CleaningService : creates per scenario
    ScenarioDefinition *-- ScenarioStepDefinition
```

---

## Driving Adapters

| 진입점 | 파일 | 의존 |
|--------|------|------|
| `rvc_cli` | `src/adapters/cli/main.cpp` | `rvc_adapters` |
| `rvc_simulator` | `simulator/server/main.cpp` | `SimulatorService`, `ScenarioRunner`, httplib |
| `rvc_tests` | `tests/domain/*`, `tests/application/*` | `rvc_application`, `rvc_adapters`, GTest |
| `rvc_scenario_tests` | `scenario_test_main.cpp` | 시나리오 러너 단독 실행 |

---

## 의존성 규칙 (헥사고날)

```mermaid
flowchart LR
    D[domain] --> |no outward deps| D
    P[ports] --> D
    A[application] --> D
    A --> P
    AD[adapters] --> A
    AD --> P
    DRV[driving adapters] --> AD
    DRV --> A
```

- **Domain**은 표준 라이브러리 외 의존 없음.
- **Ports**는 Domain 타입만 참조 (INTERFACE 라이브러리 `rvc_ports`).
- **Application**은 Ports·Domain만 참조.
- **Adapters**가 Ports를 구현; CLI·시뮬레이터·테스트가 Application을 호출.

---

## 관련 문서

- [UseCase_Diagram.md](UseCase_Diagram.md)
- [Sequence_Diagram.md](Sequence_Diagram.md)
- [Module_View.md](Module_View.md)
