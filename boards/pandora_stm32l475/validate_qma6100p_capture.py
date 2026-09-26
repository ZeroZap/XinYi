#!/usr/bin/env python3
"""Validate an identity-bound Pandora QMA6100P basic-chain capture."""

import argparse
import hashlib
import json
from pathlib import Path
import re

BANNER = "PANDORA QMA6100P I2C2 PROBE"
IDENTITY_PREFIX = "FIRMWARE_COMMIT "
DEVICE = re.compile(r"^QMA6100P_ADDR=(0x1[23]) CHIP_ID=(0x90)$")
SAMPLE = re.compile(
    r"^QMA6100P_SAMPLE n=([0-9]+) raw=(-?[0-9]+),(-?[0-9]+),(-?[0-9]+) "
    r"mg=(-?[0-9]+),(-?[0-9]+),(-?[0-9]+) status=0x([0-9A-F]{2}) "
    r"int1_level=([01]) int2_level=([01]) int1_edges=([0-9]+) int2_edges=([0-9]+)$"
)
IRQ_MAP = "QMA6100P_IRQ_MAP INT1=PC6 INT2=PD15 ACTIVE=HIGH"
DONE = "QMA6100P_PROBE_DONE"
ERROR_MARKERS = (
    "QMA6100P_NOT_FOUND",
    "QMA6100P_ID_IO_ERROR",
    "QMA6100P_INIT_ERROR",
    "QMA6100P_IRQ_CONFIG_ERROR",
    "QMA6100P_STATUS_ERROR",
    "QMA6100P_RAW_ERROR",
    "QMA6100P_ACCEL_ERROR",
    "HardFault",
    "ASSERT",
)


def analyze_capture(payload: bytes, firmware_commit: str) -> dict:
    failures: list[str] = []
    if re.fullmatch(r"[0-9a-f]{40}", firmware_commit) is None:
        failures.append("invalid exact firmware commit")

    text = payload.decode("ascii", errors="replace")
    lines = [line.strip() for line in text.replace("\r", "").splitlines() if line.strip()]
    banner_positions = [index for index, line in enumerate(lines) if line == BANNER]
    if not banner_positions:
        failures.append("missing firmware banner")
        cycle = lines
    else:
        cycle = lines[banner_positions[-1] :]

    expected_identity = IDENTITY_PREFIX + firmware_commit
    if len(cycle) < 2 or cycle[1] != expected_identity:
        failures.append("missing or out-of-order exact firmware identity")

    address = None
    chip_id = None
    if len(cycle) < 3:
        failures.append("missing device identity")
    else:
        device = DEVICE.fullmatch(cycle[2])
        if device is None:
            failures.append("invalid QMA6100P address or chip identity")
        else:
            address, chip_id = device.groups()

    if len(cycle) < 4 or cycle[3] != IRQ_MAP:
        failures.append("missing or out-of-order IRQ mapping marker")

    for marker in ERROR_MARKERS:
        if any(marker in line for line in cycle):
            failures.append(f"error marker present: {marker}")

    samples = []
    malformed_samples = 0
    for line in cycle:
        if not line.startswith("QMA6100P_SAMPLE"):
            continue
        match = SAMPLE.fullmatch(line)
        if match is None:
            malformed_samples += 1
            continue
        sequence = int(match.group(1))
        raw = tuple(int(match.group(index)) for index in range(2, 5))
        mg = tuple(int(match.group(index)) for index in range(5, 8))
        status = match.group(8)
        levels = tuple(int(match.group(index)) for index in range(9, 11))
        edges = tuple(int(match.group(index)) for index in range(11, 13))
        if raw != tuple(max(-8192, min(8191, value)) for value in raw):
            failures.append(f"sample {sequence} raw axis outside signed 14-bit range")
        # The probe intentionally calls read_raw() and then read_accel(), which performs a
        # second sensor transaction.  Do not require the two snapshots to be bit-identical;
        # only guard the configured +/-2 g public output range here.
        if any(value < -2000 or value > 2000 for value in mg):
            failures.append(f"sample {sequence} acceleration outside configured range")
        samples.append((sequence, raw, mg, status, levels, edges))

    if malformed_samples:
        failures.append(f"malformed sample count {malformed_samples}")
    if len(samples) != 40:
        failures.append(f"sample count {len(samples)} != 40")
    sequences = [entry[0] for entry in samples]
    if sequences != list(range(40)):
        failures.append("sample sequence is not exact 0..39")
    if not cycle or cycle[-1] != DONE:
        failures.append("missing or out-of-order completion marker")

    unique_raw = len({entry[1] for entry in samples})
    if samples and unique_raw < 2:
        failures.append("raw samples are frozen")

    statuses = sorted({entry[3] for entry in samples})
    int1_edges_max = max((entry[5][0] for entry in samples), default=0)
    int2_edges_max = max((entry[5][1] for entry in samples), default=0)
    interrupt_observed = any(status != "00" for status in statuses) or int1_edges_max > 0 or int2_edges_max > 0

    raw_axes = [entry[1] for entry in samples]
    return {
        "status": "B1_QMA6100P_BASIC_CHAIN_PASS" if not failures else "QMA6100P_VALIDATION_FAILED",
        "firmware_commit": firmware_commit,
        "capture_bytes": len(payload),
        "capture_sha256": hashlib.sha256(payload).hexdigest(),
        "address": address,
        "chip_id": chip_id,
        "sample_count": len(samples),
        "unique_raw_samples": unique_raw,
        "raw_min": [min(axis) for axis in zip(*raw_axes)] if raw_axes else [],
        "raw_max": [max(axis) for axis in zip(*raw_axes)] if raw_axes else [],
        "interrupt_status_values": statuses,
        "int1_edges_max": int1_edges_max,
        "int2_edges_max": int2_edges_max,
        "interrupt_evidence": "OBSERVED" if interrupt_observed else "NOT_OBSERVED",
        "failures": failures,
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("capture", type=Path)
    parser.add_argument("--firmware-commit", required=True)
    parser.add_argument("--json", type=Path)
    args = parser.parse_args()

    try:
        payload = args.capture.read_bytes()
    except OSError as error:
        result = {
            "status": "QMA6100P_VALIDATION_FAILED",
            "firmware_commit": args.firmware_commit,
            "failures": [f"capture unreadable: {error}"],
        }
    else:
        result = analyze_capture(payload, args.firmware_commit)

    rendered = json.dumps(result, indent=2, sort_keys=True) + "\n"
    if args.json is not None:
        args.json.parent.mkdir(parents=True, exist_ok=True)
        args.json.write_text(rendered, encoding="utf-8")
    print(rendered, end="")
    return 0 if result["status"] == "B1_QMA6100P_BASIC_CHAIN_PASS" else 1


if __name__ == "__main__":
    raise SystemExit(main())
