#!/usr/bin/env python3
"""Summarize Cobertura XML coverage for unit vs simulator runs."""

from __future__ import annotations

import sys
import xml.etree.ElementTree as ET
from pathlib import Path


def normalize_filename(filename: str) -> str:
    normalized = filename.replace("\\", "/")
    for marker in ("src/", "simulator/server/", "tests/"):
        idx = normalized.find(marker)
        if idx != -1:
            return normalized[idx:]
    return normalized


def parse_cobertura(path: Path) -> dict[str, dict[str, float]]:
    tree = ET.parse(path)
    root = tree.getroot()

    by_file: dict[str, dict[str, float]] = {}
    for package in root.findall("packages/package"):
        for clazz in package.findall("classes/class"):
            filename = clazz.get("filename", "").replace("\\", "/")
            if not filename:
                continue

            lines = clazz.findall("lines/line")
            if not lines:
                continue

            hit = sum(1 for line in lines if int(line.get("hits", "0")) > 0)
            total = len(lines)
            rate = (hit / total * 100.0) if total else 0.0

            key = normalize_filename(filename)
            if key not in by_file:
                by_file[key] = {"hit": 0.0, "total": 0.0}
            by_file[key]["hit"] += hit
            by_file[key]["total"] += total

    for stats in by_file.values():
        stats["rate"] = (stats["hit"] / stats["total"] * 100.0) if stats["total"] else 0.0

    return by_file


def aggregate(files: dict[str, dict[str, float]], prefix: str) -> dict[str, float]:
    hit = 0.0
    total = 0.0
    for name, stats in files.items():
        if name.startswith(prefix):
            hit += stats["hit"]
            total += stats["total"]
    return {
        "hit": hit,
        "total": total,
        "rate": (hit / total * 100.0) if total else 0.0,
    }


def overall(files: dict[str, dict[str, float]]) -> dict[str, float]:
    hit = sum(v["hit"] for v in files.values())
    total = sum(v["total"] for v in files.values())
    return {
        "hit": hit,
        "total": total,
        "rate": (hit / total * 100.0) if total else 0.0,
    }


def format_pct(rate: float) -> str:
    return f"{rate:.1f}%"


def write_summary(unit_path: Path, sim_path: Path, out_path: Path) -> None:
    unit = parse_cobertura(unit_path)
    sim = parse_cobertura(sim_path)

    unit_all = overall(unit)
    sim_all = overall(sim)

    sections = [
        ("domain", "src/domain/"),
        ("application", "src/application/"),
        ("adapters", "src/adapters/"),
        ("simulator", "simulator/server/"),
    ]

    lines = [
        "# 테스트 커버리지 요약",
        "",
        "도구: [OpenCppCoverage](https://github.com/OpenCppCoverage/OpenCppCoverage) (라인 커버리지)",
        "",
        "## 전체",
        "",
        "| 구분 | 커버 라인 | 전체 라인 | 커버리지 |",
        "|------|-----------|-----------|----------|",
        f"| Unit 테스트 (`rvc_tests`) | {int(unit_all['hit'])} | {int(unit_all['total'])} | {format_pct(unit_all['rate'])} |",
        f"| Simulator 자동 테스트 (`rvc_scenario_tests`) | {int(sim_all['hit'])} | {int(sim_all['total'])} | {format_pct(sim_all['rate'])} |",
        "",
        "## 영역별 (라인 커버리지)",
        "",
        "| 영역 | Unit | Simulator |",
        "|------|------|-----------|",
    ]

    for label, prefix in sections:
        u = aggregate(unit, prefix)
        s = aggregate(sim, prefix)
        lines.append(
            f"| {label} | {format_pct(u['rate'])} ({int(u['hit'])}/{int(u['total'])}) "
            f"| {format_pct(s['rate'])} ({int(s['hit'])}/{int(s['total'])}) |"
        )

    lines.extend(
        [
            "",
            "## 파일별",
            "",
            "### Unit 테스트",
            "",
            "| 파일 | 커버리지 |",
            "|------|----------|",
        ]
    )

    for name in sorted(unit):
        if name.startswith("src/") or name.startswith("simulator/"):
            lines.append(f"| `{name}` | {format_pct(unit[name]['rate'])} |")

    lines.extend(
        [
            "",
            "### Simulator 자동 테스트",
            "",
            "| 파일 | 커버리지 |",
            "|------|----------|",
        ]
    )

    for name in sorted(sim):
        if name.startswith("src/") or name.startswith("simulator/"):
            lines.append(f"| `{name}` | {format_pct(sim[name]['rate'])} |")

    lines.extend(
        [
            "",
            "## 재현 방법",
            "",
            "```powershell",
            "cmake --build build --config Debug --target rvc_tests rvc_scenario_tests",
            "powershell -File scripts/coverage.ps1",
            "```",
            "",
        ]
    )

    out_path.write_text("\n".join(lines), encoding="utf-8")


def main() -> int:
    if len(sys.argv) != 4:
        print("Usage: summarize_coverage.py <unit.xml> <simulator.xml> <summary.md>")
        return 1

    write_summary(Path(sys.argv[1]), Path(sys.argv[2]), Path(sys.argv[3]))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
