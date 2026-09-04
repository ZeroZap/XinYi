#!/usr/bin/env python3
"""Validate a bounded Pandora OSAL/FreeRTOS UART stress capture."""

import argparse
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import re
import sys

BANNER = "PANDORA STM32L475VE XINYI OSAL FREERTOS READY"
IDENTITY_PREFIX = "FIRMWARE_COMMIT "
PIPELINE = (
    "OSAL_MUTEX_FAST",
    "OSAL_TASK_FAST",
    "OSAL_QUEUE_SEND",
    "OSAL_EVENT_SET",
    "OSAL_IPC_SEND",
    "OSAL_PM_TICK",
    "OSAL_SEM_TAKE",
    "OSAL_EVENT_WAIT",
    "OSAL_QUEUE_RECV",
    "OSAL_DEVICE_LOOKUP",
    "[I] OSAL_TRACE_DELIVER",
    "OSAL_IPC_DELIVER",
    "OSAL_MUTEX_SLOW",
    "OSAL_TASK_SLOW",
)
REQUIRED_ONESHOT = (
    "OSAL_RESOURCE_EXHAUSTED",
    "OSAL_BLOCKING_TIMEOUT_OK",
    "OSAL_RESOURCE_RECOVERED",
    "OSAL_LIFECYCLE_REINIT",
)
TIM6_RECOVERY = (
    "OSAL_TIM6_IRQ_TIMEOUT_EXPECTED",
    "OSAL_TIM6_IRQ_RECOVERED",
)
IPC_RECOVERY = (
    "OSAL_IPC_SATURATED",
    "OSAL_IPC_RECOVERED",
)
MULTI_PRODUCER_COMPLETION = "OSAL_MULTI_PRODUCER_OK"
MULTI_CONSUMER_DISTRIBUTION = "OSAL_MULTI_CONSUMER_DISTRIBUTED"
MULTI_CONSUMER_TAKES = (
    "OSAL_MULTI_CONSUMER_0_TAKE",
    "OSAL_MULTI_CONSUMER_1_TAKE",
)
EXPECTED_MULTI_MESSAGES = 16
ERROR_MARKERS = (
    "OSAL_BLOCKING_TIMEOUT_ERROR",
    "OSAL_EVENT_MISMATCH",
    "OSAL_IPC_SATURATION_ERROR",
    "OSAL_ISR_TIMEOUT",
    "OSAL_TIM6_IRQ_RECOVERY_ERROR",
    "OSAL_TIM6_IRQ_TIMEOUT",
    "OSAL_MUTEX_MISMATCH",
    "OSAL_MUTEX_TIMEOUT",
    "OSAL_MULTI_CONSUMER_ERROR",
    "OSAL_MULTI_PRODUCER_ERROR",
    "OSAL_QUEUE_MISMATCH",
    "OSAL_RESOURCE_ERROR",
    "OSAL_SEM_TIMEOUT",
)


def validate_commit(revision: str) -> str:
    if re.fullmatch(r"[0-9a-f]{40}", revision) is None:
        raise ValueError("firmware commit must be an exact 40-character lowercase Git SHA")
    return revision


def count_ordered_cycles(lines: list[str]) -> int:
    cycles = 0
    state = 0
    for line in lines:
        if line == PIPELINE[state]:
            state += 1
            if state == len(PIPELINE):
                cycles += 1
                state = 0
        elif line == PIPELINE[0]:
            state = 1
    return cycles


def analyze_capture(
    payload: bytes,
    firmware_commit: str,
    duration_seconds: int,
    min_pipeline_cycles: int,
    min_isr_wakes: int,
) -> dict:
    text = payload.decode("ascii", errors="replace")
    lines = [line.strip() for line in text.splitlines() if line.strip()]
    identity = IDENTITY_PREFIX + firmware_commit
    failures: list[str] = []

    if lines.count(BANNER) != 1:
        failures.append(f"banner count {lines.count(BANNER)} != 1")
    if lines.count(identity) != 1:
        failures.append("firmware identity mismatch")

    identity_index = lines.index(identity) if identity in lines else -1
    banner_index = lines.index(BANNER) if BANNER in lines else -1
    if identity_index < 0 or banner_index < 0 or identity_index <= banner_index:
        failures.append("firmware identity is not ordered after banner")

    positions = []
    for marker in REQUIRED_ONESHOT:
        if lines.count(marker) != 1:
            failures.append(f"{marker} count {lines.count(marker)} != 1")
        positions.append(lines.index(marker) if marker in lines else -1)
    if -1 not in positions and positions != sorted(positions):
        failures.append("resource/timeout/lifecycle markers are not ordered")

    tim6_recovery_positions = []
    for marker in TIM6_RECOVERY:
        if lines.count(marker) != 1:
            failures.append("TIM6 IRQ recovery marker count mismatch")
        tim6_recovery_positions.append(lines.index(marker) if marker in lines else -1)
    if -1 not in tim6_recovery_positions and tim6_recovery_positions != sorted(
        tim6_recovery_positions
    ):
        failures.append("TIM6 IRQ recovery markers are not ordered")

    ipc_recovery_positions = []
    for marker in IPC_RECOVERY:
        if lines.count(marker) != 1:
            failures.append("IPC saturation/recovery marker count mismatch")
        ipc_recovery_positions.append(lines.index(marker) if marker in lines else -1)
    if -1 not in ipc_recovery_positions and ipc_recovery_positions != sorted(
        ipc_recovery_positions
    ):
        failures.append("IPC saturation/recovery markers are not ordered")

    if lines.count(MULTI_PRODUCER_COMPLETION) != 1:
        failures.append(
            f"{MULTI_PRODUCER_COMPLETION} count "
            f"{lines.count(MULTI_PRODUCER_COMPLETION)} != 1"
        )
    if lines.count(MULTI_CONSUMER_DISTRIBUTION) != 1:
        failures.append(
            f"{MULTI_CONSUMER_DISTRIBUTION} count "
            f"{lines.count(MULTI_CONSUMER_DISTRIBUTION)} != 1"
        )
    multi_consumer_take_counts = [lines.count(marker) for marker in MULTI_CONSUMER_TAKES]
    if sum(multi_consumer_take_counts) != EXPECTED_MULTI_MESSAGES:
        failures.append(
            f"multi-consumer take total {sum(multi_consumer_take_counts)} "
            f"!= {EXPECTED_MULTI_MESSAGES}"
        )
    if any(count == 0 for count in multi_consumer_take_counts):
        failures.append("both multi-consumers must receive payloads")

    cycles = count_ordered_cycles(lines)
    isr_wakes = lines.count("OSAL_ISR_TAKE")
    tim6_irq_wakes = lines.count("OSAL_TIM6_IRQ_TAKE")
    if cycles < min_pipeline_cycles:
        failures.append(f"pipeline cycles {cycles} < {min_pipeline_cycles}")
    if isr_wakes < min_isr_wakes:
        failures.append(f"ISR wakes {isr_wakes} < {min_isr_wakes}")
    if tim6_irq_wakes < min_isr_wakes:
        failures.append(f"TIM6 IRQ wakes {tim6_irq_wakes} < {min_isr_wakes}")

    errors = {marker: lines.count(marker) for marker in ERROR_MARKERS if marker in lines}
    if errors:
        failures.append("runtime error markers present")

    return {
        "status": "STRESS_REVIEW_CANDIDATE" if not failures else "STRESS_VALIDATION_FAILED",
        "firmware_commit": firmware_commit,
        "capture_duration_seconds": duration_seconds,
        "capture_bytes": len(payload),
        "capture_sha256": hashlib.sha256(payload).hexdigest(),
        "ordered_pipeline_cycles": cycles,
        "isr_take_count": isr_wakes,
        "tim6_irq_take_count": tim6_irq_wakes,
        "multi_consumer_take_counts": multi_consumer_take_counts,
        "error_markers": errors,
        "failures": failures,
        "scope": "bounded Pandora OSAL/FreeRTOS runtime stress candidate",
        "not_evidence_for": [
            "performance",
            "arbitrary peripheral IRQ",
            "STM32U5 runtime",
            "complete RTOS product qualification",
        ],
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--input", type=Path, required=True)
    parser.add_argument("--metadata", type=Path, required=True)
    parser.add_argument("--firmware-commit", required=True)
    parser.add_argument("--duration-seconds", type=int, required=True)
    parser.add_argument("--min-pipeline-cycles", type=int, required=True)
    parser.add_argument("--min-isr-wakes", type=int, required=True)
    args = parser.parse_args()

    try:
        firmware_commit = validate_commit(args.firmware_commit)
    except ValueError as error:
        parser.error(str(error))
    for name, value in (
        ("duration-seconds", args.duration_seconds),
        ("min-pipeline-cycles", args.min_pipeline_cycles),
        ("min-isr-wakes", args.min_isr_wakes),
    ):
        if value <= 0:
            parser.error(f"--{name} must be positive")

    payload = args.input.read_bytes()
    result = analyze_capture(
        payload,
        firmware_commit,
        args.duration_seconds,
        args.min_pipeline_cycles,
        args.min_isr_wakes,
    )
    result["recorded_at_utc"] = datetime.now(timezone.utc).isoformat()
    result["source_device"] = "/dev/serial/by-id/usb-wch.cn_WCH-Link_B49C8F0639CE-if01"
    args.metadata.parent.mkdir(parents=True, exist_ok=True)
    args.metadata.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(
        f"{result['status']} cycles={result['ordered_pipeline_cycles']} "
        f"isr_wakes={result['isr_take_count']} bytes={result['capture_bytes']}"
    )
    for failure in result["failures"]:
        print(f"- {failure}")
    return 0 if result["status"] == "STRESS_REVIEW_CANDIDATE" else 1


if __name__ == "__main__":
    sys.exit(main())
