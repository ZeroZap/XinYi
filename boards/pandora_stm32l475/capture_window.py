#!/usr/bin/env python3
"""Capture a bounded Pandora UART window and write it, unjudged, to a file.

Why this exists
---------------
`validate_ap3216c_hw_i2c_capture.py` and `validate_icm20608_capture.py` only judge an
existing file. The images they judge print their banner immediately after reset, so the
UART has to be open and flushed *before* the reset happens - a capture tool that opens
the port after resetting loses the first lines and the validator then correctly reports
missing markers.

This tool owns exactly that ordering and nothing else: open, flush, reset, read for N
seconds, write the raw bytes plus a metadata record. It makes no judgement about the
content, so the same capture can be judged by whichever validator owns the image. Exit
code is fail-closed about *transport* only:

  0  bytes were captured (content is somebody else's verdict)
  2  no bytes arrived in the window
  3  the device could not be opened

Usage
-----
  boards/pandora_stm32l475/capture_window.py --device /dev/ttyUSB0 --seconds 6 \\
      --reset-command "st-flash reset" --output capture.log --metadata capture.json

Both ST-Link flashing and the reset command are separate: this tool never writes flash.
"""

from __future__ import annotations

import argparse
import datetime
import hashlib
import json
import os
import pathlib
import select
import subprocess
import sys
import termios
import time


def open_serial(device: str, baudrate: int = 115200) -> int:
    """Open `device` raw 8-N-1 with no flow control and return the fd."""
    fd = os.open(device, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
    attributes = termios.tcgetattr(fd)
    speed = getattr(termios, f"B{baudrate}", None)
    if speed is None:
        os.close(fd)
        raise ValueError(f"unsupported baudrate {baudrate}")
    attributes[0] = 0  # iflag: no translation, no software flow control
    attributes[1] = 0  # oflag
    attributes[2] = termios.CS8 | termios.CREAD | termios.CLOCAL  # cflag: 8-N-1, no modem control
    attributes[3] = 0  # lflag: raw, no echo, no canonical mode
    attributes[4] = speed
    attributes[5] = speed
    attributes[6][termios.VMIN] = 0
    attributes[6][termios.VTIME] = 0
    termios.tcsetattr(fd, termios.TCSANOW, attributes)
    termios.tcflush(fd, termios.TCIOFLUSH)
    return fd


def capture_window(
    device: str,
    seconds: float,
    reset_command: list[str] | None,
    baudrate: int = 115200,
    opener=open_serial,
    runner=subprocess.run,
) -> tuple[str, bytes, str | None]:
    """Open and flush the port, then reset and read for `seconds`.

    The reset runs *after* the flush on purpose: a port opened afterwards would miss the
    banner that the validator requires.
    """
    try:
        fd = opener(device, baudrate)
    except OSError as error:
        return "DEVICE_OPEN_FAILED", b"", str(error)
    except ValueError as error:
        return "DEVICE_OPEN_FAILED", b"", str(error)

    payload = bytearray()
    deadline = time.monotonic() + seconds
    reset_error = None
    try:
        if reset_command:
            try:
                runner(reset_command, check=True, capture_output=True)
            except (OSError, subprocess.CalledProcessError) as error:
                reset_error = str(error)
        while time.monotonic() < deadline:
            readable, _, _ = select.select([fd], [], [], 0.05)
            if not readable:
                continue
            try:
                chunk = os.read(fd, 4096)
            except BlockingIOError:
                continue
            except OSError as error:
                return "CAPTURE_IO_FAILED", bytes(payload), str(error)
            if not chunk:
                return "CAPTURE_IO_FAILED", bytes(payload), "device returned EOF"
            payload.extend(chunk)
    finally:
        os.close(fd)

    if reset_error is not None:
        return "RESET_FAILED", bytes(payload), reset_error
    return ("CAPTURED" if payload else "NO_DATA_TIMEOUT"), bytes(payload), None


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=(__doc__ or "").splitlines()[0])
    parser.add_argument("--device", default="/dev/ttyUSB0")
    parser.add_argument("--baudrate", type=int, default=115200)
    parser.add_argument("--seconds", type=float, default=6.0)
    parser.add_argument("--reset-command", default=None,
                        help="command run after the port is open and flushed, e.g. 'st-flash reset'")
    parser.add_argument("--output", type=pathlib.Path, required=True)
    parser.add_argument("--metadata", type=pathlib.Path, required=True)
    args = parser.parse_args(argv)

    reset_command = args.reset_command.split() if args.reset_command else None
    status, payload, error = capture_window(args.device, args.seconds, reset_command,
                                            args.baudrate)
    record = {
        "device": args.device,
        "baudrate": args.baudrate,
        "format": "8-N-1",
        "window_seconds": args.seconds,
        "reset_command": args.reset_command,
        "captured_at_utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
        "bytes_captured": len(payload),
        "sha256": hashlib.sha256(payload).hexdigest(),
        "non_ascii_byte_count": sum(byte > 0x7F for byte in payload),
        "capture_status": status,
        "error": error,
    }
    args.metadata.parent.mkdir(parents=True, exist_ok=True)
    args.metadata.write_text(json.dumps(record, indent=2, sort_keys=True) + "\n",
                             encoding="utf-8")
    if status != "DEVICE_OPEN_FAILED":
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_bytes(payload)

    print(f"{status} device={args.device} bytes={len(payload)} sha256={record['sha256']}")
    if error is not None:
        print(f"ERROR {error}")
    if status == "DEVICE_OPEN_FAILED":
        return 3
    return 0 if status == "CAPTURED" else 2


if __name__ == "__main__":
    raise SystemExit(main())
