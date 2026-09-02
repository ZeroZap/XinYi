#!/usr/bin/env python3
"""Capture a bounded Pandora UART log without granting runtime evidence on timeout."""

import argparse
from datetime import datetime, timezone
import json
import os
from pathlib import Path
import re
import select
import sys
import termios
import time


PANDORA_BANNER = b"PANDORA STM32L475VE XINYI SMOKE OK"
AHT10_ACK = b"AHT10 0x38 ACK"
AHT10_NACK = b"AHT10 0x38 NACK"
FIRMWARE_COMMIT_PREFIX = b"FIRMWARE_COMMIT "
AHT10_MEASUREMENT = re.compile(
    rb"AHT10 RH_milli_percent=([0-9]+) T_milli_c=(-?[0-9]+)(?:\r?\n|$)"
)


def has_plausible_aht10_measurement(payload: bytes) -> bool:
    match = AHT10_MEASUREMENT.search(payload)
    if match is None:
        return False
    humidity_milli_percent = int(match.group(1))
    temperature_milli_c = int(match.group(2))
    return 0 <= humidity_milli_percent <= 100000 and -50000 <= temperature_milli_c <= 150000


def has_ordered_aht10_recovery(payload: bytes) -> bool:
    banner_offset = payload.find(PANDORA_BANNER)
    if banner_offset < 0:
        return False
    banner_end = banner_offset + len(PANDORA_BANNER)
    nack_offset = payload.find(AHT10_NACK, banner_end)
    if nack_offset < 0:
        return False
    for earlier_measurement in AHT10_MEASUREMENT.finditer(payload, banner_end, nack_offset):
        humidity_milli_percent = int(earlier_measurement.group(1))
        temperature_milli_c = int(earlier_measurement.group(2))
        if 0 <= humidity_milli_percent <= 100000 and -50000 <= temperature_milli_c <= 150000:
            return False
    ack_offset = payload.find(AHT10_ACK, nack_offset + len(AHT10_NACK))
    if ack_offset < 0:
        return False
    measurement = AHT10_MEASUREMENT.search(payload, ack_offset + len(AHT10_ACK))
    if measurement is None:
        return False
    humidity_milli_percent = int(measurement.group(1))
    temperature_milli_c = int(measurement.group(2))
    return 0 <= humidity_milli_percent <= 100000 and -50000 <= temperature_milli_c <= 150000


def has_ordered_aht10_startup(payload: bytes) -> bool:
    banner_offset = payload.find(PANDORA_BANNER)
    if banner_offset < 0:
        return False
    ack_offset = payload.find(AHT10_ACK, banner_offset + len(PANDORA_BANNER))
    if ack_offset < 0:
        return False
    measurement = AHT10_MEASUREMENT.search(payload, ack_offset + len(AHT10_ACK))
    if measurement is None:
        return False
    humidity_milli_percent = int(measurement.group(1))
    temperature_milli_c = int(measurement.group(2))
    return 0 <= humidity_milli_percent <= 100000 and -50000 <= temperature_milli_c <= 150000


def has_ordered_firmware_commit(payload: bytes, firmware_commit_marker: bytes) -> bool:
    banner_offset = payload.find(PANDORA_BANNER)
    if banner_offset < 0:
        return False
    marker_offset = payload.find(firmware_commit_marker, banner_offset + len(PANDORA_BANNER))
    if marker_offset < 0:
        return False
    ack_offset = payload.find(AHT10_ACK, marker_offset + len(firmware_commit_marker))
    return ack_offset >= 0


def validate_firmware_commit(revision: str) -> str:
    if re.fullmatch(r"[0-9a-f]{40}", revision) is None:
        raise ValueError("firmware commit must be an exact 40-character lowercase Git SHA")
    return revision


def write_metadata(path: Path, record: dict) -> None:
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


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--device", default="/dev/ttyACM0")
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--metadata", type=Path, required=True)
    parser.add_argument("--timeout", type=float, default=6.0)
    parser.add_argument(
        "--firmware-commit",
        required=True,
        help="Git revision used to build and program the captured firmware",
    )
    args = parser.parse_args()
    if args.timeout <= 0:
        parser.error("--timeout must be positive")

    try:
        firmware_commit = validate_firmware_commit(args.firmware_commit)
    except ValueError as error:
        parser.error(str(error))
    status, payload, error = capture(args.device, args.timeout)
    runtime_evidence = "NONE"
    firmware_commit_marker = FIRMWARE_COMMIT_PREFIX + firmware_commit.encode("ascii")
    firmware_commit_marker_matched = has_ordered_firmware_commit(payload, firmware_commit_marker)
    if status == "CAPTURED":
        missing_markers = []
        if PANDORA_BANNER not in payload:
            missing_markers.append("Pandora banner")
        if AHT10_ACK not in payload:
            missing_markers.append("AHT10 ACK")
        if not has_plausible_aht10_measurement(payload):
            missing_markers.append("plausible AHT10 measurement")
        if not firmware_commit_marker_matched:
            missing_markers.append("ordered firmware commit marker")
        if not has_ordered_aht10_startup(payload):
            missing_markers.append("ordered Pandora/AHT10 startup")
        if missing_markers:
            status = "CAPTURE_CONTENT_MISMATCH"
            error = "required runtime marker(s) not found: " + ", ".join(missing_markers)
        else:
            runtime_evidence = (
                "B2_REVIEW_CANDIDATE"
                if has_ordered_aht10_recovery(payload)
                else "B1_REVIEW_CANDIDATE"
            )
    if status != "DEVICE_OPEN_FAILED":
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_bytes(payload)

    record = {
        "baudrate": 115200,
        "bytes_captured": len(payload),
        "captured_at_utc": datetime.now(timezone.utc).isoformat(),
        "device": args.device,
        "error": error,
        "format": "8-N-1",
        "firmware_commit": firmware_commit,
        "firmware_commit_marker_matched": firmware_commit_marker_matched,
        "required_firmware_commit_marker": firmware_commit_marker.decode("ascii"),
        "required_ack_marker": AHT10_ACK.decode("ascii"),
        "required_nack_marker": AHT10_NACK.decode("ascii"),
        "required_marker": PANDORA_BANNER.decode("ascii"),
        "required_measurement_pattern": AHT10_MEASUREMENT.pattern.decode("ascii"),
        "runtime_evidence": runtime_evidence,
        "status": status,
        "timeout_seconds": args.timeout,
    }
    write_metadata(args.metadata, record)
    print(f"{status} bytes={len(payload)} device={args.device}")

    if status == "CAPTURED":
        return 0
    if status == "NO_DATA_TIMEOUT":
        return 2
    return 3


if __name__ == "__main__":
    sys.exit(main())
