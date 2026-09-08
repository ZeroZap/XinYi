#!/usr/bin/env python3
"""Validate an identity-bound Pandora STM32L475 Crypto KAT capture."""

import argparse
import hashlib
import json
from pathlib import Path
import sys

MARKERS = [
    "PANDORA STM32L475VE CRYPTO SOFTWARE READY",
    "CRYPTO_SHA256_KAT_PASS",
    "CRYPTO_HMAC_SHA256_KAT_PASS",
    "CRYPTO_AES128_KAT_PASS",
    "CRYPTO_REPEAT_1000_PASS",
    "B1_CRYPTO_SOFTWARE_KAT_PASS",
]


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
    text = raw.decode("ascii", errors="replace")
    identity = f"FIRMWARE_COMMIT {args.firmware_commit}"
    counts = {marker: text.count(marker) for marker in MARKERS}
    failures = []

    if not raw:
        failures.append("capture is empty")
    if len(args.firmware_commit) != 40 or any(c not in "0123456789abcdef" for c in args.firmware_commit):
        failures.append("firmware commit is not a lowercase 40-hex identity")
    if text.count(identity) != 1:
        failures.append(f"firmware identity count {text.count(identity)} != 1")
    for marker, count in counts.items():
        if count != 1:
            failures.append(f"marker {marker} count {count} != 1")
    positions = [text.find(marker) for marker in MARKERS]
    if all(position >= 0 for position in positions) and positions != sorted(positions):
        failures.append("Crypto markers are not strictly ordered")
    error_lines = [line for line in text.splitlines() if "CRYPTO_" in line and "ERROR" in line]
    if error_lines:
        failures.append(f"error marker count {len(error_lines)} != 0")

    if args.readback_sha256 != args.firmware_sha256:
        failures.append("Flash read-back SHA-256 differs from firmware SHA-256")

    record = {
        "schema": "xinyi-pandora-crypto-runtime-v1",
        "status": "B1_CRYPTO_SOFTWARE_KAT_PASS" if not failures else "REJECTED",
        "board": "Pandora STM32L475VE",
        "core": "Cortex-M4F",
        "implementation": "software",
        "firmware_commit": args.firmware_commit,
        "firmware_sha256": args.firmware_sha256,
        "readback_sha256": args.readback_sha256,
        "readback_byte_identical": args.readback_sha256 == args.firmware_sha256,
        "probe": "ST-Link V2.1",
        "probe_serial": args.probe_serial,
        "capture_sha256": hashlib.sha256(raw).hexdigest(),
        "captured_bytes": len(raw),
        "marker_counts": counts,
        "error_marker_count": len(error_lines),
        "limitations": [
            "Bounded runtime KAT/self-test evidence only",
            "No constant-time, side-channel, performance, entropy, hardware-acceleration, or compliance claim",
            "LWC algorithms are not included in this runtime image",
        ],
        "failures": failures,
    }
    args.record.parent.mkdir(parents=True, exist_ok=True)
    args.record.write_text(json.dumps(record, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(record["status"])
    return 0 if not failures else 1


if __name__ == "__main__":
    sys.exit(main())
