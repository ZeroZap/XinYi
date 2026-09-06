#!/usr/bin/env python3
"""Validate bounded Pandora ISR-to-Broker ingress evidence from the last boot."""

import argparse
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import re
import sys

BANNER = "PANDORA STM32L475VE XINYI OSAL FREERTOS READY"
IDENTITY_PREFIX = "FIRMWARE_COMMIT "
MARKERS = (
    "OSAL_IPC_ISR_BACKPRESSURE",
    "OSAL_IPC_ISR_STREAM_DELIVER",
    "OSAL_IPC_TASK_PRODUCER_PROGRESS",
    "OSAL_IPC_ISR_RECOVERED",
    "OSAL_IPC_ISR_SUSTAINED_OK",
)
BOUNDED_EXPECTED_COUNTS = {
    "OSAL_IPC_ISR_BACKPRESSURE": 1,
    "OSAL_IPC_ISR_STREAM_DELIVER": 16,
    "OSAL_IPC_TASK_PRODUCER_PROGRESS": 1,
    "OSAL_IPC_ISR_RECOVERED": 1,
    "OSAL_IPC_ISR_SUSTAINED_OK": 1,
}
ERROR = "OSAL_IPC_ISR_ERROR"
PIPELINE = (
    "OSAL_IPC_SEND",
    "OSAL_PM_TICK",
    "OSAL_DEVICE_LOOKUP",
    "[I] OSAL_TRACE_DELIVER",
    "OSAL_IPC_DELIVER",
)


def count_ordered_cycles(lines: list[str], markers: tuple[str, ...]) -> int:
    cycles = 0
    state = 0
    for line in lines:
        if line == markers[state]:
            state += 1
            if state == len(markers):
                cycles += 1
                state = 0
        elif line == markers[0]:
            state = 1
    return cycles


def validate_commit(revision: str) -> str:
    if re.fullmatch(r"[0-9a-f]{40}", revision) is None:
        raise ValueError("firmware commit must be an exact 40-character lowercase Git SHA")
    return revision


def analyze_capture(
    payload: bytes,
    firmware_commit: str,
    duration_seconds: int | None = None,
    expected_recovery_cycles: int = 1,
    min_pipeline_cycles: int = 0,
) -> dict:
    text = payload.decode("ascii", errors="replace")
    lines = [line.strip() for line in text.splitlines() if line.strip()]
    identity = IDENTITY_PREFIX + firmware_commit
    failures: list[str] = []

    banner_positions = [index for index, line in enumerate(lines) if line == BANNER]
    if not banner_positions:
        failures.append("matching runtime banner missing")
        boot_lines: list[str] = []
    else:
        boot_lines = lines[banner_positions[-1] :]
    if boot_lines.count(identity) != 1:
        failures.append("last-boot firmware identity mismatch")

    normalized_boot_lines = [
        "OSAL_IPC_ISR_STREAM_DELIVER"
        if line.startswith("OSAL_IPC_ISR_STREAM_DELIVER ")
        else line
        for line in boot_lines
    ]
    sequence_values = []
    for line in boot_lines:
        match = re.fullmatch(r"OSAL_IPC_ISR_STREAM_DELIVER ([0-9]+)", line)
        if match is not None:
            sequence_values.append(int(match.group(1)))
    counts = {marker: normalized_boot_lines.count(marker) for marker in MARKERS}
    expected_counts = {
        "OSAL_IPC_ISR_BACKPRESSURE": expected_recovery_cycles,
        "OSAL_IPC_ISR_STREAM_DELIVER": 16 * expected_recovery_cycles,
        "OSAL_IPC_TASK_PRODUCER_PROGRESS": expected_recovery_cycles,
        "OSAL_IPC_ISR_RECOVERED": expected_recovery_cycles,
        "OSAL_IPC_ISR_SUSTAINED_OK": 1,
    }
    if counts != expected_counts:
        failures.append("IPC ISR ingress marker count mismatch")
    recovery_cycle = (
        "OSAL_IPC_ISR_BACKPRESSURE",
        *("OSAL_IPC_ISR_STREAM_DELIVER",) * 16,
        "OSAL_IPC_TASK_PRODUCER_PROGRESS",
        "OSAL_IPC_ISR_RECOVERED",
    )
    if count_ordered_cycles(normalized_boot_lines, recovery_cycle) != expected_recovery_cycles:
        failures.append("IPC ISR recovery cycle ordering/count mismatch")
    if expected_recovery_cycles > 1 and (
        len(sequence_values) != 16 * expected_recovery_cycles
        or any(current <= previous for previous, current in zip(sequence_values, sequence_values[1:]))
    ):
        failures.append("IPC ISR payload sequence is not strictly monotonic delivery")
    if counts["OSAL_IPC_TASK_PRODUCER_PROGRESS"] != expected_recovery_cycles:
        failures.append("IPC task producer made no bounded progress")
    pipeline_cycles = count_ordered_cycles(boot_lines, PIPELINE)
    if pipeline_cycles < min_pipeline_cycles:
        failures.append("cross-component pipeline continuity mismatch")
    if duration_seconds is not None and duration_seconds < 600:
        failures.append(f"capture duration {duration_seconds} < 600 seconds")
    if ERROR in boot_lines:
        failures.append("IPC ISR ingress error marker present")

    return {
        "status": (
            "IPC_ISR_STRESS_REVIEW_CANDIDATE"
            if not failures and expected_recovery_cycles > 1
            else "IPC_ISR_REVIEW_CANDIDATE" if not failures else "IPC_ISR_VALIDATION_FAILED"
        ),
        "firmware_commit": firmware_commit,
        "capture_bytes": len(payload),
        "capture_sha256": hashlib.sha256(payload).hexdigest(),
        "marker_counts": counts,
        "ordered_pipeline_cycles": pipeline_cycles,
        "failures": failures,
        "scope": "last-boot bounded Pandora single TIM6 ISR ingress repeated recovery stress",
        "not_evidence_for": ["performance throughput", "multi-ISR producer", "multi-hour endurance"],
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--input", type=Path, required=True)
    parser.add_argument("--metadata", type=Path, required=True)
    parser.add_argument("--firmware-commit", required=True)
    parser.add_argument("--duration-seconds", type=int)
    parser.add_argument("--expected-recovery-cycles", type=int, default=1)
    parser.add_argument("--min-pipeline-cycles", type=int, default=0)
    args = parser.parse_args()
    try:
        firmware_commit = validate_commit(args.firmware_commit)
    except ValueError as error:
        parser.error(str(error))
    result = analyze_capture(
        args.input.read_bytes(), firmware_commit, args.duration_seconds,
        args.expected_recovery_cycles, args.min_pipeline_cycles,
    )
    result["recorded_at_utc"] = datetime.now(timezone.utc).isoformat()
    result["source_device"] = "/dev/serial/by-id/usb-wch.cn_WCH-Link_B49C8F0639CE-if01"
    args.metadata.parent.mkdir(parents=True, exist_ok=True)
    args.metadata.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(result["status"])
    for failure in result["failures"]:
        print(f"- {failure}")
    return 0 if not result["failures"] else 1


if __name__ == "__main__":
    sys.exit(main())
