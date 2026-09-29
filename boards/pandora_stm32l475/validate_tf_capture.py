#!/usr/bin/env python3
"""Validate an identity-bound Pandora STM32L475 TF-card read-only capture."""

import argparse
import hashlib
import json
from pathlib import Path
import re
import sys

BANNER = "PANDORA TF SPI1 PROBE"
DONE = "PANDORA_TF_PROBE_DONE"
READY = re.compile(r"^PANDORA_TF_READY type=(SDHC|SDSC) blocks=([0-9]+) bytes=([0-9]+)$", re.MULTILINE)
BLOCK0 = "PANDORA_TF_BLOCK0_OK signature=55AA"


def valid_sha256(value: str) -> bool:
    return len(value) == 64 and all(character in "0123456789abcdef" for character in value)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--capture", required=True, type=Path)
    parser.add_argument("--firmware-commit", required=True)
    parser.add_argument("--firmware-sha256", required=True)
    parser.add_argument("--readback-sha256", required=True)
    parser.add_argument("--probe-serial", required=True)
    parser.add_argument("--record", required=True, type=Path)
    args = parser.parse_args()

    raw = args.capture.read_bytes() if args.capture.exists() else b""
    text = raw.decode("ascii", errors="replace").replace("\r\n", "\n")
    identity = f"FIRMWARE_COMMIT {args.firmware_commit}"
    ready_matches = list(READY.finditer(text))
    failures = []

    if not raw:
        failures.append("capture is empty")
    if len(args.firmware_commit) != 40 or any(
        character not in "0123456789abcdef" for character in args.firmware_commit
    ):
        failures.append("firmware commit is not a lowercase 40-hex identity")
    if not valid_sha256(args.firmware_sha256):
        failures.append("firmware SHA-256 is not lowercase 64-hex")
    if not valid_sha256(args.readback_sha256):
        failures.append("read-back SHA-256 is not lowercase 64-hex")
    if text.count(identity) != 1:
        failures.append(f"firmware identity count {text.count(identity)} != 1")
    if text.count(BANNER) != 1:
        failures.append(f"banner count {text.count(BANNER)} != 1")
    if len(ready_matches) != 1:
        failures.append(f"ready marker count {len(ready_matches)} != 1")
    if text.count(BLOCK0) != 1:
        failures.append(f"block0 marker count {text.count(BLOCK0)} != 1")
    if text.count(DONE) != 1:
        failures.append(f"done marker count {text.count(DONE)} != 1")

    if len(ready_matches) == 1:
        card_type = ready_matches[0].group(1)
        block_count = int(ready_matches[0].group(2))
        capacity_bytes = int(ready_matches[0].group(3))
        if block_count == 0:
            failures.append("block count is zero")
        if capacity_bytes != block_count * 512:
            failures.append("reported capacity does not equal block_count * 512")
    else:
        card_type = None
        block_count = None
        capacity_bytes = None

    positions = [
        text.find(BANNER),
        text.find(identity),
        ready_matches[0].start() if len(ready_matches) == 1 else -1,
        text.find(BLOCK0),
        text.find(DONE),
    ]
    if all(position >= 0 for position in positions) and positions != sorted(positions):
        failures.append("TF-card markers are not strictly ordered")

    error_lines = [line for line in text.splitlines() if "PANDORA_TF_" in line and "ERROR" in line]
    if error_lines:
        failures.append(f"error marker count {len(error_lines)} != 0")
    if args.readback_sha256 != args.firmware_sha256:
        failures.append("Flash read-back SHA-256 differs from firmware SHA-256")

    record = {
        "schema": "xinyi-pandora-tf-read-runtime-v1",
        "status": "B1_PANDORA_TF_READ_PASS" if not failures else "REJECTED",
        "board": "Pandora STM32L475VE",
        "interface": "SPI1 PA5/PA6/PA7, CS PC3",
        "operation": "read-only block 0",
        "firmware_commit": args.firmware_commit,
        "firmware_sha256": args.firmware_sha256,
        "readback_sha256": args.readback_sha256,
        "readback_byte_identical": args.readback_sha256 == args.firmware_sha256,
        "probe": "ST-Link V2.1",
        "probe_serial": args.probe_serial,
        "capture_sha256": hashlib.sha256(raw).hexdigest(),
        "captured_bytes": len(raw),
        "card_type": card_type,
        "block_count": block_count,
        "capacity_bytes": capacity_bytes,
        "block0_signature": "55AA" if text.count(BLOCK0) == 1 else None,
        "error_marker_count": len(error_lines),
        "limitations": [
            "Read-only block-0 runtime evidence for the inserted TF card",
            "No write, write-readback, filesystem, hot-plug, endurance, performance, or power-loss claim",
            "A 55AA signature is reported only as observed bytes and does not prove a valid partition table",
        ],
        "failures": failures,
    }
    args.record.parent.mkdir(parents=True, exist_ok=True)
    args.record.write_text(json.dumps(record, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(record["status"])
    return 0 if not failures else 1


if __name__ == "__main__":
    sys.exit(main())
