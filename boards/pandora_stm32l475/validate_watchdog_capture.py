#!/usr/bin/env python3
"""Validate an identity-bound Pandora watchdog refresh-policy board capture.

Why this exists
---------------
The SYS row of `docs/validation/component-evidence-matrix.md` still lists
"watchdog refresh policy" as pending. The reset-reason half of that row is closed:
S2-04 and S5-107 already produced write/verify/read-back plus UART evidence for a
software reset, an IWDG timeout and an ST-Link pin reset.

This validator judges the other half -- the policy itself -- from a capture of the
`watchdog_main.c` board image, and it is deliberately fail-closed and
identity-bound:

  * the capture must carry the *exact* firmware commit that was flashed, so a
    capture from a different build can never be presented as this one;
  * both phases must be present in the right order. The feed phase proves the
    board survived a span that is a large multiple of the armed window, which is
    only possible because the feeds happened; the report phase proves the board
    then died of a *watchdog* reset when the feeds stopped;
  * any error marker fails the capture, and the failing step is reported.

The negative control is the point of the whole exercise. A capture that only shows
"the board survived" is indistinguishable from a capture of an inert watchdog: if
the watchdog were not really armed, nothing would ever reset the board and the
report phase would simply be missing. Requiring the report phase -- and requiring
it to name the watchdog as the cause -- is what makes the survival meaningful.

The capture spans a reset, so it contains two boots' worth of output and the
partial line that a reset cuts in half. The validator therefore judges the two
phases separately and tolerates a truncated line at the phase boundary rather than
requiring the whole capture to be well formed.

Usage
-----
  boards/pandora_stm32l475/validate_watchdog_capture.py capture.log \\
      --firmware-commit <40-hex-sha> [--json result.json]

Exit code 0 means the capture carries the terminal pass marker with the expected
identity; 1 means it does not; 2 means the capture was unreadable.
"""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import re

BANNER = "PANDORA WATCHDOG REFRESH READY"
IDENTITY_PREFIX = "FIRMWARE_COMMIT "
TERMINAL = "WATCHDOG_REFRESH_POLICY_PASS"

PHASE_FEED = "WATCHDOG_PHASE FEED"
PHASE_REPORT = "WATCHDOG_PHASE REPORT"
ARMED = "WATCHDOG_ARMED"
SURVIVED = "WATCHDOG_REFRESH_SURVIVED"
MARKER_SET = "WATCHDOG_MARKER_SET"
FEED_STOPPED = "WATCHDOG_FEED_STOPPED"
EXPIRY_OK = "WATCHDOG_EXPIRY_RESET_OK"
RESET_KIND = "SYS_RESET_KIND "

# The report phase has to name the watchdog as the cause of the reset that
# followed the stopped feed. Any other cause means the reset was not the
# watchdog's doing, so the refresh claim above it was decoration.
EXPECTED_RESET_KIND = "WATCHDOG"

# The error markers the image can emit. Each one fails the capture and is
# reported, so a failing image says which step failed instead of only "a marker is
# missing".
ERROR_MARKERS = (
    "WATCHDOG_INIT_ERROR",
    "WATCHDOG_WINDOW_ERROR",
    "WATCHDOG_FEED_ERROR",
    "WATCHDOG_MARKER_ERROR",
    "WATCHDOG_RESET_CAUSE_MISMATCH",
    "HardFault",
    "ASSERT",
)


def _parse_marker_fields(text: str, prefix: str) -> dict[str, str]:
    """Parse the ``key=value`` fields of a marker line into a dictionary."""
    fields: dict[str, str] = {}
    for token in text.split():
        if "=" in token:
            key, _, value = token.partition("=")
            fields[key] = value
    return fields


def analyze_capture(payload: bytes, firmware_commit: str) -> dict:
    """Judge a watchdog refresh-policy board capture and return the verdict."""
    failures: list[str] = []

    if re.fullmatch(r"[0-9a-f]{40}", firmware_commit) is None:
        failures.append("invalid exact firmware commit")

    text = payload.decode("ascii", errors="replace")
    lines = [line.strip() for line in text.replace("\r", "").splitlines() if line.strip()]

    banner_positions = [index for index, line in enumerate(lines) if line == BANNER]
    if not banner_positions:
        failures.append("missing firmware banner")
        return {
            "status": "WATCHDOG_VALIDATION_FAILED",
            "firmware_commit": firmware_commit,
            "capture_bytes": len(payload),
            "capture_sha256": hashlib.sha256(payload).hexdigest(),
            "checks_passed": 0,
            "error_steps": [],
            "failures": failures,
        }

    # The capture spans a reset, so it holds more than one boot. The feed phase
    # comes first and the report phase second; each is judged on the boot that
    # produced it. The banner string repeats, so the boot boundaries are the
    # banner *positions*, and each boot runs from its own banner to the next one.
    identity = IDENTITY_PREFIX + firmware_commit
    feed_boot = None
    report_boot = None
    feed_boot_start = -1
    report_boot_start = -1
    for index, position in enumerate(banner_positions):
        end = banner_positions[index + 1] if index + 1 < len(banner_positions) else len(lines)
        boot = lines[position:end]
        if PHASE_FEED in boot and feed_boot is None:
            feed_boot = boot
            feed_boot_start = position
        if PHASE_REPORT in boot and report_boot is None:
            report_boot = boot
            report_boot_start = position

    checks_passed = 0
    reported_steps: list[str] = []

    for marker in ERROR_MARKERS:
        if any(marker in line for line in lines):
            failures.append(f"error marker present: {marker}")
            reported_steps.append(marker)

    # --- the feed phase -----------------------------------------------------
    feed_fields: dict[str, str] = {}
    if feed_boot is None:
        failures.append(f"missing feed phase marker: {PHASE_FEED}")
    else:
        if len(feed_boot) < 2 or feed_boot[1] != identity:
            failures.append("missing or out-of-order exact firmware identity in the feed phase")

        armed_lines = [line for line in feed_boot if line.startswith(ARMED)]
        if not armed_lines:
            failures.append(f"missing marker: {ARMED}")
        else:
            armed_fields = _parse_marker_fields(armed_lines[0], ARMED)
            feed_fields = armed_fields
            try:
                window_ms = int(armed_fields["nominal_window_ms"])
            except (KeyError, ValueError):
                failures.append(f"{ARMED} marker has no usable nominal_window_ms")
                window_ms = 0
            try:
                interval_ms = int(armed_fields["feed_interval_ms"])
            except (KeyError, ValueError):
                failures.append(f"{ARMED} marker has no usable feed_interval_ms")
                interval_ms = 0
            try:
                cycles = int(armed_fields["cycles"])
            except (KeyError, ValueError):
                failures.append(f"{ARMED} marker has no usable cycles")
                cycles = 0

            if window_ms <= 0:
                failures.append(f"{ARMED} marker reports a non-positive nominal window")
            if interval_ms <= 0:
                failures.append(f"{ARMED} marker reports a non-positive feed interval")
            if cycles <= 0:
                failures.append(f"{ARMED} marker reports a non-positive cycle count")
            if window_ms > 0 and interval_ms > 0 and interval_ms >= window_ms:
                # A feed interval at or beyond the window cannot prove anything:
                # the board would be racing the expiry instead of being kept alive
                # by the refresh.
                failures.append(
                    f"feed interval {interval_ms} ms is not shorter than the armed window "
                    f"{window_ms} ms"
                )
            checks_passed += 1

        feed_cycles = [line for line in feed_boot if line.startswith("WATCHDOG_FEED cycle=")]
        if not feed_cycles:
            failures.append("missing marker: WATCHDOG_FEED")
        else:
            seen: list[int] = []
            for line in feed_cycles:
                fields = _parse_marker_fields(line, "WATCHDOG_FEED")
                try:
                    seen.append(int(fields["cycle"]))
                except (KeyError, ValueError):
                    failures.append(f"feed marker has a malformed cycle field: {line!r}")
            if seen != sorted(seen) or len(set(seen)) != len(seen):
                failures.append("feed markers are out of order or repeated")
            # The armed marker declares how many cycles the image intended, so a
            # capture that shows fewer is a truncated run rather than a shorter
            # one. Without this, dropping a feed marker would still pass.
            try:
                declared_cycles = int(feed_fields.get("cycles", "0"))
            except ValueError:
                declared_cycles = 0
            if declared_cycles > 0 and len(seen) != declared_cycles:
                failures.append(
                    f"observed {len(seen)} feed markers, but the armed marker declared "
                    f"{declared_cycles} cycles"
                )
            elif declared_cycles > 0 and seen != list(range(1, declared_cycles + 1)):
                failures.append("feed markers do not cover the declared cycle sequence")
            checks_passed += 1

        survived_lines = [line for line in feed_boot if line.startswith(SURVIVED)]
        if not survived_lines:
            failures.append(f"missing marker: {SURVIVED}")
        else:
            survived_fields = _parse_marker_fields(survived_lines[0], SURVIVED)
            try:
                survived_ms = int(survived_fields["elapsed_ms"])
            except (KeyError, ValueError):
                failures.append(f"{SURVIVED} marker has no usable elapsed_ms")
                survived_ms = 0
            try:
                window_ms = int(feed_fields["nominal_window_ms"])
            except (KeyError, ValueError):
                window_ms = 0
            if survived_ms <= 0:
                failures.append(f"{SURVIVED} marker reports a non-positive elapsed time")
            if window_ms > 0 and survived_ms < window_ms:
                # The board did not even outlive one window, so the feeds cannot
                # be what kept it alive.
                failures.append(
                    f"the board survived {survived_ms} ms, which is shorter than the armed "
                    f"window {window_ms} ms"
                )
            checks_passed += 1

        if not any(line.startswith(MARKER_SET) for line in feed_boot):
            failures.append(f"missing marker: {MARKER_SET}")
        if not any(line.startswith(FEED_STOPPED) for line in feed_boot):
            failures.append(f"missing marker: {FEED_STOPPED}")

        # The feed phase must have stopped before the report phase begins.
        if report_boot is not None and feed_boot is not None:
            if report_boot_start < feed_boot_start:
                failures.append("the report phase began before the feed phase")

    # --- the report phase ----------------------------------------------------
    if report_boot is None:
        failures.append(f"missing report phase marker: {PHASE_REPORT}")
    else:
        if len(report_boot) < 2 or report_boot[1] != identity:
            failures.append("missing or out-of-order exact firmware identity in the report phase")

        kind_lines = [line for line in report_boot if line.startswith(RESET_KIND)]
        if not kind_lines:
            failures.append(f"missing marker: {RESET_KIND.strip()}")
        else:
            kind = kind_lines[0][len(RESET_KIND) :].strip()
            if kind != EXPECTED_RESET_KIND:
                failures.append(
                    f"the report phase reset cause is {kind!r}, not {EXPECTED_RESET_KIND!r}: "
                    "the board did not die of a watchdog reset"
                )
            checks_passed += 1

        if not any(line.startswith(EXPIRY_OK) for line in report_boot):
            failures.append(f"missing marker: {EXPIRY_OK}")
        else:
            checks_passed += 1

        if not any(line == TERMINAL for line in report_boot):
            failures.append(f"missing completion marker: {TERMINAL}")
        else:
            checks_passed += 1

    # The terminal marker is what a passing capture ends with. It has to be the
    # last marker of the report phase, so a truncated or reordered capture fails.
    if report_boot is not None and report_boot and report_boot[-1] != TERMINAL:
        failures.append("missing or out-of-order completion marker")

    return {
        "status": "B1_WATCHDOG_REFRESH_POLICY_PASS" if not failures else "WATCHDOG_VALIDATION_FAILED",
        "firmware_commit": firmware_commit,
        "capture_bytes": len(payload),
        "capture_sha256": hashlib.sha256(payload).hexdigest(),
        "boots_seen": len(banner_positions),
        "checks_passed": checks_passed,
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
            "status": "WATCHDOG_VALIDATION_FAILED",
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
    return 0 if result["status"] == "B1_WATCHDOG_REFRESH_POLICY_PASS" else 1


if __name__ == "__main__":
    raise SystemExit(main())
