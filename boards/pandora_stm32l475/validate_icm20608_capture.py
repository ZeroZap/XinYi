#!/usr/bin/env python3
"""Validate a bounded Pandora ICM20608 capture taken over the CP2102 UART.

The image prints a one-shot identity block and then a repeating sample block, so
a capture is judged on three things: the ordered identity markers are present and
in order, the identity the driver accepted is really 0xAE, and every sample line
decodes into numbers that a board sitting still can produce. A capture that only
proves the markers exist is not evidence that the sensor answered.
"""

from __future__ import annotations

import argparse
import json
import re
from pathlib import Path

WHO_AM_I = 0xAE
SAMPLE_COUNT_MIN = 3
# A board at rest measures one g; allow for mounting angle, offset and the
# sensor's own zero-g tolerance without accepting a dead or saturated channel.
ACCEL_MAGNITUDE_MIN_MG = 700.0
ACCEL_MAGNITUDE_MAX_MG = 1300.0
GYRO_LIMIT_DPS = 2000.0
REGISTER_COUNT = 7

SAMPLE = re.compile(
    r"^ICM20608 ACCEL_mg=(-?\d+),(-?\d+),(-?\d+) GYRO_dps=(-?\d+),(-?\d+),(-?\d+)$"
)
REGS = re.compile(r"^ICM20608_REGS_HEX=((?:[0-9A-F]{2},){6}[0-9A-F]{2})$")
RAW = re.compile(r"^ICM20608_RAW14_HEX=([0-9A-F]{28})$")
WHOAMI = re.compile(r"^ICM20608_WHO_AM_I_RAW=(\d+)$")

ERROR_MARKERS = ("PANDORA_ICM20608_", "HardFault", "ASSERT")

ORDERED_MARKERS = (
    "PANDORA ICM20608 SENSOR READY",
    "I2C3_PINS SCL=PC0 SDA=PC1",
    "PANDORA_I2C3_INIT_DONE",
    "PANDORA_I2C3_SCAN_DONE count=",
)


def analyze_capture(payload: bytes, firmware_commit: str) -> dict:
    failures: list[str] = []
    if re.fullmatch(r"[0-9a-f]{40}", firmware_commit) is None:
        failures.append("invalid exact firmware commit")

    non_ascii_count = sum(byte > 0x7F for byte in payload)
    text = payload.decode("ascii", errors="replace")
    if non_ascii_count > 4:
        failures.append(f"non-ASCII byte count {non_ascii_count} > 4")

    ordered = (ORDERED_MARKERS[0], f"FIRMWARE_COMMIT {firmware_commit}", *ORDERED_MARKERS[1:])
    position = -1
    evidence_start = -1
    for marker in ordered:
        found = text.find(marker, position + 1)
        if found < 0:
            failures.append(f"missing or out-of-order marker: {marker}")
        else:
            if evidence_start < 0:
                evidence_start = found
            position = found

    evidence_text = text[evidence_start:] if evidence_start >= 0 else text
    for marker in ERROR_MARKERS:
        if marker in evidence_text:
            failures.append(f"error marker present: {marker}")

    whoami_values = [
        int(match.group(1))
        for match in (WHOAMI.fullmatch(line) for line in evidence_text.replace("\r", "").splitlines())
        if match is not None
    ]
    if not whoami_values:
        failures.append("missing ICM20608_WHO_AM_I_RAW")
    elif any(value != WHO_AM_I for value in whoami_values):
        failures.append(
            f"WHO_AM_I mismatch: {[hex(v) for v in whoami_values]} != {hex(WHO_AM_I)}"
        )

    samples: list[dict] = []
    malformed = 0
    register_rows = 0
    raw_rows = 0
    for line in evidence_text.replace("\r", "").splitlines():
        if line.startswith("ICM20608 ACCEL_mg="):
            match = SAMPLE.fullmatch(line)
            if match is None:
                malformed += 1
                continue
            accel = [int(match.group(index)) for index in (1, 2, 3)]
            gyro = [int(match.group(index)) for index in (4, 5, 6)]
            samples.append({"accel_mg": accel, "gyro_dps": gyro})
        elif line.startswith("ICM20608_REGS_HEX="):
            match = REGS.fullmatch(line)
            if match is None or len(match.group(1).split(",")) != REGISTER_COUNT:
                malformed += 1
            else:
                register_rows += 1
                values = [int(pair, 16) for pair in match.group(1).split(",")]
                if all(value == 0xFF for value in values):
                    failures.append("register block reads all 0xFF (bus not answering)")
                if all(value == 0x00 for value in values):
                    failures.append("register block reads all 0x00 (device not configured)")
        elif line.startswith("ICM20608_RAW14_HEX="):
            if RAW.fullmatch(line) is None:
                malformed += 1
            else:
                raw_rows += 1

    if malformed:
        failures.append(f"malformed sample lines: {malformed}")
    if not samples:
        failures.append("no decoded samples")
    elif len(samples) < SAMPLE_COUNT_MIN:
        failures.append(f"only {len(samples)} samples, need at least {SAMPLE_COUNT_MIN}")
    if register_rows == 0:
        failures.append("no register block")
    if raw_rows != len(samples):
        failures.append(f"raw rows {raw_rows} do not match sample rows {len(samples)}")

    magnitudes = [
        (sum(axis * axis for axis in sample["accel_mg"]) ** 0.5) for sample in samples
    ]
    for sample, magnitude in zip(samples, magnitudes):
        if not ACCEL_MAGNITUDE_MIN_MG <= magnitude <= ACCEL_MAGNITUDE_MAX_MG:
            failures.append(
                f"accel magnitude {magnitude:.0f} mg outside "
                f"{ACCEL_MAGNITUDE_MIN_MG:.0f}..{ACCEL_MAGNITUDE_MAX_MG:.0f}"
            )
            break
    for sample in samples:
        if any(abs(value) > GYRO_LIMIT_DPS for value in sample["gyro_dps"]):
            failures.append(f"gyro sample outside +/-{GYRO_LIMIT_DPS:.0f} dps")
            break

    return {
        "status": "ICM20608_B1_REVIEW_CANDIDATE" if not failures else "CONTENT_MISMATCH",
        "failures": failures,
        "samples": len(samples),
        "register_rows": register_rows,
        "raw_rows": raw_rows,
        "whoami": [hex(value) for value in whoami_values],
        "accel_magnitude_mg": [round(magnitude, 1) for magnitude in magnitudes],
        "firmware_commit": firmware_commit,
        "bytes_captured": len(payload),
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("capture", type=Path)
    parser.add_argument("--firmware-commit", required=True)
    parser.add_argument("--metadata", type=Path)
    arguments = parser.parse_args()

    result = analyze_capture(arguments.capture.read_bytes(), arguments.firmware_commit)
    if arguments.metadata is not None:
        arguments.metadata.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(result, indent=2))
    return 0 if result["status"] == "ICM20608_B1_REVIEW_CANDIDATE" else 1


if __name__ == "__main__":
    raise SystemExit(main())
