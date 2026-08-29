#!/usr/bin/env python3
"""Guard the Sprint 5 reference RTOS decision and evidence boundary."""

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[3]
DECISION = ROOT / "docs" / "validation" / "reference-rtos-decision.md"
TRACKER = ROOT / "docs" / "plans" / "SPRINT_TRACKER.md"
AUDIT_PLAN = ROOT / "docs" / "plans" / "2026-08-17-component-audit-sprint-plan.md"
EVIDENCE = ROOT / "docs" / "validation" / "component-evidence-matrix.md"
OSAL_CMAKE = ROOT / "components" / "kernel" / "osal" / "CMakeLists.txt"
BACKEND_COMPARISON = ROOT / "components" / "kernel" / "osal" / "BACKEND_COMPARISON.md"
FREERTOS_README = ROOT / "components" / "kernel" / "osal" / "freertos" / "README.md"
KERNEL_README = ROOT / "components" / "kernel" / "README.md"
ROOT_KCONFIG = ROOT / "Kconfig"
ROOT_CMAKE = ROOT / "CMakeLists.txt"
THIRD_PARTY_CMAKE = ROOT / "third_party" / "CMakeLists.txt"
FREERTOS_BACKEND = (
    ROOT / "components" / "kernel" / "osal" / "backend" / "freertos" / "xy_os_freertos.c"
)
FREERTOS_KERNEL = ROOT / "third_party" / "freertos" / "FreeRTOS"
FREERTOS_CONFIG = (
    ROOT
    / "components"
    / "kernel"
    / "osal"
    / "config"
    / "freertos"
    / "FreeRTOSConfig.h"
)
FREERTOS_COMPILE_PROBE = (
    ROOT / "components" / "kernel" / "osal" / "scripts" / "compile_freertos_stm32u5.py"
)


def require(condition: bool, message: str, errors: list[str]) -> None:
    if not condition:
        errors.append(message)


def main() -> int:
    errors: list[str] = []
    require(DECISION.is_file(), "reference RTOS decision record is missing", errors)
    if errors:
        print("reference_rtos_decision failed:")
        for error in errors:
            print(f"- {error}")
        return 1

    decision = DECISION.read_text(encoding="utf-8")
    tracker = TRACKER.read_text(encoding="utf-8")
    audit_plan = AUDIT_PLAN.read_text(encoding="utf-8")
    evidence = EVIDENCE.read_text(encoding="utf-8")
    osal_cmake = OSAL_CMAKE.read_text(encoding="utf-8")
    backend_comparison = BACKEND_COMPARISON.read_text(encoding="utf-8")
    freertos_readme = FREERTOS_README.read_text(encoding="utf-8")
    kernel_readme = KERNEL_README.read_text(encoding="utf-8")
    root_kconfig = ROOT_KCONFIG.read_text(encoding="utf-8")
    root_cmake = ROOT_CMAKE.read_text(encoding="utf-8")
    third_party_cmake = THIRD_PARTY_CMAKE.read_text(encoding="utf-8")

    for token in (
        "FreeRTOS",
        "REFERENCE_SELECTED",
        "root-selected-compile-guarded-runtime-pending",
        "hardware-pending",
        "不构成 RTOS runtime、ISR、并发或实板证据",
        "FreeRTOSConfig.h",
        "Cortex-M33",
        "third_party/freertos/FreeRTOS",
    ):
        require(token in decision, f"decision must preserve token: {token}", errors)

    require("RT-Thread" in decision and "not selected" in decision,
            "decision must record why RT-Thread is not the reference backend", errors)
    require("| S5-01 |" in tracker and "REFERENCE_SELECTED" in tracker,
            "Sprint tracker must record the bounded S5-01 reference decision", errors)
    require("**选择**：FreeRTOS" in audit_plan,
            "audit plan must resolve the reference RTOS choice to FreeRTOS", errors)
    require("[FreeRTOS reference](reference-rtos-decision.md)" in evidence,
            "evidence matrix must preserve the selected compile-only reference boundary", errors)
    require('set(OSAL_BACKEND "baremetal"' in osal_cmake,
            "default OSAL backend must remain baremetal until integration is proven", errors)
    for token in ("config KERNEL_OSAL", "config OSAL_BACKEND_FREERTOS"):
        require(token in root_kconfig,
                f"root Kconfig must expose the reference backend token: {token}", errors)
    require("CONFIG_OSAL_BACKEND_FREERTOS" in root_cmake,
            "root CMake must map the generated FreeRTOS selection", errors)
    require('set(RTOS_BACKEND "freertos"' in root_cmake,
            "root CMake must select the matching third-party kernel", errors)
    require("freertos/FreeRTOS" in third_party_cmake and "${FREERTOS_DIR}/tasks.c" in third_party_cmake,
            "third-party CMake must use the pinned FreeRTOS kernel layout", errors)
    require("ARM_CM33_NTZ" in third_party_cmake,
            "third-party CMake must use the guarded Cortex-M33 port", errors)
    for forbidden in (
        "0.3-0.5",
        "50-80",
        "~6 KB",
        "~1-2 µs",
        "same application-level API",
        "migration seamless",
    ):
        require(forbidden not in backend_comparison,
                f"backend comparison must not preserve unverified claim: {forbidden}", errors)
    for token in (
        "source/compile inventory only",
        "no project-owned runtime benchmark evidence",
        "runtime-pending",
    ):
        require(token in backend_comparison,
                f"backend comparison must preserve evidence boundary: {token}", errors)
    for forbidden in ("Complete FreeRTOS implementation", "full multitasking support"):
        require(forbidden not in freertos_readme,
                f"FreeRTOS README must not preserve unverified claim: {forbidden}", errors)
    for token in ("compile-guarded-runtime-pending", "does not claim runtime"):
        require(token in freertos_readme,
                f"FreeRTOS README must preserve evidence boundary: {token}", errors)
    require("source/static-library gate" in kernel_readme and "runtime/ISR/并发/实板 pending" in kernel_readme,
            "kernel README must record the bounded FreeRTOS compile evidence", errors)
    require(FREERTOS_BACKEND.is_file(), "FreeRTOS OSAL adapter is missing", errors)
    require((FREERTOS_KERNEL / "include" / "FreeRTOS.h").is_file(),
            "vendored FreeRTOS kernel headers are missing", errors)
    require(FREERTOS_CONFIG.is_file(),
            "XinYi-owned STM32U5 FreeRTOSConfig.h is missing", errors)
    require(FREERTOS_COMPILE_PROBE.is_file(),
            "STM32U5 FreeRTOS adapter/kernel compile probe is missing", errors)
    if FREERTOS_CONFIG.is_file():
        config = FREERTOS_CONFIG.read_text(encoding="utf-8")
        for token in (
            "configCPU_CLOCK_HZ",
            "configTICK_RATE_HZ",
            "configMAX_PRIORITIES",
            "configCHECK_FOR_STACK_OVERFLOW",
            "configASSERT",
        ):
            require(token in config, f"FreeRTOS config must preserve token: {token}", errors)

    if errors:
        print("reference_rtos_decision failed:")
        for error in errors:
            print(f"- {error}")
        return 1

    print("reference_rtos_decision_ok reference=FreeRTOS integration=compile-guarded runtime=pending hardware=pending")
    return 0


if __name__ == "__main__":
    sys.exit(main())
