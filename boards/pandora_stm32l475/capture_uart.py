#!/usr/bin/env python3
"""Capture a bounded Pandora UART log without granting runtime evidence on timeout."""

import argparse
from datetime import datetime, timezone
import json
import os
from pathlib import Path
import select
import subprocess
import sys
import termios
import time


def source_commit(root: Path) -> str:
    return subprocess.run(
        ["git", "rev-parse", "HEAD"],
        cwd=root,
        check=True,
        capture_output=True,
        text=True,
    ).stdout.strip()


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
            if chunk:
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
    args = parser.parse_args()
    if args.timeout <= 0:
        parser.error("--timeout must be positive")

    root = Path(__file__).resolve().parents[2]
    status, payload, error = capture(args.device, args.timeout)
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
        "source_commit": source_commit(root),
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
