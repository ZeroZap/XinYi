#!/usr/bin/env python3
"""Validate identity-bound Pandora PM sleep/wakeup UART evidence."""

import argparse
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import re
import sys

BANNER = "PANDORA STM32L475VE XINYI OSAL FREERTOS READY"
IDENTITY_PREFIX = "FIRMWARE_COMMIT "
PM_MARKERS = (
    "OSAL_PM_SLEEP_ENTER",
    "OSAL_PM_WAKE_IRQ",
    "OSAL_PM_SLEEP_WAKE_OK",
)
PM_REPEAT_CYCLES = 32
PM_CYCLE_PREFIX = "OSAL_PM_SLEEP_CYCLE "
PM_REPEAT_MARKER = "OSAL_PM_SLEEP_REPEAT_OK"
POST_REPEAT_PROGRESS_MARKER = "OSAL_IPC_SEND"
ERROR_MARKERS = (
    "OSAL_PM_SLEEP_ERROR",
    "OSAL_PM_WAKE_ERROR",
)


def validate_commit(revision: str) -> str:
    if re.fullmatch(r"[0-9a-f]{40}", revision) is None:
        raise ValueError("firmware commit must be an exact 40-character lowercase Git SHA")
    return revision


def analyze_capture(payload: bytes, firmware_commit: str) -> dict:
    text = payload.decode("ascii", errors="replace")
    lines = [line.strip() for line in text.splitlines() if line.strip()]
    identity = IDENTITY_PREFIX + firmware_commit
    marker_counts = {marker: lines.count(marker) for marker in PM_MARKERS + ERROR_MARKERS}
    marker_counts[PM_REPEAT_MARKER] = lines.count(PM_REPEAT_MARKER)
    failures: list[str] = []

    if not payload:
        failures.append("capture is empty")
    banner_positions = [index for index, line in enumerate(lines) if line.endswith(BANNER)]
    identity_positions = [index for index, line in enumerate(lines) if line == identity]
    if not banner_positions:
        failures.append("banner count 0")
    if not identity_positions:
        failures.append("firmware identity count 0")
    for marker in PM_MARKERS:
        if marker_counts[marker] != PM_REPEAT_CYCLES:
            failures.append(
                f"{marker} count {marker_counts[marker]} != {PM_REPEAT_CYCLES}"
            )
    if marker_counts[PM_REPEAT_MARKER] != 1:
        failures.append(f"{PM_REPEAT_MARKER} count {marker_counts[PM_REPEAT_MARKER]} != 1")

    ordered = [identity]
    for cycle in range(1, PM_REPEAT_CYCLES + 1):
        ordered.extend((*PM_MARKERS, f"{PM_CYCLE_PREFIX}{cycle}"))
    ordered.append(PM_REPEAT_MARKER)
    positions = []
    search_start = identity_positions[-1] if identity_positions else 0
    for marker in ordered:
        try:
            position = lines.index(marker, search_start)
        except ValueError:
            position = -1
        positions.append(position)
        if position >= 0:
            search_start = position + 1
    if -1 in positions:
        failures.append("PM markers are not strictly ordered")

    repeat_position = positions[-1]
    post_repeat_task_progress = False
    if repeat_position >= 0:
        next_identity_position = next(
            (position for position in identity_positions if position > repeat_position), len(lines)
        )
        post_repeat_task_progress = POST_REPEAT_PROGRESS_MARKER in lines[
            repeat_position + 1 : next_identity_position
        ]
    if not post_repeat_task_progress:
        failures.append("no task progress after PM repeat completion")

    error_marker_count = sum(marker_counts[marker] for marker in ERROR_MARKERS)
    if error_marker_count:
        failures.append(f"PM error marker count {error_marker_count} != 0")

    return {
        "record_type": "xinyi-pandora-stm32l475-pm-sleep-wakeup",
        "status": "REJECTED" if failures else "B1_PM_SLEEP_REPEAT_PASS",
        "firmware_commit": firmware_commit,
        "captured_bytes": len(payload),
        "capture_sha256": hashlib.sha256(payload).hexdigest(),
        "marker_counts": marker_counts,
        "boot_count": len(identity_positions),
        "error_marker_count": error_marker_count,
        "failures": failures,
        "repeat_cycles": PM_REPEAT_CYCLES,
        "post_repeat_task_progress": post_repeat_task_progress,
        "scope": "Pandora shallow SLEEP/WFI repeated through PM/HAL and resumed by configured tick IRQ",
        "not_evidence_for": [
            "power current",
            "STOP/STANDBY/SHUTDOWN mode",
            "wake latency or performance",
            "multi-hour endurance",
            "STM32U5 runtime",
        ],
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--capture", type=Path, required=True)
    parser.add_argument("--firmware-commit", required=True)
    parser.add_argument("--record", type=Path, required=True)
    args = parser.parse_args()

    try:
        firmware_commit = validate_commit(args.firmware_commit)
    except ValueError as error:
        parser.error(str(error))

    result = analyze_capture(args.capture.read_bytes(), firmware_commit)
    result["recorded_at_utc"] = datetime.now(timezone.utc).isoformat()
    result["source_device"] = "/dev/serial/by-id/usb-wch.cn_WCH-Link_B49C8F0639CE-if01"
    args.record.parent.mkdir(parents=True, exist_ok=True)
    args.record.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(
        f"{result['status']} bytes={result['captured_bytes']} "
        f"errors={result['error_marker_count']}"
    )
    for failure in result["failures"]:
        print(f"- {failure}")
    return 0 if result["status"] == "B1_PM_SLEEP_REPEAT_PASS" else 1


if __name__ == "__main__":
    sys.exit(main())
