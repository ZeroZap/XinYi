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
EXPECTED_COUNTS = {
    "OSAL_IPC_ISR_BACKPRESSURE": 1,
    "OSAL_IPC_ISR_STREAM_DELIVER": 16,
    "OSAL_IPC_TASK_PRODUCER_PROGRESS": 1,
    "OSAL_IPC_ISR_RECOVERED": 1,
    "OSAL_IPC_ISR_SUSTAINED_OK": 1,
}
ERROR = "OSAL_IPC_ISR_ERROR"


def validate_commit(revision: str) -> str:
    if re.fullmatch(r"[0-9a-f]{40}", revision) is None:
        raise ValueError("firmware commit must be an exact 40-character lowercase Git SHA")
    return revision


def analyze_capture(payload: bytes, firmware_commit: str) -> dict:
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

    counts = {marker: boot_lines.count(marker) for marker in MARKERS}
    if counts != EXPECTED_COUNTS:
        failures.append("IPC ISR ingress marker count mismatch")
    positions = [boot_lines.index(marker) if marker in boot_lines else -1 for marker in MARKERS]
    if -1 not in positions and positions != sorted(positions):
        failures.append("IPC ISR ingress markers are not ordered")
    if counts["OSAL_IPC_TASK_PRODUCER_PROGRESS"] != 1:
        failures.append("IPC task producer made no bounded progress")
    if ERROR in boot_lines:
        failures.append("IPC ISR ingress error marker present")

    return {
        "status": "IPC_ISR_REVIEW_CANDIDATE" if not failures else "IPC_ISR_VALIDATION_FAILED",
        "firmware_commit": firmware_commit,
        "capture_bytes": len(payload),
        "capture_sha256": hashlib.sha256(payload).hexdigest(),
        "marker_counts": counts,
        "failures": failures,
        "scope": "last-boot bounded Pandora single TIM6 ISR ingress sustained recovery",
        "not_evidence_for": ["performance throughput", "multi-ISR producer", "long-duration stress"],
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--input", type=Path, required=True)
    parser.add_argument("--metadata", type=Path, required=True)
    parser.add_argument("--firmware-commit", required=True)
    args = parser.parse_args()
    try:
        firmware_commit = validate_commit(args.firmware_commit)
    except ValueError as error:
        parser.error(str(error))
    result = analyze_capture(args.input.read_bytes(), firmware_commit)
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
