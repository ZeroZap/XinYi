#!/usr/bin/env python3
"""Capture and judge a bounded Pandora AHT10 (software-I2C4) UART window.

Two subcommands:

    capture --device /dev/ttyUSB0 --firmware-commit <sha> --output ... --metadata ...
    analyze --input <capture> --firmware-commit <sha> [--metadata ...]

`capture` opens and flushes the UART *before* resetting the target so the boot
header is not lost, then judges the window it collected. `analyze` judges an
existing capture. Both fail closed: no status other than `AHT10_SOFT_I2C4_B1`
means the window does not prove the AHT10 chain ran at that exact commit.
"""

import argparse
from datetime import datetime, timezone
import json
import os
from pathlib import Path
import re
import select
import subprocess
import termios
import time

SAMPLE = re.compile(r"^AHT10 SAMPLE hum_centi=([0-9]+) temp_centi=(-?[0-9]+)$")
ERROR_MARKERS = ("PANDORA_AHT10_", "HardFault", "ASSERT")
HUMIDITY_MAX_CENTI = 10000
TEMPERATURE_MIN_CENTI = -4000
TEMPERATURE_MAX_CENTI = 8500
MIN_SAMPLES = 10
OK_STATUS = "AHT10_SOFT_I2C4_B1"
FAILED_STATUS = "AHT10_SOFT_I2C4_VALIDATION_FAILED"


def validate_firmware_commit(revision: str) -> str:
    if re.fullmatch(r"[0-9a-f]{40}", revision) is None:
        raise ValueError("firmware commit must be an exact 40-character lowercase Git SHA")
    return revision


def analyze_capture(payload: bytes, firmware_commit: str) -> dict:
    failures: list[str] = []
    if re.fullmatch(r"[0-9a-f]{40}", firmware_commit) is None:
        failures.append("invalid exact firmware commit")

    non_ascii_count = sum(byte > 0x7F for byte in payload)
    text = payload.decode("ascii", errors="replace")
    if non_ascii_count > 4:
        failures.append(f"non-ASCII byte count {non_ascii_count} > 4")

    ordered_markers = (
        "PANDORA AHT10 SENSOR READY",
        f"FIRMWARE_COMMIT {firmware_commit}",
        "AHT10_BUS=SOFT_I2C4 SCL=PD6 SDA=PC1 ADDR=0x38",
        "PANDORA_I2C4_INIT_DONE",
        "I2C4_ACK=0x38",
        "PANDORA_I2C4_SCAN_DONE count=",
        "AHT10_DRIVER_READY ADDR=0x38",
    )
    position = -1
    cycle_start = -1
    for marker in ordered_markers:
        found = text.find(marker, position + 1)
        if found < 0:
            failures.append(f"missing or out-of-order marker: {marker}")
        else:
            if cycle_start < 0:
                cycle_start = found
            position = found

    evidence_text = text[cycle_start:] if cycle_start >= 0 else text
    for marker in ERROR_MARKERS:
        if marker in evidence_text:
            failures.append(f"error marker present: {marker}")

    samples = []
    malformed_samples = 0
    for line in evidence_text.replace("\r", "").splitlines():
        if not line.startswith("AHT10 SAMPLE "):
            continue
        match = SAMPLE.fullmatch(line)
        if match is None:
            malformed_samples += 1
            continue
        humidity_centi, temperature_centi = (int(group) for group in match.groups())
        if humidity_centi > HUMIDITY_MAX_CENTI:
            failures.append("sample outside AHT10 humidity range")
        if temperature_centi < TEMPERATURE_MIN_CENTI or temperature_centi > TEMPERATURE_MAX_CENTI:
            failures.append("sample outside AHT10 temperature range")
        samples.append((humidity_centi, temperature_centi))

    if malformed_samples:
        failures.append(f"malformed sample count {malformed_samples}")
    if len(samples) < MIN_SAMPLES:
        failures.append(f"sample count {len(samples)} < {MIN_SAMPLES}")

    humidities = [humidity for humidity, _ in samples]
    temperatures = [temperature for _, temperature in samples]
    return {
        "status": OK_STATUS if not failures else FAILED_STATUS,
        "firmware_commit": firmware_commit,
        "bus": "SOFT_I2C4 SCL=PD6 SDA=PC1",
        "address_7bit": "0x38",
        "sample_count": len(samples),
        "unique_sample_count": len(set(samples)),
        "humidity_centi_range": [min(humidities), max(humidities)] if humidities else [],
        "temperature_centi_range": [min(temperatures), max(temperatures)] if temperatures else [],
        "non_ascii_byte_count": non_ascii_count,
        "failures": failures,
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


def capture_window(
    device: str, timeout: float, reset_command: list[str] | None = None
) -> tuple[str, bytes, str | None]:
    """Open the UART, optionally reset the target, then collect until it is quiet."""

    try:
        fd = os.open(device, os.O_RDONLY | os.O_NOCTTY | os.O_NONBLOCK)
    except OSError as error:
        return "DEVICE_OPEN_FAILED", b"", str(error)

    payload = bytearray()
    deadline = time.monotonic() + timeout
    try:
        configure_uart(fd)
        if reset_command:
            subprocess.run(reset_command, capture_output=True, check=False)
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
    firmware_commit = validate_firmware_commit(args.firmware_commit)
    reset_command = args.reset_command.split() if args.reset_command else None
    status, payload, error = capture_window(args.device, args.timeout, reset_command)
    record = {
        "device": args.device,
        "baudrate": 115200,
        "format": "8-N-1",
        "timeout_seconds": args.timeout,
        "reset_command": args.reset_command,
        "captured_at_utc": datetime.now(timezone.utc).isoformat(),
        "bytes_captured": len(payload),
        "capture_status": status,
        "error": error,
        "firmware_commit": firmware_commit,
    }
    if status == "DEVICE_OPEN_FAILED":
        record["status"] = "DEVICE_OPEN_FAILED"
        write_json(args.metadata, record)
        print(f"DEVICE_OPEN_FAILED device={args.device} error={error}")
        return 3
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(payload)
    result = analyze_capture(payload, firmware_commit)
    record.update(result)
    write_json(args.metadata, record)
    print(f"{result['status']} capture={status} bytes={len(payload)}")
    for failure in result["failures"]:
        print(f"FAIL {failure}")
    return 0 if result["status"] == OK_STATUS else 1


def command_analyze(args) -> int:
    firmware_commit = validate_firmware_commit(args.firmware_commit)
    try:
        payload = args.input.read_bytes()
    except OSError as error:
        result = {
            "status": FAILED_STATUS,
            "firmware_commit": firmware_commit,
            "sample_count": 0,
            "failures": [f"capture unreadable: {error}"],
        }
    else:
        result = analyze_capture(payload, firmware_commit)
    if args.metadata is not None:
        write_json(args.metadata, result)
    print(result["status"])
    for failure in result["failures"]:
        print(f"FAIL {failure}")
    return 0 if result["status"] == OK_STATUS else 1


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    subparsers = parser.add_subparsers(dest="command", required=True)

    capture_parser = subparsers.add_parser("capture", help="capture a UART window and judge it")
    capture_parser.add_argument("--device", default="/dev/ttyUSB0")
    capture_parser.add_argument("--timeout", type=float, default=6.0)
    capture_parser.add_argument("--firmware-commit", required=True)
    capture_parser.add_argument(
        "--reset-command",
        default="",
        help="command run after the UART is open and flushed (default: no reset)",
    )
    capture_parser.add_argument("--output", type=Path, required=True)
    capture_parser.add_argument("--metadata", type=Path, required=True)
    capture_parser.set_defaults(handler=command_capture)

    analyze_parser = subparsers.add_parser("analyze", help="judge an existing capture file")
    analyze_parser.add_argument("--input", type=Path, required=True)
    analyze_parser.add_argument("--firmware-commit", required=True)
    analyze_parser.add_argument("--metadata", type=Path)
    analyze_parser.set_defaults(handler=command_analyze)

    args = parser.parse_args()
    try:
        return args.handler(args)
    except ValueError as error:
        parser.error(str(error))
    return 2


if __name__ == "__main__":
    raise SystemExit(main())
