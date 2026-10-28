#!/usr/bin/env python3
"""Measure the Pandora OSAL runtime timeline instead of inferring it from source.

Why this exists
---------------
`rtos_main.c` writes every marker through `xy_hal_uart_send()` inside a
`__disable_irq()` window, and one 40 KiB runtime image has produced both a complete
32-cycle chain and runs that advanced only 2-4 fast/slow cycles inside the same 70 s
window, with zero error markers either way. The suspected cause (the logger masks the
kernel tick for the whole blocking send, ~1.7 ms per line at 115200) was read off the
source and never measured, and the image cannot time itself: adding timestamps to the
existing markers would change the vocabulary the other validators pin.

So the timebase stays outside the board. `capture` records the host arrival offset of
every line, and `analyze` turns those offsets into per-cycle wall time, the largest
marker-to-marker gaps, and the excess wall time of the windows whose only firmware work
is a known `xy_os_delay()`.

What the numbers mean
---------------------
`delay_window.excess_ms` is wall time inside a window that asks for a fixed number of
kernel ticks (for example the fast task's `xy_os_delay(500)`), minus the requested
duration. Something did not deliver those ticks on schedule: either the tick interrupt
was masked, or the task was not scheduled while the tick ran. This tool separates
*when* the wall clock went and how large that excess is; it does not by itself attribute
the excess to the logger, and it is not a latency, jitter, or throughput measurement.

Usage
-----
  validate_runtime_timeline.py capture --device /dev/ttyUSB0 --seconds 70 \
      --reset-command "st-flash reset" --output run.log --timeline run.timeline.json
  validate_runtime_timeline.py analyze --timeline run.timeline.json \
      --firmware-commit <sha> --record run.timeline-record.json
"""

from __future__ import annotations

import argparse
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import re
import select
import subprocess
import sys
import termios
import time

BANNER = "PANDORA STM32L475VE XINYI OSAL FREERTOS READY"
IDENTITY_PREFIX = "FIRMWARE_COMMIT "
CYCLE_START = "OSAL_TASK_FAST"
CYCLE_END = "OSAL_TASK_SLOW"
DEFAULT_DELAY_FROM = "OSAL_PM_TICK"
DEFAULT_DELAY_TO = "OSAL_MUTEX_FAST"
DEFAULT_REQUESTED_DELAY_MS = 500.0
DEFAULT_MAX_CYCLE_MS = 1500.0
DEFAULT_MIN_CYCLES = 2
DEFAULT_TOP_GAPS = 8
BAUDRATE = 115200


def validate_commit(revision: str) -> str:
    if re.fullmatch(r"[0-9a-f]{40}", revision) is None:
        raise ValueError("firmware commit must be an exact 40-character lowercase Git SHA")
    return revision


def write_json(path: Path, record: dict) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(record, indent=2, sort_keys=True) + "\n", encoding="utf-8")


def configure_uart(fd: int) -> None:
    attrs = termios.tcgetattr(fd)
    attrs[0] = 0
    attrs[1] = 0
    attrs[2] = termios.CS8 | termios.CREAD | termios.CLOCAL
    attrs[3] = 0
    attrs[4] = termios.B115200
    attrs[5] = termios.B115200
    attrs[6][termios.VMIN] = 0
    attrs[6][termios.VTIME] = 0
    termios.tcsetattr(fd, termios.TCSANOW, attrs)
    termios.tcflush(fd, termios.TCIFLUSH)


def split_lines(payload: bytes, stamps: list[tuple[int, float]]) -> list[dict]:
    """Turn timestamped chunks into (arrival_ms, text) lines.

    A line's timestamp is the arrival time of the chunk that carried it. The firmware
    writes one marker per blocking send, so chunk arrivals are the marker arrivals;
    a trailing partial line stays with the chunk that carried it and is still reported.
    """
    lines: list[dict] = []
    carried = b""
    carried_ms = 0.0
    previous_end = 0
    for end, t_ms in stamps:
        chunk = carried + payload[previous_end:end]
        previous_end = end
        carried = b""
        carried_ms = t_ms
        pieces = chunk.split(b"\n")
        carried = pieces.pop()
        for piece in pieces:
            text = piece.replace(b"\r", b"").strip()
            if text:
                lines.append({"t_ms": round(t_ms, 3), "text": text.decode("ascii", "replace")})
    tail = carried.replace(b"\r", b"").strip()
    if tail:
        lines.append({"t_ms": round(carried_ms, 3), "text": tail.decode("ascii", "replace")})
    return lines


def capture(
    device: str,
    seconds: float,
    reset_command: str | None,
    arm_delay: float,
    output: Path,
    timeline_path: Path,
    clock=time.monotonic,
) -> dict:
    record: dict = {
        "record_type": "xinyi-pandora-stm32l475-runtime-timeline-capture",
        "device": device,
        "baudrate": BAUDRATE,
        "requested_seconds": seconds,
        "reset_command": reset_command,
        "status": "REJECTED",
    }
    try:
        fd = os.open(device, os.O_RDONLY | os.O_NOCTTY | os.O_NONBLOCK)
    except OSError as error:
        record["error"] = f"device open failed: {error}"
        record["captured_bytes"] = 0
        write_json(timeline_path, record)
        return record

    payload = bytearray()
    stamps: list[tuple[int, float]] = []
    try:
        configure_uart(fd)
        if reset_command:
            # The port is open and flushed first, so the boot banner cannot be missed.
            if arm_delay > 0:
                time.sleep(arm_delay)
            subprocess.run(reset_command.split(), check=True, capture_output=True)
        start = clock()
        deadline = start + seconds
        while True:
            remaining = deadline - clock()
            if remaining <= 0:
                break
            readable, _, _ = select.select([fd], [], [], remaining)
            if not readable:
                break
            chunk = os.read(fd, 4096)
            if not chunk:
                break
            payload.extend(chunk)
            stamps.append((len(payload), (clock() - start) * 1000.0))
    except OSError as error:
        record["error"] = f"capture io failed: {error}"
    except subprocess.CalledProcessError as error:
        record["error"] = f"reset command failed: {error}"
    finally:
        os.close(fd)

    lines = split_lines(bytes(payload), stamps)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(bytes(payload))
    record.update(
        {
            "captured_bytes": len(payload),
            "capture_sha256": hashlib.sha256(bytes(payload)).hexdigest(),
            "captured_at_utc": datetime.now(timezone.utc).isoformat(),
            "line_count": len(lines),
            "first_line_ms": lines[0]["t_ms"] if lines else None,
            "last_line_ms": lines[-1]["t_ms"] if lines else None,
            "banner_seen": any(line["text"].endswith(BANNER) for line in lines),
            "lines": lines,
        }
    )
    record["status"] = "CAPTURED" if payload else "NO_DATA_TIMEOUT"
    write_json(timeline_path, record)
    return record


def _cycle_walls(lines: list[dict], start_marker: str, end_marker: str) -> list[dict]:
    cycles: list[dict] = []
    start_ms: float | None = None
    for line in lines:
        text = line["text"]
        if start_ms is None and (text == start_marker or text == end_marker):
            start_ms = line["t_ms"]
        if text == end_marker:
            cycles.append(
                {
                    "index": len(cycles) + 1,
                    "start_ms": round(start_ms if start_ms is not None else line["t_ms"], 3),
                    "end_ms": line["t_ms"],
                    "wall_ms": round(line["t_ms"] - (start_ms or line["t_ms"]), 3),
                }
            )
            start_ms = line["t_ms"]
    return cycles


def _delay_windows(
    lines: list[dict], from_marker: str, to_marker: str, requested_ms: float
) -> list[dict]:
    windows: list[dict] = []
    pending: dict | None = None
    for line in lines:
        if line["text"] == from_marker:
            pending = line
        elif line["text"] == to_marker and pending is not None:
            wall_ms = line["t_ms"] - pending["t_ms"]
            windows.append(
                {
                    "from_ms": pending["t_ms"],
                    "to_ms": line["t_ms"],
                    "wall_ms": round(wall_ms, 3),
                    "requested_ms": requested_ms,
                    "excess_ms": round(wall_ms - requested_ms, 3),
                }
            )
            pending = None
    return windows


def _largest_gaps(lines: list[dict], limit: int) -> list[dict]:
    gaps: list[dict] = []
    for previous, current in zip(lines, lines[1:]):
        gaps.append(
            {
                "after": previous["text"],
                "before": current["text"],
                "gap_ms": round(current["t_ms"] - previous["t_ms"], 3),
            }
        )
    gaps.sort(key=lambda gap: gap["gap_ms"], reverse=True)
    return gaps[:limit]


def analyze(
    timeline: dict,
    firmware_commit: str,
    requested_delay_ms: float = DEFAULT_REQUESTED_DELAY_MS,
    max_cycle_ms: float = DEFAULT_MAX_CYCLE_MS,
    min_cycles: int = DEFAULT_MIN_CYCLES,
    delay_from: str = DEFAULT_DELAY_FROM,
    delay_to: str = DEFAULT_DELAY_TO,
    top_gaps: int = DEFAULT_TOP_GAPS,
) -> dict:
    lines = timeline.get("lines") or []
    failures: list[str] = []
    if timeline.get("status") != "CAPTURED" or not lines:
        failures.append("capture has no lines to measure")

    stamps = [line["t_ms"] for line in lines]
    if any(later < earlier for earlier, later in zip(stamps, stamps[1:])):
        failures.append("line timestamps are not monotonic")

    identity = IDENTITY_PREFIX + firmware_commit
    identity_count = sum(1 for line in lines if line["text"] == identity)
    if identity_count == 0:
        failures.append("firmware identity marker missing")
    if not any(line["text"].endswith(BANNER) for line in lines):
        failures.append("runtime banner missing")

    cycles = _cycle_walls(lines, CYCLE_START, CYCLE_END)
    if len(cycles) < min_cycles:
        failures.append(f"measured cycles {len(cycles)} < required {min_cycles}")

    windows = _delay_windows(lines, delay_from, delay_to, requested_delay_ms)
    # The first MUTEX_FAST of a run has no preceding delay anchor, so n cycles carry
    # n-1 complete xy_os_delay() windows.
    required_windows = max(1, min_cycles - 1)
    if len(windows) < required_windows:
        failures.append(f"measured delay windows {len(windows)} < required {required_windows}")

    slowest_cycle = max((cycle["wall_ms"] for cycle in cycles), default=0.0)
    worst_excess = max((window["excess_ms"] for window in windows), default=0.0)
    if failures:
        verdict = "INSUFFICIENT_MEASUREMENT"
        status = "REJECTED"
    elif slowest_cycle > max_cycle_ms:
        verdict = "CYCLE_WALL_EXCEEDS_BOUND"
        status = "TIMELINE_MEASURED"
    else:
        verdict = "CYCLE_WALL_WITHIN_BOUND"
        status = "TIMELINE_MEASURED"

    return {
        "record_type": "xinyi-pandora-stm32l475-runtime-timeline",
        "status": status,
        "verdict": verdict,
        "firmware_commit": firmware_commit,
        "device": timeline.get("device"),
        "requested_seconds": timeline.get("requested_seconds"),
        "captured_bytes": timeline.get("captured_bytes"),
        "capture_sha256": timeline.get("capture_sha256"),
        "captured_at_utc": timeline.get("captured_at_utc"),
        "line_count": len(lines),
        "identity_markers": identity_count,
        "banner_markers": sum(1 for line in lines if line["text"].endswith(BANNER)),
        "first_line_ms": stamps[0] if stamps else None,
        "last_line_ms": stamps[-1] if stamps else None,
        "observed_window_ms": round(stamps[-1] - stamps[0], 3) if len(stamps) > 1 else 0.0,
        "cycles": cycles,
        "cycle_count": len(cycles),
        "slowest_cycle_ms": slowest_cycle,
        "max_cycle_ms": max_cycle_ms,
        "delay_windows": windows,
        "delay_window_count": len(windows),
        "worst_delay_excess_ms": worst_excess,
        "requested_delay_ms": requested_delay_ms,
        "largest_gaps": _largest_gaps(lines, top_gaps),
        "failures": failures,
        "interpretation": (
            "delay_window excess_ms is wall time inside a window that asks for a fixed "
            "number of kernel ticks, minus the requested duration. It measures that the "
            "ticks did not arrive on time; it does not by itself attribute the delay to "
            "the logger and is not a latency, jitter or throughput measurement."
        ),
        "scope": (
            "host-side arrival timeline of the Pandora OSAL runtime marker stream: per-cycle "
            "wall time, marker-to-marker gaps, and excess wall time inside known "
            "xy_os_delay() windows"
        ),
        "not_evidence_for": [
            "kernel tick loss attribution (needs firmware-side counters)",
            "interrupt latency, scheduler jitter or throughput",
            "PM system sleep/wake (no charger IC on this board)",
            "FOTA confirm branch, endurance, power-loss or brownout",
            "STM32U5 runtime",
        ],
    }


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=(__doc__ or "").splitlines()[0])
    subparsers = parser.add_subparsers(dest="command", required=True)

    capture_parser = subparsers.add_parser("capture", help="record a timestamped UART timeline")
    capture_parser.add_argument("--device", default="/dev/ttyUSB0")
    capture_parser.add_argument("--seconds", type=float, default=70.0)
    capture_parser.add_argument("--arm-delay", type=float, default=0.3)
    capture_parser.add_argument(
        "--reset-command",
        default=None,
        help='command run after the port is open and flushed, e.g. "st-flash reset"',
    )
    capture_parser.add_argument("--output", type=Path, required=True)
    capture_parser.add_argument("--timeline", type=Path, required=True)

    analyze_parser = subparsers.add_parser("analyze", help="measure a captured timeline")
    analyze_parser.add_argument("--timeline", type=Path, required=True)
    analyze_parser.add_argument("--firmware-commit", required=True)
    analyze_parser.add_argument("--record", type=Path, required=True)
    analyze_parser.add_argument("--requested-delay-ms", type=float, default=DEFAULT_REQUESTED_DELAY_MS)
    analyze_parser.add_argument("--max-cycle-ms", type=float, default=DEFAULT_MAX_CYCLE_MS)
    analyze_parser.add_argument("--min-cycles", type=int, default=DEFAULT_MIN_CYCLES)
    analyze_parser.add_argument("--delay-from", default=DEFAULT_DELAY_FROM)
    analyze_parser.add_argument("--delay-to", default=DEFAULT_DELAY_TO)
    analyze_parser.add_argument("--top-gaps", type=int, default=DEFAULT_TOP_GAPS)

    args = parser.parse_args(argv)

    if args.command == "capture":
        if args.seconds <= 0:
            parser.error("--seconds must be positive")
        record = capture(
            args.device,
            args.seconds,
            args.reset_command,
            args.arm_delay,
            args.output,
            args.timeline,
        )
        print(
            f"{record['status']} bytes={record.get('captured_bytes', 0)} "
            f"lines={record.get('line_count', 0)} device={args.device}"
        )
        if record.get("error"):
            print(f"- {record['error']}")
        return 0 if record["status"] == "CAPTURED" else 2

    try:
        firmware_commit = validate_commit(args.firmware_commit)
    except ValueError as error:
        parser.error(str(error))

    timeline = json.loads(args.timeline.read_text(encoding="utf-8"))
    result = analyze(
        timeline,
        firmware_commit,
        requested_delay_ms=args.requested_delay_ms,
        max_cycle_ms=args.max_cycle_ms,
        min_cycles=args.min_cycles,
        delay_from=args.delay_from,
        delay_to=args.delay_to,
        top_gaps=args.top_gaps,
    )
    result["recorded_at_utc"] = datetime.now(timezone.utc).isoformat()
    write_json(args.record, result)
    print(
        f"{result['status']}/{result['verdict']} cycles={result['cycle_count']} "
        f"slowest_cycle_ms={result['slowest_cycle_ms']} "
        f"worst_delay_excess_ms={result['worst_delay_excess_ms']}"
    )
    for gap in result["largest_gaps"][:3]:
        print(f"- gap {gap['gap_ms']} ms: {gap['after']} -> {gap['before']}")
    for failure in result["failures"]:
        print(f"- {failure}")
    return 0 if result["status"] == "TIMELINE_MEASURED" else 1


if __name__ == "__main__":
    sys.exit(main())
