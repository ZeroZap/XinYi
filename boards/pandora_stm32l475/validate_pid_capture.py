#!/usr/bin/env python3
"""Validate an identity-bound Pandora PID board runtime capture.

Why this exists
---------------
The PID component has Host tests, but those stub `xy_os_tick_get()` with a
variable, so the elapsed interval that drives `dt` has never been the real one.
The board image therefore runs the controller against the FreeRTOS tick, and
this validator is what turns the resulting UART bytes into a verdict instead of
a log a human eyeballs.

It is deliberately fail-closed and identity-bound:

  * the capture must carry the *exact* firmware commit that was flashed, so a
    capture from a different build can never be presented as this one;
  * the four check markers must appear in the order the image emits them, and
    the terminal marker must be last, so a truncated or reordered capture fails;
  * any error marker -- including one that names the failing step -- fails the
    capture, and the step number is reported so the failure is actionable.

Usage
-----
  boards/pandora_stm32l475/validate_pid_capture.py capture.log \\
      --firmware-commit <40-hex-sha> [--json result.json]

Exit code 0 means the capture carries the terminal pass marker with the
expected identity; 1 means it does not; 2 means the capture was unreadable.
"""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import re

BANNER = "PANDORA PID BOARD READY"
IDENTITY_PREFIX = "FIRMWARE_COMMIT "
TERMINAL = "B1_PID_BOARD_RUNTIME_PASS"

# The checks, in the order the image runs them. Each entry is the marker that
# proves the check passed; the image emits the matching *_ERROR_STEP_<n>
# marker instead when the check fails.
CHECKS: tuple[tuple[str, str], ...] = (
    ("PANDORA_PID_LOOP_OK_A", "PANDORA_PID_LOOP_ERROR_STEP_"),
    ("PANDORA_PID_WINDUP_OK_A", "PANDORA_PID_WINDUP_ERROR_STEP_"),
    ("PANDORA_PID_CONTRACT_OK_A", "PANDORA_PID_CONTRACT_ERROR_STEP_"),
    ("PANDORA_PID_RESET_OK_A", "PANDORA_PID_RESET_ERROR_STEP_"),
)

# The image's own fault handler prints nothing before it spins, so a crash is
# reported by the absence of the terminal marker rather than by a marker.
ERROR_MARKERS = (
    "HardFault",
    "ASSERT",
)


def analyze_capture(payload: bytes, firmware_commit: str) -> dict:
    """Judge a PID board capture and return the verdict as a dictionary."""
    failures: list[str] = []

    if re.fullmatch(r"[0-9a-f]{40}", firmware_commit) is None:
        failures.append("invalid exact firmware commit")

    text = payload.decode("ascii", errors="replace")
    lines = [line.strip() for line in text.replace("\r", "").splitlines() if line.strip()]

    banner_positions = [index for index, line in enumerate(lines) if line == BANNER]
    if not banner_positions:
        failures.append("missing firmware banner")
        cycle: list[str] = []
    else:
        # The last banner wins: a capture that spans two boots is judged on the
        # boot that produced the terminal marker.
        cycle = lines[banner_positions[-1] :]

    expected_identity = IDENTITY_PREFIX + firmware_commit
    if len(cycle) < 2 or cycle[1] != expected_identity:
        failures.append("missing or out-of-order exact firmware identity")

    for marker in ERROR_MARKERS:
        if any(marker in line for line in cycle):
            failures.append(f"error marker present: {marker}")

    # The error markers are matched before the pass markers so a failing image
    # reports *which* check failed instead of only "a marker is missing".
    reported_steps: list[str] = []
    for pass_marker, error_prefix in CHECKS:
        error_index = None
        for index, line in enumerate(cycle):
            if line.startswith(error_prefix):
                error_index = index
                break
        if error_index is not None:
            step = cycle[error_index][len(error_prefix) :]
            if not re.fullmatch(r"[0-9]+", step):
                failures.append(
                    f"error marker {error_prefix!r} has a malformed step field {step!r}"
                )
            reported_steps.append(f"{error_prefix}{step}")
            failures.append(f"error marker present: {error_prefix}{step}")
            continue
        if pass_marker not in cycle:
            failures.append(f"missing check marker: {pass_marker}")

    # The pass markers must appear in the order the image emits them.
    positions = []
    for pass_marker, _ in CHECKS:
        position = cycle.index(pass_marker) if pass_marker in cycle else -1
        positions.append(position)
    present = [position for position in positions if position >= 0]
    if present != sorted(present):
        failures.append("check markers are out of order")

    if not cycle or cycle[-1] != TERMINAL:
        failures.append("missing or out-of-order completion marker")

    return {
        "status": "B1_PID_BOARD_RUNTIME_PASS" if not failures else "PID_VALIDATION_FAILED",
        "firmware_commit": firmware_commit,
        "capture_bytes": len(payload),
        "capture_sha256": hashlib.sha256(payload).hexdigest(),
        "checks": [pass_marker for pass_marker, _ in CHECKS],
        "checks_passed": len(present),
        "error_steps": reported_steps,
        "failures": failures,
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("capture", type=Path)
    parser.add_argument("--firmware-commit", required=True)
    parser.add_argument("--json", type=Path)
    args = parser.parse_args()

    try:
        payload = args.capture.read_bytes()
    except OSError as error:
        result = {
            "status": "PID_VALIDATION_FAILED",
            "firmware_commit": args.firmware_commit,
            "failures": [f"capture unreadable: {error}"],
        }
    else:
        result = analyze_capture(payload, args.firmware_commit)

    rendered = json.dumps(result, indent=2, sort_keys=True) + "\n"
    if args.json is not None:
        args.json.parent.mkdir(parents=True, exist_ok=True)
        args.json.write_text(rendered, encoding="utf-8")
    print(rendered, end="")
    return 0 if result["status"] == "B1_PID_BOARD_RUNTIME_PASS" else 1


if __name__ == "__main__":
    raise SystemExit(main())
