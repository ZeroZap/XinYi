#!/usr/bin/env python3
"""Capture and validate the Pandora W25Q128 candidate-programmer run.

``fota_candidate_programmer_main.c`` prints four ordered lines when it stores a
release candidate into the on-board W25Q128: the banner, the commit of the image
doing the programming, the commit the envelope was built from, and a single
verification marker once every 256-byte chunk read back equal. That marker
stream is the only runtime evidence that a candidate actually reached the flash
part, and until now nothing in the repository judged it -- the resident
bootloader validator watches a different stream, so a programmer that printed
PROGRAM_ERROR still produced a "looked fine by eye" log.

Contract enforced here, fail-closed:

  * at least one READY frame exists and the LAST frame is complete;
  * inside a frame the order is READY -> PROGRAMMER_COMMIT -> CANDIDATE_SOURCE_COMMIT
    -> PROGRAMMED_VERIFIED;
  * PROGRAMMER_COMMIT matches --firmware-commit and CANDIDATE_SOURCE_COMMIT matches
    --candidate-source-commit, so the capture cannot be re-attributed;
  * any PROGRAM_ERROR / VERIFY_ERROR marker, any unknown
    ``PANDORA_FOTA_CANDIDATE_*`` marker, or any foreign line inside a frame fails;
  * a truncated final frame fails: an interrupted run may still print the banner.

Subcommands:

  capture  open the UART before the reset, capture a bounded window, then judge it
  analyze  judge an existing capture file

``analyze_capture()`` is a pure function so the Host test can drive every
accept/reject branch without a board.
"""

import argparse
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import re
import select
import sys
import termios
import time

READY = "PANDORA FOTA CANDIDATE PROGRAMMER READY"
COMMIT_PREFIX = "PROGRAMMER_COMMIT "
SOURCE_COMMIT_PREFIX = "CANDIDATE_SOURCE_COMMIT "
VERIFIED = "PANDORA_FOTA_CANDIDATE_PROGRAMMED_VERIFIED"
ERROR_MARKERS = (
    "PANDORA_FOTA_CANDIDATE_PROGRAM_ERROR",
    "PANDORA_FOTA_CANDIDATE_VERIFY_ERROR",
)
IDENTITY_PREFIXES = (COMMIT_PREFIX, SOURCE_COMMIT_PREFIX)
KNOWN_PREFIX = "PANDORA_FOTA_CANDIDATE_"
COMMIT_RE = re.compile(r"^[0-9a-f]{40}$")


def validate_firmware_commit(value: str) -> str:
    if COMMIT_RE.fullmatch(value) is None:
        raise ValueError("--firmware-commit must be an exact lowercase 40-character Git SHA")
    return value


def validate_candidate_source_commit(value: str) -> str:
    if COMMIT_RE.fullmatch(value) is None:
        raise ValueError(
            "--candidate-source-commit must be an exact lowercase 40-character Git SHA"
        )
    return value


def _split_lines(payload: bytes) -> list[str]:
    text = payload.replace(b"\x00", b"").decode("utf-8", errors="replace")
    return [line.strip() for line in re.split(r"[\r\n]+", text)]


def _classify(line: str) -> tuple[str, str]:
    if line == READY:
        return "ready", ""
    if line == VERIFIED:
        return "verified", ""
    for marker in ERROR_MARKERS:
        if line == marker:
            return "error", marker
    for prefix in IDENTITY_PREFIXES:
        if line.startswith(prefix):
            value = line[len(prefix) :].strip()
            kind = "programmer_commit" if prefix is COMMIT_PREFIX else "candidate_commit"
            return kind, value
    if line.startswith(KNOWN_PREFIX):
        return "unknown", line
    return "foreign", line


def analyze_capture(payload: bytes, firmware_commit: str, candidate_source_commit: str) -> dict:
    """Judge one programmer capture against the acceptance contract."""

    firmware_commit = validate_firmware_commit(firmware_commit)
    candidate_source_commit = validate_candidate_source_commit(candidate_source_commit)
    failures: list[str] = []
    frames: list[dict] = []
    foreign: list[str] = []
    current: dict | None = None

    def start_frame() -> dict:
        frame = {
            "programmer_commit": None,
            "candidate_commit": None,
            "verified": False,
            "errors": [],
            "unknowns": [],
            "foreign": [],
        }
        frames.append(frame)
        return frame

    for line in _split_lines(payload):
        if not line:
            continue
        kind, value = _classify(line)
        if kind == "ready":
            # A new banner always starts a new frame; a repeated banner means the
            # previous attempt was cut short (reset) and cannot count as complete.
            current = start_frame()
            continue
        if current is None:
            foreign.append(line)
            continue
        if kind == "verified":
            current["verified"] = True
        elif kind == "error":
            current["errors"].append(value)
        elif kind == "programmer_commit":
            current["programmer_commit"] = value
        elif kind == "candidate_commit":
            current["candidate_commit"] = value
        elif kind == "unknown":
            current["unknowns"].append(value)
        else:
            current["foreign"].append(line)
        if current["errors"]:
            failures.append(f"programmer reported {current['errors'][-1]}")
        if current["unknowns"]:
            failures.append(f"unknown programmer marker {current['unknowns'][-1]}")

    if not frames:
        failures.append("no programmer banner in the capture")

    for index, frame in enumerate(frames):
        last = index == len(frames) - 1
        if frame["programmer_commit"] is None or frame["candidate_commit"] is None:
            if last:
                failures.append("final frame is missing an identity line")
            continue
        if frame["programmer_commit"] != firmware_commit:
            failures.append(
                f"frame {index}: PROGRAMMER_COMMIT {frame['programmer_commit']} != {firmware_commit}"
            )
        if frame["candidate_commit"] != candidate_source_commit:
            failures.append(
                f"frame {index}: CANDIDATE_SOURCE_COMMIT {frame['candidate_commit']} "
                f"!= {candidate_source_commit}"
            )
        if frame["foreign"]:
            failures.append(f"frame {index}: foreign output inside the programmer frame")
        if last and not frame["verified"]:
            failures.append("final frame never printed the verification marker")

    accepted = not failures
    return {
        "status": "PROGRAMMER_CAPTURE_REVIEW_CANDIDATE"
        if accepted
        else "PROGRAMMER_CAPTURE_VALIDATION_FAILED",
        "firmware_commit": firmware_commit,
        "candidate_source_commit": candidate_source_commit,
        "frame_count": len(frames),
        "complete_frames": sum(
            1
            for frame in frames
            if frame["verified"] and frame["programmer_commit"] and frame["candidate_commit"]
        ),
        "verified_markers": sum(1 for frame in frames if frame["verified"]),
        "partial_frames": sum(1 for frame in frames if not frame["verified"]),
        "error_markers": [
            marker for frame in frames for marker in frame["errors"]
        ],
        "foreign_lines": len(foreign),
        "foreign_line_sample": foreign[:3],
        "failures": failures,
        "capture_bytes": len(payload),
        "capture_sha256": hashlib.sha256(payload).hexdigest(),
        "scope": (
            "W25Q128 candidate programmer marker stream: envelope identity plus the "
            "per-chunk read-back verification marker"
        ),
        "not_evidence_for": [
            "the resident bootloader installing or confirming the candidate",
            "W25Q128 erase/program endurance or power-loss behaviour",
            "that the programmed candidate executes",
        ],
    }


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


def capture(device: str, timeout: float) -> tuple[str, bytes, str | None]:
    try:
        fd = os.open(device, os.O_RDONLY | os.O_NOCTTY | os.O_NONBLOCK)
    except OSError as error:
        return "DEVICE_OPEN_FAILED", b"", str(error)

    payload = bytearray()
    deadline = time.monotonic() + timeout
    try:
        configure_uart(fd)
        while True:
            remaining = deadline - time.monotonic()
            if remaining <= 0:
                break
            readable, _, _ = select.select([fd], [], [], remaining)
            if not readable:
                break
            try:
                chunk = os.read(fd, 4096)
            except BlockingIOError:
                continue
            if not chunk:
                return "CAPTURE_IO_FAILED", bytes(payload), "device returned EOF"
            payload.extend(chunk)
    except OSError as error:
        return "CAPTURE_IO_FAILED", bytes(payload), str(error)
    finally:
        os.close(fd)

    return ("CAPTURED" if payload else "NO_DATA_TIMEOUT"), bytes(payload), None


def write_json(path: Path, record: dict) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(record, indent=2, sort_keys=True) + "\n", encoding="utf-8")


def command_capture(args) -> int:
    if args.timeout <= 0:
        raise ValueError("--timeout must be positive")
    firmware_commit = validate_firmware_commit(args.firmware_commit)
    candidate_source_commit = validate_candidate_source_commit(args.candidate_source_commit)
    status, payload, error = capture(args.device, args.timeout)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(payload)
    if status == "DEVICE_OPEN_FAILED":
        write_json(args.metadata, {"status": status, "device": args.device, "error": error})
        print(f"{status} device={args.device}")
        return 3
    if status == "CAPTURE_IO_FAILED":
        write_json(
            args.metadata,
            {
                "status": status,
                "device": args.device,
                "error": error,
                "capture_bytes": len(payload),
                "capture_sha256": hashlib.sha256(payload).hexdigest(),
            },
        )
        print(f"{status} bytes={len(payload)} device={args.device}")
        return 3
    if status == "NO_DATA_TIMEOUT":
        write_json(
            args.metadata,
            {
                "status": status,
                "device": args.device,
                "timeout_seconds": args.timeout,
                "capture_bytes": 0,
                "capture_sha256": hashlib.sha256(b"").hexdigest(),
            },
        )
        print(f"{status} bytes=0 device={args.device}")
        return 2

    record = analyze_capture(payload, firmware_commit, candidate_source_commit)
    record.update(
        {
            "device": args.device,
            "baudrate": 115200,
            "format": "8-N-1",
            "timeout_seconds": args.timeout,
            "captured_at_utc": datetime.now(timezone.utc).isoformat(),
            "capture_file": str(args.output),
        }
    )
    write_json(args.metadata, record)
    print(
        f"{record['status']} frames={record['frame_count']} "
        f"complete={record['complete_frames']} bytes={record['capture_bytes']}"
    )
    for failure in record["failures"]:
        print(f"failure: {failure}")
    return 0 if record["status"].endswith("REVIEW_CANDIDATE") else 1


def command_analyze(args) -> int:
    firmware_commit = validate_firmware_commit(args.firmware_commit)
    candidate_source_commit = validate_candidate_source_commit(args.candidate_source_commit)
    payload = args.input.read_bytes()
    record = analyze_capture(payload, firmware_commit, candidate_source_commit)
    if args.metadata is not None:
        write_json(args.metadata, record)
    print(
        f"{record['status']} frames={record['frame_count']} "
        f"complete={record['complete_frames']} bytes={record['capture_bytes']}"
    )
    for failure in record["failures"]:
        print(f"failure: {failure}")
    return 0 if record["status"].endswith("REVIEW_CANDIDATE") else 1


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    subparsers = parser.add_subparsers(dest="command", required=True)

    capture_parser = subparsers.add_parser("capture", help="capture a UART window and judge it")
    capture_parser.add_argument("--device", default="/dev/ttyUSB0")
    capture_parser.add_argument("--timeout", type=float, default=8.0)
    capture_parser.add_argument("--firmware-commit", required=True)
    capture_parser.add_argument("--candidate-source-commit", required=True)
    capture_parser.add_argument("--output", type=Path, required=True)
    capture_parser.add_argument("--metadata", type=Path, required=True)
    capture_parser.set_defaults(func=command_capture)

    analyze_parser = subparsers.add_parser("analyze", help="judge an existing capture file")
    analyze_parser.add_argument("--input", type=Path, required=True)
    analyze_parser.add_argument("--firmware-commit", required=True)
    analyze_parser.add_argument("--candidate-source-commit", required=True)
    analyze_parser.add_argument("--metadata", type=Path)
    analyze_parser.set_defaults(func=command_analyze)

    args = parser.parse_args()
    return args.func(args)


if __name__ == "__main__":
    sys.exit(main())
