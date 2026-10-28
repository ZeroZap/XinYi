#!/usr/bin/env python3
"""Validate identity-bound Pandora PM charger-boundary and HAL shallow-sleep evidence.

Pandora carries no charger IC, so ``xy_charger_hw_init()`` fails closed and
``xy_pm_init()`` can never succeed on this board. PM *system* sleep is therefore
unreachable here; what the runtime image must prove instead is that the boundary holds,
nothing is left half-initialised, and the HAL shallow-sleep path the PM platform layer
would call still executes and returns.
"""

import argparse
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import re
import sys

BANNER = "PANDORA STM32L475VE XINYI OSAL FREERTOS READY"
IDENTITY_PREFIX = "FIRMWARE_COMMIT "
REFUSAL_PREFIX = "OSAL_PM_INIT_REFUSED "
BOUNDARY_MARKER = "OSAL_PM_CHARGER_BOUNDARY_OK"
PM_REPEAT_CYCLES = 32
PM_CYCLE_PREFIX = "OSAL_PM_HAL_SLEEP_CYCLE "
PM_REPEAT_MARKER = "OSAL_PM_HAL_SLEEP_REPEAT_OK"
ERROR_MARKERS = (
    "OSAL_PM_CHARGER_BOUNDARY_MISSING",
    "OSAL_PM_FAIL_CLOSED_ERROR",
    "OSAL_PM_HAL_SLEEP_ERROR",
)


def validate_commit(revision: str) -> str:
    if re.fullmatch(r"[0-9a-f]{40}", revision) is None:
        raise ValueError("firmware commit must be an exact 40-character lowercase Git SHA")
    return revision


def analyze_capture(payload: bytes, firmware_commit: str) -> dict:
    text = payload.decode("ascii", errors="replace")
    lines = [line.strip() for line in text.splitlines() if line.strip()]
    identity = IDENTITY_PREFIX + firmware_commit
    marker_counts = {marker: lines.count(marker) for marker in ERROR_MARKERS}
    marker_counts[BOUNDARY_MARKER] = lines.count(BOUNDARY_MARKER)
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

    if marker_counts[BOUNDARY_MARKER] != 1:
        failures.append(f"{BOUNDARY_MARKER} count {marker_counts[BOUNDARY_MARKER]} != 1")
    if marker_counts[PM_REPEAT_MARKER] != 1:
        failures.append(f"{PM_REPEAT_MARKER} count {marker_counts[PM_REPEAT_MARKER]} != 1")

    # The refusal value is the raw error xy_pm_init() propagated out of the fail-closed
    # charger backend. Zero would mean init succeeded, i.e. the boundary did not hold.
    refusal_values = [
        line[len(REFUSAL_PREFIX):].strip()
        for line in lines
        if line.startswith(REFUSAL_PREFIX)
    ]
    refusal_value = refusal_values[-1] if refusal_values else ""
    if not refusal_values:
        failures.append("PM initialisation refusal was never reported")
    elif refusal_value in ("", "0"):
        failures.append("PM initialisation was not refused, charger boundary did not hold")

    cycles: list[int] = []
    for line in lines:
        if line.startswith(PM_CYCLE_PREFIX):
            suffix = line[len(PM_CYCLE_PREFIX):].strip()
            if suffix.isdigit() and suffix.isascii():
                cycles.append(int(suffix))
    if len(cycles) != PM_REPEAT_CYCLES:
        failures.append(f"HAL sleep cycle count {len(cycles)} != {PM_REPEAT_CYCLES}")
    elif cycles != list(range(1, PM_REPEAT_CYCLES + 1)):
        failures.append("HAL sleep cycles are not each reported exactly once, in order")

    ordered = []
    if refusal_values:
        ordered.append(REFUSAL_PREFIX + refusal_value)
    ordered.append(BOUNDARY_MARKER)
    for cycle in range(1, PM_REPEAT_CYCLES + 1):
        ordered.append(f"{PM_CYCLE_PREFIX}{cycle}")
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

    error_marker_count = sum(marker_counts[marker] for marker in ERROR_MARKERS)
    if error_marker_count:
        failures.append(f"PM error marker count {error_marker_count} != 0")

    return {
        "record_type": "xinyi-pandora-stm32l475-pm-charger-boundary-hal-sleep",
        "status": "REJECTED" if failures else "B1_PM_CHARGER_FAILCLOSED_HAL_SLEEP_PASS",
        "firmware_commit": firmware_commit,
        "captured_bytes": len(payload),
        "capture_sha256": hashlib.sha256(payload).hexdigest(),
        "marker_counts": marker_counts,
        "boot_count": len(identity_positions),
        "error_marker_count": error_marker_count,
        "failures": failures,
        "repeat_cycles": PM_REPEAT_CYCLES,
        "pm_init_refusal": refusal_value,
        "scope": (
            "Pandora PM charger fail-closed boundary (no charger IC on this board) plus "
            "HAL shallow SLEEP/WFI entered and resumed 32 times from a task"
        ),
        "not_evidence_for": [
            "PM system sleep/wake: xy_pm_init() cannot succeed without a charger IC",
            "power current",
            "STOP/STANDBY/SHUTDOWN mode",
            "wake latency or performance",
            "multi-hour endurance",
            "STM32U5 runtime",
            "any charger hardware behaviour",
        ],
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--capture", type=Path, required=True)
    parser.add_argument("--firmware-commit", required=True)
    parser.add_argument("--record", type=Path, required=True)
    # Pandora evidence comes out of the CP2102 UART only; the ST-Link/WCH-Link virtual
    # serial is not an evidence channel on this board, so the source has to be stated
    # rather than assumed.
    parser.add_argument("--source-device", default="/dev/ttyUSB0")
    args = parser.parse_args()

    try:
        firmware_commit = validate_commit(args.firmware_commit)
    except ValueError as error:
        parser.error(str(error))

    result = analyze_capture(args.capture.read_bytes(), firmware_commit)
    result["recorded_at_utc"] = datetime.now(timezone.utc).isoformat()
    result["source_device"] = args.source_device
    args.record.parent.mkdir(parents=True, exist_ok=True)
    args.record.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(
        f"{result['status']} bytes={result['captured_bytes']} "
        f"errors={result['error_marker_count']}"
    )
    for failure in result["failures"]:
        print(f"- {failure}")
    return 0 if result["status"] == "B1_PM_CHARGER_FAILCLOSED_HAL_SLEEP_PASS" else 1


if __name__ == "__main__":
    sys.exit(main())
