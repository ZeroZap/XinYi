#!/usr/bin/env python3
"""Validate a bounded Pandora AHT10 (software-I2C4) capture."""

import argparse
import json
from pathlib import Path
import re

SAMPLE = re.compile(r"^AHT10 SAMPLE hum_centi=([0-9]+) temp_centi=(-?[0-9]+)$")
ERROR_MARKERS = ("PANDORA_AHT10_", "HardFault", "ASSERT")
HUMIDITY_MAX_CENTI = 10000
TEMPERATURE_MIN_CENTI = -4000
TEMPERATURE_MAX_CENTI = 8500
MIN_SAMPLES = 10


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
        "status": "AHT10_SOFT_I2C4_B1" if not failures else "AHT10_SOFT_I2C4_VALIDATION_FAILED",
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
            "status": "AHT10_SOFT_I2C4_VALIDATION_FAILED",
            "firmware_commit": args.firmware_commit,
            "sample_count": 0,
            "failures": [f"capture unreadable: {error}"],
        }
    else:
        result = analyze_capture(payload, args.firmware_commit)

    args.json.parent.mkdir(parents=True, exist_ok=True)
    args.json.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(result["status"])
    return 0 if result["status"] == "AHT10_SOFT_I2C4_B1" else 1


if __name__ == "__main__":
    raise SystemExit(main())
