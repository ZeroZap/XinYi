#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
import select
import termios
import time
from pathlib import Path


DEFAULT_MARKERS = (
    "XINYI OPENCH CH32V307 UART1 READY",
    "{commit}",
    "PA9=UART1_TX PA10=UART1_RX WCHLINK=CH549F",
    "OPENCH_UART1_ALIVE",
)


def classify_capture(
    data: bytes, firmware_commit: str, expected: tuple[str, ...] = ()
) -> tuple[str, list[str]]:
    text = data.decode("ascii", errors="replace")
    templates = expected or DEFAULT_MARKERS
    # "{commit}" keeps the provenance check available to images that do print the
    # commit; images whose marker vocabulary has no commit line must pass their
    # own ordered markers and rely on the programmer's image SHA256 for
    # provenance.
    markers = [template.replace("{commit}", f"FIRMWARE_COMMIT {firmware_commit}") for template in templates]
    if not data:
        return "NO_DATA", markers
    search_from = 0
    while True:
        cycle_start = text.find(markers[0], search_from)
        if cycle_start < 0:
            return "CONTENT_MISMATCH", markers
        position = cycle_start
        for marker in markers[1:]:
            position = text.find(marker, position + 1)
            if position < 0:
                break
        else:
            return "B1_REVIEW_CANDIDATE", markers
        search_from = cycle_start + 1


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("port")
    parser.add_argument("--firmware-commit", required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--metadata", type=Path, required=True)
    parser.add_argument("--duration-ms", type=int, default=3000)
    parser.add_argument(
        "--expect",
        action="append",
        metavar="MARKER",
        help=(
            "expected marker, repeatable and ordered; replaces the UART1 smoke set. "
            "Passing the same marker twice requires it to repeat, which distinguishes "
            "a live sampling loop from a single boot line. '{commit}' expands to the "
            "FIRMWARE_COMMIT line for images that print it"
        ),
    )
    args = parser.parse_args()
    if len(args.firmware_commit) != 40 or any(c not in "0123456789abcdef" for c in args.firmware_commit):
        parser.error("--firmware-commit must be exactly 40 lowercase hexadecimal characters")
    with open(args.port, "rb", buffering=0) as stream:
        fd = stream.fileno()
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
        end = time.monotonic() + args.duration_ms / 1000.0
        data = bytearray()
        while time.monotonic() < end:
            ready, _, _ = select.select([stream], [], [], end - time.monotonic())
            if ready:
                data.extend(stream.read(4096))
    payload = bytes(data)
    status, markers = classify_capture(payload, args.firmware_commit, tuple(args.expect or ()))
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.metadata.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(payload)
    record = {
        "status": status,
        "device": args.port,
        "firmware_commit": args.firmware_commit,
        "bytes_captured": len(payload),
        "capture_sha256": hashlib.sha256(payload).hexdigest(),
        "required_markers": markers,
    }
    args.metadata.write_text(json.dumps(record, indent=2) + "\n", encoding="utf-8")
    print(f"bytes={len(payload)} status={status}")
    return 0 if status == "B1_REVIEW_CANDIDATE" else 1


if __name__ == "__main__":
    raise SystemExit(main())
