#!/usr/bin/env python3
"""Validate a bounded Pandora AP3216C hardware-I2C3 capture."""

import argparse
import json
from pathlib import Path
import re

SAMPLE = re.compile(
    r"^AP3216C_INT=0x[0-9A-F]{2} RAW_HEX=([0-9A-F]{12}) "
    r"ALS_lux=([0-9]+) PS_raw=([0-9]+) IR_raw=([0-9]+)$"
)
ERROR_MARKERS = ("PANDORA_AP3216C_", "HardFault", "ASSERT")


def analyze_capture(payload: bytes, firmware_commit: str) -> dict:
    failures: list[str] = []
    if re.fullmatch(r"[0-9a-f]{40}", firmware_commit) is None:
        failures.append("invalid exact firmware commit")

    non_ascii_count = sum(byte > 0x7F for byte in payload)
    text = payload.decode("ascii", errors="replace")
    if non_ascii_count > 4:
        failures.append(f"non-ASCII byte count {non_ascii_count} > 4")

    ordered_markers = (
        "PANDORA AP3216C SENSOR READY",
        f"FIRMWARE_COMMIT {firmware_commit}",
        "AP3216C_BUS=HW_I2C3 SCL=PC0 SDA=PC1 ADDR=0x1E",
        "AP3216C_HW_I2C_READY",
        "AP3216C_NACK_OBSERVED ADDR=0x7F",
        "AP3216C_NACK_RECOVERED ADDR=0x1E",
        "AP3216C_CONFIG=0x03 MODE=ALS_PS",
        "AP3216C_DIAG ",
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
        if not line.startswith("AP3216C_INT="):
            continue
        match = SAMPLE.fullmatch(line)
        if match is None:
            malformed_samples += 1
            continue
        als, ps, ir = (int(match.group(index)) for index in range(2, 5))
        if als > 22937 or ps > 1023 or ir > 1023:
            failures.append("sample outside AP3216C public range")
        samples.append((als, ps, ir))

    if malformed_samples:
        failures.append(f"malformed sample count {malformed_samples}")
    if len(samples) < 10:
        failures.append(f"sample count {len(samples)} < 10")

    return {
        "status": (
            "AP3216C_HW_I2C3_B1"
            if not failures
            else "AP3216C_HW_I2C3_VALIDATION_FAILED"
        ),
        "firmware_commit": firmware_commit,
        "bus": "HW_I2C3 SCL=PC0 SDA=PC1",
        "address_7bit": "0x1E",
        "sample_count": len(samples),
        "unique_sample_count": len(set(samples)),
        "non_ascii_byte_count": non_ascii_count,
        "failures": failures,
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("capture", type=Path)
    parser.add_argument("--firmware-commit", required=True)
    parser.add_argument("--json", required=True, type=Path)
    args = parser.parse_args()

    try:
        payload = args.capture.read_bytes()
    except OSError as error:
        result = {
            "status": "AP3216C_HW_I2C3_VALIDATION_FAILED",
            "firmware_commit": args.firmware_commit,
            "sample_count": 0,
            "failures": [f"capture unreadable: {error}"],
        }
    else:
        result = analyze_capture(payload, args.firmware_commit)

    args.json.parent.mkdir(parents=True, exist_ok=True)
    args.json.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(result["status"])
    return 0 if result["status"] == "AP3216C_HW_I2C3_B1" else 1


if __name__ == "__main__":
    raise SystemExit(main())
