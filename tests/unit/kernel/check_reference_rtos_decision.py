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
FREERTOS_BACKEND = (
    ROOT / "components" / "kernel" / "osal" / "backend" / "freertos" / "xy_os_freertos.c"
)
FREERTOS_KERNEL = ROOT / "third_party" / "freertos" / "FreeRTOS"
FREERTOS_CONFIG = ROOT / "third_party" / "freertos" / "FreeRTOSConfig.h"


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

    for token in (
        "FreeRTOS",
        "REFERENCE_SELECTED",
        "integration-pending",
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
    require("FreeRTOS reference selected" in evidence,
            "evidence matrix must preserve the selected-but-not-integrated boundary", errors)
    require('set(OSAL_BACKEND "baremetal"' in osal_cmake,
            "default OSAL backend must remain baremetal until integration is proven", errors)
    require(FREERTOS_BACKEND.is_file(), "FreeRTOS OSAL adapter is missing", errors)
    require((FREERTOS_KERNEL / "include" / "FreeRTOS.h").is_file(),
            "vendored FreeRTOS kernel headers are missing", errors)
    require(not FREERTOS_CONFIG.exists(),
            "guard assumptions changed: a root FreeRTOSConfig.h now exists; perform integration review",
            errors)

    if errors:
        print("reference_rtos_decision failed:")
        for error in errors:
            print(f"- {error}")
        return 1

    print("reference_rtos_decision_ok reference=FreeRTOS integration=pending hardware=pending")
    return 0


if __name__ == "__main__":
    sys.exit(main())
