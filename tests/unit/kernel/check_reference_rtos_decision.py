#!/usr/bin/env python3
"""Guard the Sprint 5 reference RTOS decision and evidence boundary."""

import hashlib
import json
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
OSAL_README = ROOT / "components" / "kernel" / "osal" / "README.md"
OSAL_QUICK_START = ROOT / "components" / "kernel" / "osal" / "QUICK_START.md"
OSAL_IMPLEMENTATION_STATUS = (
    ROOT / "components" / "kernel" / "osal" / "IMPLEMENTATION_STATUS.md"
)
COMPONENTS_README = ROOT / "components" / "README.md"
DEVELOPMENT_PRIORITY = ROOT / "components" / "DEVELOPMENT_PRIORITY.md"
COMPONENT_INDEX = ROOT / "docs" / "components" / "index.md"
OSAL_INTRODUCTION = ROOT / "docs" / "components" / "osal" / "introduction.md"
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
RESOURCE_EVIDENCE = (
    ROOT
    / "docs"
    / "validation"
    / "evidence"
    / "pandora-stm32l475"
    / "2026-09-04"
)
RESOURCE_LOG = RESOURCE_EVIDENCE / "uart-wchlink-osal-resource-e6cd0906.txt"
RESOURCE_METADATA = RESOURCE_EVIDENCE / "uart-wchlink-osal-resource-e6cd0906.json"
RESOURCE_COMMIT = "e6cd0906f0937c36d566eb88439b510b545d8250"
RESOURCE_LOG_SHA256 = "3808c1623b409665ac6d6c89171e4294c66a7c2b52cbb8ac80ec95d66b327c37"


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
    osal_readme = OSAL_README.read_text(encoding="utf-8")
    osal_quick_start = OSAL_QUICK_START.read_text(encoding="utf-8")
    osal_implementation_status = OSAL_IMPLEMENTATION_STATUS.read_text(encoding="utf-8")
    components_readme = COMPONENTS_README.read_text(encoding="utf-8")
    development_priority = DEVELOPMENT_PRIORITY.read_text(encoding="utf-8")
    component_index = COMPONENT_INDEX.read_text(encoding="utf-8")
    osal_introduction = OSAL_INTRODUCTION.read_text(encoding="utf-8")
    root_kconfig = ROOT_KCONFIG.read_text(encoding="utf-8")
    root_cmake = ROOT_CMAKE.read_text(encoding="utf-8")
    third_party_cmake = THIRD_PARTY_CMAKE.read_text(encoding="utf-8")

    for token in (
        "FreeRTOS",
        "BOARD_RUNTIME_PARTIAL",
        "root-selected-pandora-task-sync-isr-smoke-verified",
        "pandora-thread-sync-systick-isr-b1",
        "pandora-resource-lifecycle-b1",
        "不构成 blocking timeout、长稳压力、性能、STM32U5",
        "46499b33e332f2b4111e1c0149366b5c86064909",
        "4ebf46dacd8c906455fb541ca70c25b692cc51d8",
        "48ca0509640fd9f4fbb745957ec588e25131e160",
        "33c3a665032f62efde0e410cf21a4fd74d04975d",
        "8443f907a8ec912317a774f2129d03b7746ac7b0",
        "9c2d4d4e811a8b835f6432290bd12b6fd5000dcd",
        RESOURCE_COMMIT,
        RESOURCE_LOG_SHA256,
        "6bde99f52beda6b5b30b3bd7bc655dc8eda116662eae0a96502a36b06264d627",
        "FreeRTOSConfig.h",
        "Cortex-M33",
        "third_party/freertos/FreeRTOS",
    ):
        require(token in decision, f"decision must preserve token: {token}", errors)

    require(RESOURCE_LOG.is_file(), "Pandora resource/lifecycle UART log is missing", errors)
    require(RESOURCE_METADATA.is_file(), "Pandora resource/lifecycle metadata is missing", errors)
    if RESOURCE_LOG.is_file() and RESOURCE_METADATA.is_file():
        payload = RESOURCE_LOG.read_bytes()
        metadata = json.loads(RESOURCE_METADATA.read_text(encoding="utf-8"))
        log_lines = payload.decode("utf-8").splitlines()
        require(hashlib.sha256(payload).hexdigest() == RESOURCE_LOG_SHA256,
                "Pandora resource/lifecycle UART SHA-256 mismatch", errors)
        require(len(payload) == 2651,
                "Pandora resource/lifecycle UART byte count must remain 2651", errors)
        expected_identity = (
            "FIRMWARE_COMMIT " + RESOURCE_COMMIT
        )
        for marker in (
            "PANDORA STM32L475VE XINYI OSAL FREERTOS READY",
            expected_identity,
            "OSAL_RESOURCE_EXHAUSTED",
            "OSAL_RESOURCE_RECOVERED",
            "OSAL_LIFECYCLE_REINIT",
        ):
            require(log_lines.count(marker) == 1,
                    f"Pandora resource/lifecycle UART must preserve one {marker}", errors)
        resource_positions = [
            next((index for index, line in enumerate(log_lines) if line == marker), -1)
            for marker in (
                "OSAL_RESOURCE_EXHAUSTED",
                "OSAL_RESOURCE_RECOVERED",
                "OSAL_LIFECYCLE_REINIT",
            )
        ]
        require(-1 not in resource_positions and resource_positions == sorted(resource_positions),
                "Pandora resource/lifecycle UART markers must remain ordered", errors)
        for marker in (
            "OSAL_RESOURCE_ERROR",
            "OSAL_ISR_TIMEOUT",
            "OSAL_SEM_TIMEOUT",
            "OSAL_QUEUE_MISMATCH",
            "OSAL_EVENT_MISMATCH",
            "OSAL_MUTEX_TIMEOUT",
            "OSAL_MUTEX_MISMATCH",
        ):
            require(marker not in log_lines,
                    f"Pandora resource/lifecycle UART must not contain {marker}", errors)
        require(metadata.get("status") == "B1_RESOURCE_LIFECYCLE_REVIEWED",
                "Pandora resource/lifecycle metadata must remain reviewed B1", errors)
        require(metadata.get("firmware_commit") == RESOURCE_COMMIT,
                "Pandora resource/lifecycle metadata must bind the flashed commit", errors)
        require(metadata.get("capture_sha256") == RESOURCE_LOG_SHA256,
                "Pandora resource/lifecycle metadata must bind the UART log", errors)
        require(metadata.get("programmed_bytes") == 16080,
                "Pandora resource/lifecycle metadata must preserve programmed size", errors)
        observations = metadata.get("observations", {})
        for marker in (
            "OSAL_RESOURCE_EXHAUSTED",
            "OSAL_RESOURCE_RECOVERED",
            "OSAL_LIFECYCLE_REINIT",
        ):
            require(observations.get(marker) == 1,
                    f"Pandora resource/lifecycle metadata must preserve one {marker}", errors)
        require(observations.get("OSAL_RESOURCE_ERROR") == 0,
                "Pandora resource/lifecycle metadata must preserve zero resource errors", errors)
        require(observations.get("required_resource_marker_order") is True,
                "Pandora resource/lifecycle metadata must preserve ordered markers", errors)

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
    for document_name, document in (
        ("OSAL README", osal_readme),
        ("OSAL quick start", osal_quick_start),
        ("OSAL implementation status", osal_implementation_status),
    ):
        require("runtime-pending" in document,
                f"{document_name} must preserve the RTOS runtime-pending boundary", errors)
    for forbidden in (
        "Same code runs on bare-metal, FreeRTOS, or RT-Thread. Just switch the backend .c file",
        "✅ Complete implementation",
        "No application code changes required",
    ):
        require(forbidden not in osal_quick_start + osal_implementation_status,
                f"OSAL docs must not preserve unverified portability/completion claim: {forbidden}",
                errors)
    for document_name, document in (
        ("components README", components_readme),
        ("development priority", development_priority),
        ("component index", component_index),
        ("OSAL introduction", osal_introduction),
    ):
        require("runtime-pending" in document,
                f"{document_name} must preserve the RTOS runtime-pending boundary", errors)
    for forbidden in (
        "✅ 支持主流 RTOS",
        "| `kernel/` | OS 抽象层 (FreeRTOS/RT-Thread/RTX) | ✅ |",
        "| **OSAL** | ✅ | ✅ | ✅ | ✅ | 17 | 🟢 完善 |",
        "**状态**: ✅ 完善",
        "✅ **多 RTOS 支持**",
        "| FreeRTOS | MIT | ✅ 完善 |",
        "| RT-Thread | Apache-2.0 | ✅ 完善 |",
        "| CMSIS-RTX | Apache-2.0 | ✅ 完善 |",
    ):
        require(forbidden not in components_readme + development_priority + component_index + osal_introduction,
                f"public component docs must not preserve unverified RTOS claim: {forbidden}",
                errors)
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

    print("reference_rtos_decision_ok reference=FreeRTOS pandora_runtime=task-sync-isr-resource-lifecycle-b1 remaining=timeout-long-stress")
    return 0


if __name__ == "__main__":
    sys.exit(main())
