#!/usr/bin/env python3
"""Capture and validate the Pandora resident-bootloader decision chain.

The resident bootloader prints one marker line per step of
``pandora_fota_boot_flow_run()``. Those markers are the only runtime evidence that
the sequence ran on silicon, so they are validated here instead of being read by
eye: every boot frame is parsed into (install, decision, jump) and checked against
the order the flow owner can actually produce.

Two subcommands matter:

  capture  open the UART before the reset, capture a bounded window, then judge it
  journal  decode the two ``xy_fota_boot`` journal slots from a flash dump

``analyze_capture()`` and ``decode_journal()`` are pure functions so the Host test
can drive every accept/reject branch without a board.
"""

import argparse
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import re
import select
import struct
import sys
import termios
import time

READY = "PANDORA FOTA BOOTLOADER READY"
COMMIT_PREFIX = "BOOTLOADER_COMMIT "
INSTALLED = "PANDORA_BOOT_CANDIDATE_INSTALLED"
ALREADY_INSTALLED = "PANDORA_BOOT_CANDIDATE_ALREADY_INSTALLED"
CONFIRMED = "PANDORA_BOOT_CANDIDATE_CONFIRMED"
ATTEMPT = "PANDORA_BOOT_ATTEMPT_COMMITTED"
ROLLBACK = "PANDORA_BOOT_ROLLBACK_REQUIRED"
JUMP = "PANDORA_BOOT_JUMP_APP"
RESTAGE_AUTHORIZED = "PANDORA_BOOT_RESTAGE_AUTHORIZED"
NO_VALID_IMAGE = "PANDORA_BOOT_NO_VALID_IMAGE"

INSTALL_MARKERS = (INSTALLED, ALREADY_INSTALLED)
JUMPING_DECISIONS = (ATTEMPT, CONFIRMED)
HALTING_DECISIONS = (
    ROLLBACK,
    "PANDORA_BOOT_INSTALL_ERROR",
    "PANDORA_BOOT_ATTEMPT_ERROR",
    "PANDORA_BOOT_CONFIRM_ERROR",
    "PANDORA_BOOT_INIT_ERROR",
    "PANDORA_BOOT_RESTAGE_AUTHORIZATION_ERROR",
    NO_VALID_IMAGE,
)
DECISIONS = JUMPING_DECISIONS + HALTING_DECISIONS
ERROR_MARKERS = tuple(marker for marker in HALTING_DECISIONS if marker.endswith("_ERROR"))

# Decisions that the flow only reaches after install_once() ran.
INSTALL_BEFORE_DECISION = JUMPING_DECISIONS + (
    ROLLBACK,
    "PANDORA_BOOT_ATTEMPT_ERROR",
    "PANDORA_BOOT_CONFIRM_ERROR",
)

EXPECT_CHOICES = ("auto", "attempt", "confirm", "rollback", "install_error")

# A line that starts with one of these is claimed by the bootloader, so an unknown one
# is a real failure rather than application output. The application banner also starts
# with "PANDORA", so the prefix has to be narrower than that.
MARKER_PREFIXES = ("PANDORA_BOOT_", "PANDORA FOTA BOOTLOADER", "BOOTLOADER_")

JOURNAL_MAGIC = 0x58424A52
JOURNAL_COMMIT = 0x434F4D54434F4D54
JOURNAL_STATES = {
    1: "INSTALLING",
    2: "INSTALLED",
    3: "CONFIRMED",
    4: "ROLLED_BACK",
    5: "RESTAGE_AUTHORIZED",
}
JOURNAL_RECORD_FORMAT = "<IIIIIIIIQ"
JOURNAL_RECORD_SIZE = 40
JOURNAL_CRC_SPAN = 24


def validate_firmware_commit(revision: str) -> str:
    if re.fullmatch(r"[0-9a-f]{40}", revision) is None:
        raise ValueError("firmware commit must be an exact 40-character lowercase Git SHA")
    return revision


def _frames(
    lines: list[str], firmware_commit: str
) -> tuple[list[dict], list[str], list[str]]:
    """Split marker lines into boot frames and collect structural failures.

    Anything that is not a bootloader marker is application output: the capture is
    armed while the previous firmware is still running, and the image the bootloader
    jumps into keeps printing afterwards. Foreign output is therefore allowed outside
    a boot frame (and counted), but never between a frame's markers.
    """
    failures: list[str] = []
    frames: list[dict] = []
    foreign: list[str] = []
    current: dict | None = None
    commit_marker = COMMIT_PREFIX + firmware_commit

    for position, line in enumerate(lines):
        if line == READY:
            if current is not None:
                if current["decision"] is None:
                    failures.append(f"boot frame {len(frames)} ended without a decision marker")
                frames.append(current)
            current = {
                "start_line": position,
                "install": None,
                "decision": None,
                "jump": False,
                "restage": False,
            }
            continue
        if current is None:
            if line.startswith(MARKER_PREFIXES):
                failures.append(f"unknown boot marker {line!r}")
            else:
                foreign.append(line)
            continue
        if line == commit_marker:
            if current["start_line"] != position - 1:
                failures.append(f"boot frame {len(frames)}: commit marker is not the first line")
            current["commit"] = firmware_commit
        elif line == RESTAGE_AUTHORIZED:
            if current["restage"]:
                failures.append(f"boot frame {len(frames)}: duplicate restage marker")
            current["restage"] = True
        elif line in INSTALL_MARKERS:
            if current["install"] is not None:
                failures.append(f"boot frame {len(frames)}: duplicate install marker")
            elif current["decision"] is not None:
                failures.append(f"boot frame {len(frames)}: install marker after the decision")
            current["install"] = line
        elif line in DECISIONS:
            if current["decision"] is not None:
                failures.append(f"boot frame {len(frames)}: duplicate decision marker {line!r}")
            elif "commit" not in current:
                failures.append(f"boot frame {len(frames)}: decision before the commit marker")
            current["decision"] = line
        elif line == JUMP:
            if current["decision"] is None:
                failures.append(f"boot frame {len(frames)}: jump without a decision marker")
            elif current["jump"]:
                failures.append(f"boot frame {len(frames)}: duplicate jump marker")
            elif current["decision"] not in JUMPING_DECISIONS:
                failures.append(
                    f"boot frame {len(frames)}: jump after halting decision "
                    f"{current['decision']!r}"
                )
            current["jump"] = True
        elif line.startswith(MARKER_PREFIXES):
            failures.append(f"unknown boot marker {line!r}")
        elif current["decision"] is not None and current["jump"]:
            # The application the bootloader handed off to is running again.
            current["ended"] = True
            foreign.append(line)
        else:
            failures.append(f"boot frame {len(frames)}: foreign line inside the boot frame: {line!r}")

    if current is not None:
        frames.append(current)

    for index, frame in enumerate(frames):
        if frame.get("commit") != firmware_commit:
            failures.append(f"boot frame {index}: identity marker missing or mismatched")
        if frame["decision"] is None:
            failures.append(f"boot frame {index}: no decision marker")
            continue
        if frame["decision"] in INSTALL_BEFORE_DECISION and frame["install"] is None:
            failures.append(f"boot frame {index}: {frame['decision']!r} without an install marker")
        if frame["decision"] in JUMPING_DECISIONS and not frame["jump"]:
            failures.append(f"boot frame {index}: {frame['decision']!r} without a jump marker")
        if frame["decision"] in HALTING_DECISIONS and frame["jump"]:
            failures.append(f"boot frame {index}: jump after halting decision")

    return frames, failures, foreign


def _terminal(frames: list[dict]) -> str:
    if not frames:
        return "none"
    last = frames[-1]
    if last["decision"] in JUMPING_DECISIONS:
        return "jump"
    if last["decision"] == ROLLBACK:
        return "halt_rollback"
    if last["decision"] == NO_VALID_IMAGE:
        return "halt_no_image"
    if last["decision"] in ERROR_MARKERS or last["decision"] is not None:
        return "halt_error"
    return "none"


def analyze_capture(payload: bytes, firmware_commit: str, expect: str = "auto") -> dict:
    firmware_commit = validate_firmware_commit(firmware_commit)
    if expect not in EXPECT_CHOICES:
        raise ValueError(f"expect must be one of {EXPECT_CHOICES}")
    text = payload.decode("ascii", errors="replace")
    lines = [line.strip() for line in text.splitlines() if line.strip()]
    frames, failures, foreign = _frames(lines, firmware_commit)

    attempts = sum(1 for frame in frames if frame["decision"] == ATTEMPT)
    confirms = sum(1 for frame in frames if frame["decision"] == CONFIRMED)
    terminal = _terminal(frames)

    expect_matched = True
    if expect == "attempt":
        expect_matched = bool(frames) and all(
            frame["decision"] in JUMPING_DECISIONS for frame in frames
        )
    elif expect == "confirm":
        expect_matched = bool(frames) and frames[-1]["decision"] == CONFIRMED
    elif expect == "rollback":
        expect_matched = bool(frames) and frames[-1]["decision"] == ROLLBACK
    elif expect == "install_error":
        expect_matched = bool(frames) and frames[-1]["decision"] == "PANDORA_BOOT_INSTALL_ERROR"
    if not expect_matched:
        failures.append(f"chain does not match the expected outcome {expect!r}")
    if not frames:
        failures.append("no boot frame captured")

    accepted = not failures
    if accepted and attempts and terminal == "jump":
        chain = "ATTEMPT"
    elif accepted and confirms:
        chain = "CONFIRM"
    elif accepted and terminal == "halt_rollback":
        chain = "ROLLBACK"
    elif accepted and terminal == "halt_no_image":
        chain = "NO_IMAGE"
    elif accepted and terminal == "halt_error":
        chain = "HALT_ERROR"
    else:
        chain = "INVALID"

    return {
        "status": "BOOTLOADER_CHAIN_REVIEW_CANDIDATE"
        if accepted
        else "BOOTLOADER_CHAIN_VALIDATION_FAILED",
        "firmware_commit": firmware_commit,
        "expect": expect,
        "expect_matched": expect_matched,
        "chain": chain,
        "terminal": terminal,
        "boot_count": len(frames),
        "attempt_markers": attempts,
        "confirm_markers": confirms,
        "install_markers": [frame["install"] for frame in frames],
        "failures": failures,
        "application_lines": len(foreign),
        "application_line_sample": foreign[:3],
        "frames": [
            {
                "restage": frame["restage"],
                "install": frame["install"],
                "decision": frame["decision"],
                "jump": frame["jump"],
            }
            for frame in frames
        ],
        "capture_bytes": len(payload),
        "capture_sha256": hashlib.sha256(payload).hexdigest(),
        "scope": (
            "resident bootloader decision sequence as printed over UART: candidate "
            "install step, confirm/attempt/rollback decision, and vector handoff marker"
        ),
        "not_evidence_for": [
            "power-loss or brownout behaviour",
            "journal flash endurance",
            "runtime behaviour of the application the bootloader jumps into",
            "cryptographic or signature-based FOTA",
        ],
    }


def _crc32(data: bytes) -> int:
    crc = 0xFFFFFFFF
    for byte in data:
        crc ^= byte
        for _ in range(8):
            crc = (crc >> 1) ^ (0xEDB88320 if crc & 1 else 0)
    return crc ^ 0xFFFFFFFF


def _decode_record(blob: bytes) -> dict:
    if len(blob) < JOURNAL_RECORD_SIZE:
        return {"present": False, "blank": all(byte == 0xFF for byte in blob), "valid": False}
    values = struct.unpack(JOURNAL_RECORD_FORMAT, blob[:JOURNAL_RECORD_SIZE])
    (
        magic,
        generation,
        state,
        image_version,
        image_size,
        image_crc32,
        record_crc32,
        boot_attempts,
        commit,
    ) = values
    stored = _crc32(blob[:JOURNAL_CRC_SPAN])
    valid = magic == JOURNAL_MAGIC and commit == JOURNAL_COMMIT and stored == record_crc32
    return {
        "present": magic != 0xFFFFFFFF or commit != 0xFFFFFFFFFFFFFFFF,
        "blank": all(byte == 0xFF for byte in blob),
        "valid": valid,
        "magic": f"0x{magic:08x}",
        "generation": generation,
        "state": JOURNAL_STATES.get(state, f"UNKNOWN({state})"),
        "state_value": state,
        "image_version": image_version,
        "image_size": image_size,
        "image_crc32": f"0x{image_crc32:08x}",
        "record_crc32_ok": stored == record_crc32,
        "boot_attempts": boot_attempts,
    }


def decode_journal(payload: bytes, slot_size: int = 2048) -> dict:
    if slot_size < JOURNAL_RECORD_SIZE:
        raise ValueError("slot size must fit one journal record")
    if len(payload) < slot_size:
        raise ValueError("journal dump is shorter than one slot")
    frames = [
        _decode_record(payload[index * slot_size : index * slot_size + JOURNAL_RECORD_SIZE])
        for index in range(2)
    ]
    valid = [frame["valid"] for frame in frames]
    failures: list[str] = []
    active = None
    if not any(valid):
        failures.append("neither journal slot holds a valid record")
    else:
        if all(valid):
            delta = (frames[1]["generation"] - frames[0]["generation"]) & 0xFFFFFFFF
            if delta == 0:
                failures.append("both journal slots are valid with the same generation")
                active = 0
            else:
                active = 1 if delta < 0x80000000 else 0
        else:
            active = 1 if valid[1] else 0
    return {
        "status": "JOURNAL_VALID" if not failures else "JOURNAL_INVALID",
        "slot_size": slot_size,
        "slots": frames,
        "active_slot": active,
        "active": frames[active] if active is not None else None,
        "failures": failures,
        "dump_bytes": len(payload),
        "dump_sha256": hashlib.sha256(payload).hexdigest(),
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
    status, payload, error = capture(args.device, args.timeout)
    if status == "DEVICE_OPEN_FAILED":
        record = {"status": status, "device": args.device, "error": error}
        write_json(args.metadata, record)
        print(f"{status} device={args.device}")
        return 3
    if status == "CAPTURE_IO_FAILED":
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_bytes(payload)
        record = {
            "status": status,
            "device": args.device,
            "error": error,
            "capture_bytes": len(payload),
            "capture_sha256": hashlib.sha256(payload).hexdigest(),
        }
        write_json(args.metadata, record)
        print(f"{status} bytes={len(payload)} device={args.device}")
        return 3

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(payload)
    if status == "NO_DATA_TIMEOUT":
        record = {
            "status": status,
            "device": args.device,
            "timeout_seconds": args.timeout,
            "capture_bytes": 0,
            "capture_sha256": hashlib.sha256(b"").hexdigest(),
        }
        write_json(args.metadata, record)
        print(f"{status} bytes=0 device={args.device}")
        return 2

    record = analyze_capture(payload, firmware_commit, args.expect)
    record.update(
        {
            "device": args.device,
            "baudrate": 115200,
            "format": "8-N-1",
            "timeout_seconds": args.timeout,
            "captured_at_utc": datetime.now(timezone.utc).isoformat(),
        }
    )
    write_json(args.metadata, record)
    print(
        f"{record['status']} chain={record['chain']} terminal={record['terminal']} "
        f"boots={record['boot_count']} attempts={record['attempt_markers']} "
        f"bytes={record['capture_bytes']}"
    )
    for failure in record["failures"]:
        print(f"- {failure}")
    return 0 if record["status"] == "BOOTLOADER_CHAIN_REVIEW_CANDIDATE" else 1


def command_analyze(args) -> int:
    firmware_commit = validate_firmware_commit(args.firmware_commit)
    record = analyze_capture(args.input.read_bytes(), firmware_commit, args.expect)
    if args.metadata is not None:
        write_json(args.metadata, record)
    print(
        f"{record['status']} chain={record['chain']} terminal={record['terminal']} "
        f"boots={record['boot_count']} attempts={record['attempt_markers']} "
        f"bytes={record['capture_bytes']}"
    )
    for failure in record["failures"]:
        print(f"- {failure}")
    return 0 if record["status"] == "BOOTLOADER_CHAIN_REVIEW_CANDIDATE" else 1


def command_journal(args) -> int:
    record = decode_journal(args.input.read_bytes(), args.slot_size)
    if args.metadata is not None:
        write_json(args.metadata, record)
    active = record["active"]
    if active is None:
        print(f"{record['status']} active=none")
    else:
        print(
            f"{record['status']} active_slot={record['active_slot']} "
            f"state={active['state']} generation={active['generation']} "
            f"attempts={active['boot_attempts']} record_crc_ok={active['record_crc32_ok']}"
        )
    for failure in record["failures"]:
        print(f"- {failure}")
    return 0 if record["status"] == "JOURNAL_VALID" else 1


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    subparsers = parser.add_subparsers(dest="command", required=True)

    capture_parser = subparsers.add_parser("capture", help="capture a UART window and judge it")
    capture_parser.add_argument("--device", default="/dev/ttyUSB0")
    capture_parser.add_argument("--timeout", type=float, default=8.0)
    capture_parser.add_argument("--firmware-commit", required=True)
    capture_parser.add_argument("--expect", choices=EXPECT_CHOICES, default="auto")
    capture_parser.add_argument("--output", type=Path, required=True)
    capture_parser.add_argument("--metadata", type=Path, required=True)
    capture_parser.set_defaults(func=command_capture)

    analyze_parser = subparsers.add_parser("analyze", help="judge an existing capture file")
    analyze_parser.add_argument("--input", type=Path, required=True)
    analyze_parser.add_argument("--firmware-commit", required=True)
    analyze_parser.add_argument("--expect", choices=EXPECT_CHOICES, default="auto")
    analyze_parser.add_argument("--metadata", type=Path)
    analyze_parser.set_defaults(func=command_analyze)

    journal_parser = subparsers.add_parser("journal", help="decode a journal flash dump")
    journal_parser.add_argument("--input", type=Path, required=True)
    journal_parser.add_argument("--slot-size", type=int, default=2048)
    journal_parser.add_argument("--metadata", type=Path)
    journal_parser.set_defaults(func=command_journal)

    args = parser.parse_args()
    return args.func(args)


if __name__ == "__main__":
    sys.exit(main())
