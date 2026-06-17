# 로봇청소기 Control SW (RVC Controller)

자동 청소 제어 소프트웨어입니다. 헥사고날 아키텍처로 구성되어 있으며, Google Test 기반 유닛 테스트와 웹 시뮬레이터를 제공합니다.

## 요구 사항

- **CMake** 3.16 이상
- **C++17** 컴파일러
  - Windows: Visual Studio 2022 Build Tools (MSVC) 권장
- 인터넷 연결 (최초 빌드 시 Google Test, cpp-httplib 자동 다운로드)

## 빌드

프로젝트 루트(`c:\yul2ya`)에서 실행합니다.

### Windows (Visual Studio)

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Debug
```

Release 빌드:

```powershell
cmake --build build --config Release
```

### 빌드 결과물

| 실행 파일 | 경로 (Debug) |
|-----------|----------------|
| 유닛 테스트 | `build\Debug\rvc_tests.exe` |
| CLI | `build\Debug\rvc_cli.exe` |
| 웹 시뮬레이터 | `build\simulator\server\Debug\rvc_simulator.exe` |

## 테스트

### 전체 테스트 (ctest)

`ctest`가 PATH에 없으면 전체 경로를 사용합니다.

```powershell
& "C:\Program Files\CMake\bin\ctest.exe" --test-dir build -C Debug --output-on-failure
```

PATH에 CMake가 등록되어 있다면:

```powershell
ctest --test-dir build -C Debug --output-on-failure
```

### 테스트 실행 파일 직접 실행

```powershell
.\build\Debug\rvc_tests.exe
```

### 특정 테스트만 실행

한글 테스트 이름 필터 예시:

```powershell
# ctest
& "C:\Program Files\CMake\bin\ctest.exe" --test-dir build -C Debug -R "장애물회피우선"

# gtest 필터
.\build\Debug\rvc_tests.exe --gtest_filter="*장애물회피우선*"
```

도메인 테스트 전체:

```powershell
& "C:\Program Files\CMake\bin\ctest.exe" --test-dir build -C Debug -R "청소상태머신"
```

### 빌드 후 테스트 한 번에

```powershell
cmake --build build --config Debug
& "C:\Program Files\CMake\bin\ctest.exe" --test-dir build -C Debug --output-on-failure
```

## CLI 실행

센서 시나리오를 텍스트로 넣어 동작을 확인합니다.

```powershell
.\build\Debug\rvc_cli.exe forward
.\build\Debug\rvc_cli.exe obstacle
.\build\Debug\rvc_cli.exe deadend
.\build\Debug\rvc_cli.exe dust
.\build\Debug\rvc_cli.exe custom clear F clear
```

| 시나리오 | 설명 |
|----------|------|
| `forward` | 장애물 없이 직진 |
| `obstacle` | 전방 장애물 후 회전·재개 |
| `deadend` | 전·좌·우 막힘 후 후진·회전 |
| `dust` | 먼지 감지 시 부스트 |
| `custom` | 틱별 센서 지정 (`clear`, `F`, `L`, `R`, `D` 조합) |

## 웹 시뮬레이터 실행

1. 시뮬레이터 서버를 실행합니다.

```powershell
.\build\simulator\server\Debug\rvc_simulator.exe
```

2. 브라우저에서 접속합니다.

```
http://localhost:8080
```

3. UI에서 **시작** → **Tick**으로 한 단계씩 진행합니다.
   - 캔버스 클릭: 장애물 추가/제거
   - Shift + 클릭: 먼지 구역 추가/제거

### 자동 테스트 모드

수동 Tick과 별도로, UI 우측 상단 **자동 테스트** 탭에서 시나리오를 일괄 실행할 수 있습니다.

| 시나리오 | 검증 내용 |
|----------|-----------|
| 직진 청소 | 무장애물 전진 + 일반 청소 |
| 전방 장애물 회피 | 정지 → 좌회전 → 재개 |
| 전/좌/우 막힘 | 후진 → 회전 → 재개 |
| 먼지 흡입 및 부스트 | 먼지 감지 부스트 → 3초 후 일반 청소 |
| 전방 장애물 시 우회전 | 정지 → 우회전 |

API로 직접 실행:

```powershell
Invoke-RestMethod -Method Post -Uri "http://localhost:8080/api/scenarios/run"
```

### REST API

| 메서드 | 경로 | 설명 |
|--------|------|------|
| `POST` | `/api/start` | 청소 세션 시작 |
| `POST` | `/api/tick` | 센서 값으로 1틱 실행 (JSON body) |
| `POST` | `/api/scenarios/run` | 시나리오 자동 테스트 전체 실행 |
| `POST` | `/api/reset` | 상태 초기화 |
| `GET` | `/api/state` | 현재 상태 조회 |

`/api/tick` 요청 body 예시:

```json
{
  "front_blocked": true,
  "left_blocked": false,
  "right_blocked": false,
  "dust_detected": false
}
```

## 프로젝트 구조

```
src/
  domain/        # 핵심 상태 머신 (외부 의존 없음)
  application/   # CleaningService
  ports/         # 인터페이스 (포트)
  adapters/      # 인메모리 어댑터, CLI
tests/           # Google Test 유닛 테스트
simulator/
  server/        # HTTP API 서버
  web/           # 시뮬레이터 UI
doc/             # 요구사항 문서
```

## 참고

- 요구사항: [doc/Preliminary_Requirements.md](doc/Preliminary_Requirements.md)
- 테스트 소스는 UTF-8이며, MSVC 빌드 시 `/utf-8` 옵션이 적용됩니다.
- CMake·Visual Studio 설치 후에는 **새 PowerShell 창**을 열어 PATH가 반영됐는지 확인하세요.
