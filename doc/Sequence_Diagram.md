# Sequence Diagram — RVC Control SW

현재 구현의 대표 실행 흐름을 시퀀스 다이어그램으로 정리합니다.

---

## 1. CleaningService — 1틱 공통 흐름

모든 진입점(CLI, 시뮬레이터 수동/자동, 유닛 테스트)이 공유하는 핵심 시퀀스입니다.

```mermaid
sequenceDiagram
    participant Caller as 호출자
    participant CS as CleaningService
    participant SR as ISensorReader
    participant TS as ITurnStrategy
    participant TM as ITimer
    participant SM as CleaningStateMachine
    participant MA as IMotionActuator
    participant CA as ICleaningActuator

    Caller->>CS: Tick()

    CS->>SR: Read()
    SR-->>CS: SensorSnapshot

    CS->>TS: Choose(sensors)
    TS-->>CS: TurnDirection

    CS->>TM: IsBoostExpired()
    TM-->>CS: bool

    CS->>SM: Tick(TickContext)
    SM-->>CS: TickResult<br/>(motion, clean, states)

    alt motion != None
        CS->>MA: Execute(motion)
    end

    alt clean != None
        CS->>CA: Execute(clean)
        alt clean == SetBoost
            CS->>TM: StartBoost(3s)
        else clean == SetOff or SetNormal
            CS->>TM: CancelBoost()
        end
    end
```

**코드 위치**: `src/application/cleaning_service.cpp`

---

## 2. 무장애물 직진 청소

```mermaid
sequenceDiagram
    participant Op as 운영자
    participant UI as app.js
    participant HTTP as main.cpp
    participant Sim as SimulatorService
    participant CS as CleaningService
    participant SM as CleaningStateMachine

    Op->>UI: 시작 클릭
    UI->>HTTP: POST /api/start
    HTTP->>Sim: Start()
    Sim->>CS: Start()
    CS->>SM: Start()
    Note over SM: MovingForward / Normal

    loop 0.5초 간격 자동 Tick
        UI->>UI: readSensorsFromGrid()
        UI->>HTTP: POST /api/tick {sensors}
        HTTP->>Sim: Tick(sensors)
        Sim->>CS: Tick()
        CS->>SM: Tick(clear path)
        SM-->>CS: MoveForward, SetNormal
        CS-->>Sim: last_motion, last_clean
        Sim-->>HTTP: SimulatorState JSON
        HTTP-->>UI: JSON
        UI->>UI: applyMotion(MoveForward)
        UI->>UI: draw()
    end
```

**조건**: `front_blocked == false`, `dust_detected == false`

---

## 3. 전방 장애물 회피 (정지 → 회전 → 재개)

```mermaid
sequenceDiagram
    participant CS as CleaningService
    participant TS as PreferLeftTurnStrategy
    participant SM as CleaningStateMachine
    participant MA as IMotionActuator
    participant CA as ICleaningActuator

    Note over SM: MovingForward

    CS->>SM: Tick(front_blocked=true)
    SM-->>CS: Stop, SetOff<br/>Stopping
    CS->>MA: Execute(Stop)
    CS->>CA: Execute(SetOff)

    CS->>TS: Choose(sensors)
    TS-->>CS: Left
    CS->>SM: Tick(front_blocked=true)
    SM-->>CS: TurnLeft, SetOff<br/>Turning
    CS->>MA: Execute(TurnLeft)

    loop 전방이 막힌 동안
        CS->>SM: Tick(front_blocked=true)
        SM-->>CS: TurnLeft, SetOff
        CS->>MA: Execute(TurnLeft)
    end

    CS->>SM: Tick(front_blocked=false)
    SM-->>CS: MoveForward, SetNormal<br/>MovingForward
    CS->>MA: Execute(MoveForward)
    CS->>CA: Execute(SetNormal)
```

**회전 방향**: `PreferLeftTurnStrategy` — 좌측 비막힘이면 `Left`, 아니면 `Right`

---

## 4. 먼지 감지 부스트 (타이머 만료 포함)

```mermaid
sequenceDiagram
    participant CS as CleaningService
    participant SM as CleaningStateMachine
    participant CA as ICleaningActuator
    participant TM as FakeTimer

    Note over SM: MovingForward / Normal

    CS->>SM: Tick(dust_detected=true)
    SM-->>CS: MoveForward, SetBoost<br/>Boost
    CS->>CA: Execute(SetBoost)
    CS->>TM: StartBoost(3000ms)

    loop 부스트 유지 (dust 또는 타이머 미만료)
        CS->>TM: IsBoostExpired()
        TM-->>CS: false
        CS->>SM: Tick(...)
        SM-->>CS: MoveForward, SetBoost
        CS->>CA: Execute(SetBoost)
    end

    CS->>TM: IsBoostExpired()
    TM-->>CS: true
    CS->>SM: Tick(clear, boost_timer_expired=true)
    SM-->>CS: MoveForward, SetNormal<br/>Normal
    CS->>CA: Execute(SetNormal)
    CS->>TM: CancelBoost()
```

**부스트 기본 시간**: `std::chrono::seconds(3)` (`CleaningService` 생성자 기본값)

---

## 5. 장애물 앞 먼지 칸 — 회전 중 부스트 유지

먼지 칸에 도착한 틱에 전방 장애물도 감지될 때, 동작은 정지·회전하되 청소는 부스트로 유지합니다.

```mermaid
sequenceDiagram
    participant UI as app.js
    participant CS as CleaningService
    participant SM as CleaningStateMachine
    participant CA as ICleaningActuator
    participant TM as FakeTimer

    Note over UI: 로봇이 먼지 칸 도착<br/>전방에 장애물

    UI->>CS: Tick(dust=true, front_blocked=true)
    CS->>SM: Tick(context)
    SM-->>CS: Stop, SetBoost<br/>Stopping / Boost
    CS->>CA: Execute(SetBoost)
    CS->>TM: StartBoost(3s)
    UI->>UI: applyCleaningEffect(SetBoost)<br/>먼지 칸 제거

    CS->>SM: Tick(front_blocked=true, dust=false)
    SM-->>CS: TurnLeft, SetBoost<br/>Turning / Boost
    CS->>CA: Execute(SetBoost)

    CS->>SM: Tick(front_blocked=false)
    SM-->>CS: MoveForward, SetNormal or SetBoost
```

**구현**: `CleaningStateMachine::ResolveCleaningDuringManeuver()`  
**코드**: `src/domain/cleaning_state_machine.cpp`

---

## 6. 삼방 막힘 — 후진 → 회전 → 재개

```mermaid
sequenceDiagram
    participant CS as CleaningService
    participant SM as CleaningStateMachine
    participant MA as IMotionActuator

    Note over SM: MovingForward

    CS->>SM: Tick(F+L+R blocked)
    SM-->>CS: MoveBackward, SetOff<br/>BackingUp
    CS->>MA: Execute(MoveBackward)

    CS->>SM: Tick(F+L+R blocked)
    SM-->>CS: TurnLeft, SetOff<br/>Turning
    CS->>MA: Execute(TurnLeft)

    CS->>SM: Tick(front clear)
    SM-->>CS: MoveForward, SetNormal<br/>MovingForward
    CS->>MA: Execute(MoveForward)
```

---

## 7. 시뮬레이터 수동 모드 — Start ~ Tick

```mermaid
sequenceDiagram
    participant Browser as 브라우저
    participant HTTP as main.cpp
    participant Sim as SimulatorService
    participant SR as InMemorySensorReader
    participant CS as CleaningService

    Browser->>HTTP: POST /api/reset
    HTTP->>Sim: Reset()
    Sim->>SR: Reset()
    Sim->>CS: (어댑터 Clear)

    Browser->>HTTP: POST /api/start {turn_strategy?}
    HTTP->>Sim: Start(turn_strategy)
    Sim->>Sim: SwitchableTurnStrategy 설정
    Sim->>CS: Start()

    Browser->>Browser: readSensorsFromGrid()
    Browser->>HTTP: POST /api/tick JSON
    HTTP->>HTTP: ParseBool → SensorSnapshot
    HTTP->>Sim: Tick(sensors, timer_advance)
    Sim->>SR: SetSnapshots({sensors})
    Sim->>CS: Tick()
    CS-->>Sim: motion/clean history
    Sim-->>HTTP: SimulatorState
    HTTP-->>Browser: JSON
    Browser->>Browser: applyTickResult()<br/>로봇 이동·먼지 제거·상태 갱신
```

**주의**: 그리드 좌표·로봇 애니메이션은 `app.js`가 담당하며, C++ 서버는 JSON 센서만 신뢰합니다.

---

## 8. 시나리오 자동 테스트

`ScenarioRunner`는 시나리오마다 **새 어댑터·CleaningService**를 생성하며 `SimulatorService`와 상태를 공유하지 않습니다.

```mermaid
sequenceDiagram
    participant Browser as 브라우저
    participant HTTP as main.cpp
    participant Run as ScenarioRunner
    participant Def as ScenarioDefinitions
    participant CS as CleaningService
    participant Adapters as 인메모리 어댑터

    Browser->>HTTP: POST /api/scenarios/run
    HTTP->>Run: RunAll()

    loop 각 ScenarioDefinition
        Run->>Adapters: 새 인스턴스 생성
        Run->>CS: Start()
        loop 각 ScenarioStepDefinition
            Run->>Adapters: sensor_reader.SetSnapshots(step.sensors)
            Run->>Adapters: timer.Advance(step.timer_advance)
            Run->>CS: Tick()
            Run->>Run: actual vs expected<br/>(motion, clean)
        end
        Run->>Run: ScenarioResult 집계
    end

    HTTP-->>Browser: ScenarioRunSummary JSON
```

**5개 시나리오**: `forward_cleaning`, `front_obstacle`, `all_sides_blocked`, `dust_boost`, `right_turn_obstacle`

---

## 9. CLI 시나리오 실행

```mermaid
sequenceDiagram
    participant User as 사용자
    participant CLI as rvc_cli
    participant SR as InMemorySensorReader
    participant CS as CleaningService

    User->>CLI: rvc_cli obstacle
    CLI->>SR: SetSnapshots(시나리오 시퀀스)
    CLI->>CS: Start()

    loop 각 스냅샷
        CLI->>CS: Tick()
        CS-->>CLI: (actuator history)
        CLI->>User: stdout 로그
    end
```

---

## 10. 유닛 테스트 — 도메인 격리

```mermaid
sequenceDiagram
    participant Test as GTest
    participant SM as CleaningStateMachine

    Test->>SM: Start()
    Test->>SM: Tick(MakeContext(sensors))
    SM-->>Test: TickResult
    Test->>Test: EXPECT motion/clean/state
```

도메인 테스트는 **포트·어댑터 없이** `CleaningStateMachine`만 직접 검증합니다.  
애플리케이션 테스트는 `tests/application/test_fakes.hpp`의 페이크 포트로 `CleaningService`를 검증합니다.

---

## 시퀀스 ↔ 파일 매핑

| 시퀀스 | 주요 파일 |
|--------|-----------|
| 1틱 공통 | `cleaning_service.cpp`, `cleaning_state_machine.cpp` |
| 수동 시뮬레이터 | `main.cpp`, `simulator_service.cpp`, `app.js` |
| 자동 시나리오 | `scenario_runner.cpp`, `scenario_definitions.cpp` |
| CLI | `src/adapters/cli/main.cpp` |
| 유닛 테스트 | `tests/domain/*`, `tests/application/*` |

---

## 관련 문서

- [UseCase_Diagram.md](UseCase_Diagram.md)
- [Class_Diagram.md](Class_Diagram.md)
- [Module_View.md](Module_View.md)
- [Simulator_Architecture.md](Simulator_Architecture.md)
