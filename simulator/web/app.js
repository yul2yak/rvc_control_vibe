const canvas = document.getElementById("grid");
const ctx = canvas.getContext("2d");

const cellSize = 40;
const cols = Math.floor(canvas.width / cellSize);
const rows = Math.floor(canvas.height / cellSize);

const robot = { x: 2, y: Math.floor(rows / 2), dir: 0 };
const obstacles = new Set();
const dustCells = new Set();
let banner = null;

const STEP_DELAY_MS = 700;
const SCENARIO_PAUSE_MS = 1500;

function cellKey(x, y) {
    return `${x},${y}`;
}

function frontCell() {
    const offsets = [
        { x: 1, y: 0 },
        { x: 0, y: 1 },
        { x: -1, y: 0 },
        { x: 0, y: -1 },
    ];
    const o = offsets[robot.dir];
    return { x: robot.x + o.x, y: robot.y + o.y };
}

function leftCell() {
    const leftDir = (robot.dir + 3) % 4;
    const offsets = [
        { x: 1, y: 0 },
        { x: 0, y: 1 },
        { x: -1, y: 0 },
        { x: 0, y: -1 },
    ];
    const o = offsets[leftDir];
    return { x: robot.x + o.x, y: robot.y + o.y };
}

function rightCell() {
    const rightDir = (robot.dir + 1) % 4;
    const offsets = [
        { x: 1, y: 0 },
        { x: 0, y: 1 },
        { x: -1, y: 0 },
        { x: 0, y: -1 },
    ];
    const o = offsets[rightDir];
    return { x: robot.x + o.x, y: robot.y + o.y };
}

function isBlocked(x, y) {
    if (x < 0 || y < 0 || x >= cols || y >= rows) {
        return true;
    }
    return obstacles.has(cellKey(x, y));
}

function readSensorsFromGrid() {
    const f = frontCell();
    const l = leftCell();
    const r = rightCell();
    const dust = dustCells.has(cellKey(robot.x, robot.y));

    return {
        front_blocked: isBlocked(f.x, f.y),
        left_blocked: isBlocked(l.x, l.y),
        right_blocked: isBlocked(r.x, r.y),
        dust_detected: dust,
    };
}

function syncCheckboxesFromGrid() {
    const sensors = readSensorsFromGrid();
    document.getElementById("front").checked = sensors.front_blocked;
    document.getElementById("left").checked = sensors.left_blocked;
    document.getElementById("right").checked = sensors.right_blocked;
    document.getElementById("dust").checked = sensors.dust_detected;
}

function readSensorsFromForm() {
    return {
        front_blocked: document.getElementById("front").checked,
        left_blocked: document.getElementById("left").checked,
        right_blocked: document.getElementById("right").checked,
        dust_detected: document.getElementById("dust").checked,
    };
}

function updateStatus(state) {
    document.getElementById("motionState").textContent = state.motion_state;
    document.getElementById("cleaningMode").textContent = state.cleaning_mode;
    document.getElementById("lastMotion").textContent = state.last_motion;
    document.getElementById("lastClean").textContent = state.last_clean;
}

function applyCleaningEffect(cleanCommand) {
    if (cleanCommand === "SetBoost") {
        dustCells.delete(cellKey(robot.x, robot.y));
    }
}

function applyTickResult(state) {
    applyMotion(state.last_motion);
    applyCleaningEffect(state.last_clean);
    updateStatus(state);
    syncCheckboxesFromGrid();
    draw();
}

function applyMotion(command) {
    const offsets = [
        { x: 1, y: 0 },
        { x: 0, y: 1 },
        { x: -1, y: 0 },
        { x: 0, y: -1 },
    ];

    switch (command) {
        case "MoveForward": {
            const o = offsets[robot.dir];
            const nx = robot.x + o.x;
            const ny = robot.y + o.y;
            if (!isBlocked(nx, ny)) {
                robot.x = nx;
                robot.y = ny;
            }
            break;
        }
        case "TurnLeft":
            robot.dir = (robot.dir + 3) % 4;
            break;
        case "TurnRight":
            robot.dir = (robot.dir + 1) % 4;
            break;
        case "MoveBackward": {
            const o = offsets[robot.dir];
            const nx = robot.x - o.x;
            const ny = robot.y - o.y;
            if (!isBlocked(nx, ny)) {
                robot.x = nx;
                robot.y = ny;
            }
            break;
        }
        default:
            break;
    }
}

function draw() {
    ctx.clearRect(0, 0, canvas.width, canvas.height);

    for (let y = 0; y < rows; y++) {
        for (let x = 0; x < cols; x++) {
            ctx.strokeStyle = "#3a4250";
            ctx.strokeRect(x * cellSize, y * cellSize, cellSize, cellSize);

            const key = cellKey(x, y);
            if (dustCells.has(key)) {
                ctx.fillStyle = "#ffb74d33";
                ctx.fillRect(x * cellSize, y * cellSize, cellSize, cellSize);
            }
            if (obstacles.has(key)) {
                ctx.fillStyle = "#ef5350";
                ctx.fillRect(
                    x * cellSize + 4,
                    y * cellSize + 4,
                    cellSize - 8,
                    cellSize - 8
                );
            }
        }
    }

    const cx = robot.x * cellSize + cellSize / 2;
    const cy = robot.y * cellSize + cellSize / 2;
    ctx.fillStyle = "#66bb6a";
    ctx.beginPath();
    ctx.arc(cx, cy, cellSize * 0.35, 0, Math.PI * 2);
    ctx.fill();

    const nose = [
        { x: 1, y: 0 },
        { x: 0, y: 1 },
        { x: -1, y: 0 },
        { x: 0, y: -1 },
    ][robot.dir];
    ctx.strokeStyle = "#1b5e20";
    ctx.lineWidth = 3;
    ctx.beginPath();
    ctx.moveTo(cx, cy);
    ctx.lineTo(cx + nose.x * cellSize * 0.35, cy + nose.y * cellSize * 0.35);
    ctx.stroke();

    if (banner) {
        ctx.fillStyle = banner.bgColor;
        ctx.fillRect(0, canvas.height / 2 - 32, canvas.width, 64);
        ctx.fillStyle = "#ffffff";
        ctx.font = "bold 22px Segoe UI";
        ctx.textAlign = "center";
        ctx.fillText(banner.text, canvas.width / 2, canvas.height / 2 + 8);
        ctx.textAlign = "left";
    }
}

function setupWorld() {
    setupScenarioWorld("manual");
}

function setupBorders() {
    for (let x = 0; x < cols; x++) {
        obstacles.add(cellKey(x, 0));
        obstacles.add(cellKey(x, rows - 1));
    }
    for (let y = 0; y < rows; y++) {
        obstacles.add(cellKey(0, y));
        obstacles.add(cellKey(cols - 1, y));
    }
}

function setupScenarioWorld(scenarioId) {
    obstacles.clear();
    dustCells.clear();
    setupBorders();

    const row = Math.floor(rows / 2);

    if (scenarioId === "manual") {
        obstacles.add(cellKey(10, 6));
        obstacles.add(cellKey(10, 7));
        obstacles.add(cellKey(10, 8));
        dustCells.add(cellKey(5, 6));
        dustCells.add(cellKey(6, 6));
        return;
    }

    if (scenarioId === "forward_cleaning") {
        return;
    }

    if (scenarioId === "front_obstacle") {
        obstacles.add(cellKey(3, row));
        return;
    }

    if (scenarioId === "right_turn_obstacle") {
        obstacles.add(cellKey(3, row));
        obstacles.add(cellKey(2, row - 1));
        return;
    }

    if (scenarioId === "all_sides_blocked") {
        obstacles.add(cellKey(3, row));
        obstacles.add(cellKey(2, row - 1));
        obstacles.add(cellKey(2, row + 1));
        return;
    }

    if (scenarioId === "dust_boost") {
        dustCells.add(cellKey(2, row));
    }
}

function resetRobot() {
    robot.x = 2;
    robot.y = Math.floor(rows / 2);
    robot.dir = 0;
}

function showBanner(text, bgColor, durationMs) {
    return new Promise((resolve) => {
        banner = { text, bgColor };
        draw();
        setTimeout(() => {
            banner = null;
            draw();
            resolve();
        }, durationMs);
    });
}

function sleep(ms) {
    return new Promise((resolve) => setTimeout(resolve, ms));
}

async function api(path, body) {
    const res = await fetch(path, {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: body ? JSON.stringify(body) : "{}",
    });
    return res.json();
}

document.getElementById("btnStart").addEventListener("click", async () => {
    const state = await api("/api/start");
    updateStatus(state);
});

document.getElementById("btnReset").addEventListener("click", async () => {
    await api("/api/reset");
    resetRobot();
    setupWorld();
    updateStatus({
        motion_state: "Idle",
        cleaning_mode: "Off",
        last_motion: "None",
        last_clean: "None",
    });
    syncCheckboxesFromGrid();
    draw();
});

document.getElementById("btnTick").addEventListener("click", async () => {
    const sensors = readSensorsFromForm();
    const state = await api("/api/tick", sensors);
    applyTickResult(state);
});

canvas.addEventListener("click", (e) => {
    const rect = canvas.getBoundingClientRect();
    const x = Math.floor((e.clientX - rect.left) / cellSize);
    const y = Math.floor((e.clientY - rect.top) / cellSize);
    const key = cellKey(x, y);

    if (e.shiftKey) {
        if (dustCells.has(key)) {
            dustCells.delete(key);
        } else {
            dustCells.add(key);
        }
    } else {
        if (obstacles.has(key)) {
            obstacles.delete(key);
        } else {
            obstacles.add(key);
        }
    }

    syncCheckboxesFromGrid();
    draw();
});

setupWorld();
syncCheckboxesFromGrid();
draw();

function setMode(mode) {
    const isManual = mode === "manual";
    document.getElementById("manualPanel").classList.toggle("hidden", !isManual);
    document.getElementById("autoPanel").classList.toggle("hidden", isManual);
    document.getElementById("btnModeManual").className = isManual ? "active" : "inactive";
    document.getElementById("btnModeAuto").className = isManual ? "inactive" : "active";
}

function updateScenarioCard(scenarioId, scenarioResult) {
    const card = document.getElementById(`card-${scenarioId}`);
    if (!card) {
        return;
    }

    card.className = `scenario-card ${scenarioResult.passed ? "pass" : "fail"}`;
    const mark = scenarioResult.passed ? "✓" : "✗";
    let html = `<div class="scenario-title">${mark} ${scenarioResult.name}</div>`;

    if (!scenarioResult.passed) {
        html += `<div class="scenario-steps">${scenarioResult.message}</div>`;
    }

    for (const step of scenarioResult.steps) {
        const stepMark = step.passed ? "✓" : "✗";
        html += `<div class="scenario-steps">${stepMark} Step ${step.step}: `;
        html += `motion ${step.actual_motion} (기대 ${step.expected_motion}), `;
        html += `clean ${step.actual_clean} (기대 ${step.expected_clean})`;
        html += `</div>`;
    }

    card.innerHTML = html;
}

async function runScenariosVisual() {
    const btn = document.getElementById("btnRunScenarios");
    const container = document.getElementById("testResults");
    btn.disabled = true;

    try {
        const data = await fetch("/api/scenarios").then((res) => res.json());
        const summary = {
            total: data.scenarios.length,
            passed: 0,
            failed: 0,
            scenarios: [],
        };

        container.innerHTML = `<div class="test-summary">시나리오 실행 준비 중...</div>`;
        for (const scenario of data.scenarios) {
            container.innerHTML += `<div id="card-${scenario.id}" class="scenario-card running">`
                + `<div class="scenario-title">▶ ${scenario.name} 대기</div></div>`;
        }

        for (const scenario of data.scenarios) {
            const card = document.getElementById(`card-${scenario.id}`);
            if (card) {
                card.className = "scenario-card running";
                card.innerHTML = `<div class="scenario-title">▶ ${scenario.name} 실행 중...</div>`;
            }

            setupScenarioWorld(scenario.id);
            resetRobot();
            draw();

            await api("/api/reset");
            await api("/api/start", { turn_strategy: scenario.turn_strategy });

            const stepResults = [];
            let passed = true;

            for (let i = 0; i < scenario.steps.length; i++) {
                const step = scenario.steps[i];
                await sleep(STEP_DELAY_MS);

                const tickState = await api("/api/tick", {
                    ...step.sensors,
                    timer_advance_ms: step.timer_advance_ms || 0,
                });

                const stepPassed = tickState.last_motion === step.expected_motion
                    && tickState.last_clean === step.expected_clean;

                if (!stepPassed) {
                    passed = false;
                }

                applyTickResult(tickState);

                stepResults.push({
                    step: i + 1,
                    expected_motion: step.expected_motion,
                    actual_motion: tickState.last_motion,
                    expected_clean: step.expected_clean,
                    actual_clean: tickState.last_clean,
                    passed: stepPassed,
                });
            }

            const scenarioResult = {
                id: scenario.id,
                name: scenario.name,
                passed,
                message: passed ? "OK" : "검증 실패",
                steps: stepResults,
            };

            if (passed) {
                summary.passed += 1;
            } else {
                summary.failed += 1;
            }
            summary.scenarios.push(scenarioResult);

            updateScenarioCard(scenario.id, scenarioResult);
            await showBanner(
                passed ? `✓ ${scenario.name} 성공` : `✗ ${scenario.name} 실패`,
                passed ? "#2e7d3288" : "#c6282888",
                1200
            );
            await sleep(SCENARIO_PAUSE_MS);
        }

        const summaryEl = container.querySelector(".test-summary");
        if (summaryEl) {
            summaryEl.className = `test-summary ${summary.failed === 0 ? "all-pass" : "has-fail"}`;
            summaryEl.textContent = `${summary.passed} / ${summary.total} 통과`;
        }
    } catch (err) {
        container.innerHTML = `<div class="test-summary has-fail">오류: ${err.message}</div>`;
    } finally {
        btn.disabled = false;
    }
}

document.getElementById("btnModeManual").addEventListener("click", () => setMode("manual"));
document.getElementById("btnModeAuto").addEventListener("click", () => setMode("auto"));

document.getElementById("btnRunScenarios").addEventListener("click", () => {
    runScenariosVisual();
});
