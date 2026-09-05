#!/usr/bin/env python3
"""Guard public HAL/FOTA status wording against the evidence matrix."""

from __future__ import annotations

from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
INDEX = ROOT / "docs" / "components" / "index.md"
COMPONENTS_README = ROOT / "components" / "README.md"
EVIDENCE = ROOT / "docs" / "validation" / "component-evidence-matrix.md"
HAL_MATRIX = ROOT / "docs" / "validation" / "hal-platform-evidence-matrix.md"
ARCHITECTURE_ANALYSIS = ROOT / "components" / "ARCHITECTURE_ANALYSIS.md"
REFACTORING_STATUS = ROOT / "components" / "REFACTORING_COMPLETED.md"
REFACTORING_PLAN = ROOT / "components" / "ARCHITECTURE_REFACTORING_PLAN.md"
COMPONENT_GAP_ANALYSIS = ROOT / "components" / "COMPONENT_GAP_ANALYSIS.md"
BUILD_SYSTEM_ANALYSIS = ROOT / "docs" / "build_system_analysis.md"


def require(condition: bool, message: str, errors: list[str]) -> None:
    if not condition:
        errors.append(message)


def validate() -> list[str]:
    errors: list[str] = []
    index = INDEX.read_text(encoding="utf-8")
    components_readme = COMPONENTS_README.read_text(encoding="utf-8")
    evidence = EVIDENCE.read_text(encoding="utf-8")
    hal_matrix = HAL_MATRIX.read_text(encoding="utf-8")
    architecture_analysis = ARCHITECTURE_ANALYSIS.read_text(encoding="utf-8")
    refactoring_status = REFACTORING_STATUS.read_text(encoding="utf-8")
    refactoring_plan = REFACTORING_PLAN.read_text(encoding="utf-8")
    component_gap_analysis = COMPONENT_GAP_ANALYSIS.read_text(encoding="utf-8")
    build_system_analysis = BUILD_SYSTEM_ANALYSIS.read_text(encoding="utf-8")

    for token in (
        "HAL / Storage | H1（PC；canonical W25Q128 command/error/page-boundary contract）",
        "FOTA | H1（状态机 + signature-provider/boot-handoff/delta/boot-confirm fail-closed",
    ):
        require(token in evidence, f"canonical evidence matrix is missing token: {token}", errors)
    require("BOARD_PENDING" in hal_matrix, "HAL matrix must retain BOARD_PENDING", errors)

    for forbidden in (
        "| **HAL** | ✅ | ✅ | ✅ | ✅ | 11 | 🟢 完善 |",
        "| **FOTA** | ✅ | ✅ | ✅ | ✅ | 🟢 主线可用",
        "- STM32U5 (完整实现)",
        "- ✅ HAL 统一工程 100% 完成",
        "🟢 完善",
        "🟢 tail host coverage 已收口",
        "### 🟡 主线可用",
        "总计：234 个测试用例",
        "Host-guarded / 分层证据 | 13 | 81%",
        "硬件、安全或人工证据待补 | 3 | 19%",
    ):
        require(forbidden not in index, f"public component index overclaims evidence: {forbidden}", errors)

    for required in (
        "HAL：PC Host contract、部分 QEMU；STM32U5/WCH/HC32 实板证据 pending",
        "FOTA：Host fail-closed contract；board Flash、bootloader、secure provider 与实板 pending",
        "STM32U5（source/compile 前置；Board pending）",
        "Host/PC/QEMU/compile-only 不构成实板、安全或 production-ready 证据",
        "测试数量以 canonical CTest 实际发现结果为准",
        "公开入口分类不是产品完成度或 maturity 百分比",
    ):
        require(required in index, f"public component index is missing evidence boundary: {required}", errors)

    for stale_claim in (
        "| SHT40 | 温湿度 | I2C | ✅ |",
        "| DHT11 | 温湿度 | GPIO | ✅ |",
        "| SSD1306 | OLED | I2C | ✅ |",
        "| BQ25620 | 充电器 | 三段式充电 | ✅ |",
        "| `sensor/` | 传感器抽象层 | ✅ |",
        "| `fota/` | 固件升级 | ✅ |",
        "| `crypto/` | 加密库 | ✅ |",
        "| `pm/` | 电源管理 | ✅ |",
        "add_subdirectory(components/drivers/sensor)",
    ):
        require(stale_claim not in components_readme,
                f"components README retains unqualified status/build guidance: {stale_claim}", errors)
    for required in (
        "仅表示源码/目录存在，不代表进入 root product target",
        "Device-model canonical owners",
        "Host contract；实板 pending",
        "以 root Kconfig/CMake 与组件证据台账为准",
    ):
        require(required in components_readme,
                f"components README is missing source/build/evidence boundary: {required}", errors)

    for path, text in (
        (ARCHITECTURE_ANALYSIS, architecture_analysis),
        (REFACTORING_STATUS, refactoring_status),
    ):
        require("drivers/power/charger/           # 驱动层（已迁移）" not in text,
                f"{path.name} claims the nonexistent charger migration is complete", errors)
        require("`driver/charger/` | `drivers/power/charger/` | 充电器驱动" not in text,
                f"{path.name} lists the nonexistent charger owner as migrated", errors)
        require("components/charger/src/xy_bq25620.c" in text,
                f"{path.name} must identify the canonical BQ25620 owner", errors)
        require("components/drivers/power/charger/` 当前不存在" in text,
                f"{path.name} must retain the nonexistent-target boundary", errors)

    for stale_plan_token in (
        "方案 A: 完全重构（推荐）",
        "mv components/driver/charger/* components/drivers/power/charger/",
        "mkdir -p components/drivers/power/{charger,fuel_gauge}",
    ):
        require(stale_plan_token not in refactoring_plan,
                f"{REFACTORING_PLAN.name} retains an executable stale power migration: "
                f"{stale_plan_token}", errors)
    for required in (
        "未执行历史提案",
        "components/charger/src/xy_bq25620.c",
        "Fuel Gauge 保持 standalone",
        "不得执行本文旧命令",
    ):
        require(required in refactoring_plan,
                f"{REFACTORING_PLAN.name} is missing the canonical ownership boundary: {required}",
                errors)

    for stale_gap_claim in (
        "| **基础** | clib | 100% |",
        "| **硬件** | hal, device | 100% |",
        "| **存储** | dm, fota | 100% |",
        "| **显示** | gui, drivers/display | 100% |",
        "当前完成度：96% (现有组件)",
        "基础组件完整",
        "驱动组件完善",
        "现有 96% 完成",
    ):
        require(stale_gap_claim not in component_gap_analysis,
                f"{COMPONENT_GAP_ANALYSIS.name} retains unqualified maturity claim: "
                f"{stale_gap_claim}", errors)
    for required in (
        "历史差距清单",
        "不作为当前组件成熟度、产品优先级或支持状态的事实源",
        "docs/validation/component-evidence-matrix.md",
        "Host/PC/QEMU/compile-only 不构成实板、安全、性能或 production-ready 证据",
    ):
        require(required in component_gap_analysis,
                f"{COMPONENT_GAP_ANALYSIS.name} is missing its historical/evidence boundary: "
                f"{required}", errors)

    for stale_build_claim in (
        "✅ 完善",
        "所有 CMakeLists.txt 遵循相同结构",
        "所有主要组件都有构建配置",
        "总体评分**: 8.5/10",
        "make test-all",
        "-DXY_COMPONENT_FEATURE_A=ON",
    ):
        require(stale_build_claim not in build_system_analysis,
                f"{BUILD_SYSTEM_ANALYSIS.name} retains stale build claim/command: "
                f"{stale_build_claim}", errors)
    for required in (
        "历史构建系统分析",
        "不作为当前构建命令、组件选择或成熟度的事实源",
        "root Makefile、CMakeLists.txt 与 Kconfig",
        "docs/validation/kconfig-cmake-configuration-matrix.md",
        "Host/PC/compile-only 不构成实板、安全或 production-ready 证据",
    ):
        require(required in build_system_analysis,
                f"{BUILD_SYSTEM_ANALYSIS.name} is missing its historical/build-evidence boundary: "
                f"{required}", errors)

    return errors


def main() -> int:
    errors = validate()
    if errors:
        print("public_component_evidence failed:")
        for error in errors:
            print(f"- {error}")
        return 1
    print("public_component_evidence_ok hal=host-qemu-partial fota=host-fail-closed board=pending")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
