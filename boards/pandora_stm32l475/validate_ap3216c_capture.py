#!/usr/bin/env python3
"""Validate a bounded Pandora AP3216C capture without overstating response scope."""

import argparse
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import re
import statistics
import sys

SAMPLE_RE = re.compile(
    r"AP3216C_INT=0x([0-9A-F]{2}) RAW_HEX=([0-9A-F]{12}) "
    r"ALS_lux=([0-9]+) PS_raw=([0-9]+) IR_raw=([0-9]+)"
)
MIN_SAMPLES = 100
MIN_UNIQUE_LINES = 50
WINDOW_SAMPLES = 50
MIN_IR_MEDIAN_DELTA = 128.0


def analyze_capture(payload: bytes) -> dict:
    text = payload.decode("ascii", errors="replace")
    nonempty = [line.strip() for line in text.splitlines() if line.strip()]
    samples = []
    malformed = []
    for line in nonempty:
        match = SAMPLE_RE.fullmatch(line)
        if match is None:
            malformed.append(line)
            continue
        interrupt, raw, als, ps, ir = match.groups()
        samples.append(
            {
                "interrupt": int(interrupt, 16),
                "raw": raw,
                "als": int(als),
                "ps": int(ps),
                "ir": int(ir),
                "line": line,
            }
        )

    failures = []
    if any("ERROR" in line for line in nonempty):
        failures.append("error marker present")
    if malformed:
        failures.append("unrecognized non-empty line present")
    if len(samples) < MIN_SAMPLES:
        failures.append(f"sample count {len(samples)} < {MIN_SAMPLES}")

    unique_lines = len({sample["line"] for sample in samples})
    if unique_lines < MIN_UNIQUE_LINES:
        failures.append(f"unique line count {unique_lines} < {MIN_UNIQUE_LINES}")

    first_median = None
    max_later_median = None
    response_delta = 0.0
    if len(samples) >= MIN_SAMPLES:
        ir_values = [sample["ir"] for sample in samples]
        first_median = float(statistics.median(ir_values[:WINDOW_SAMPLES]))
        max_later_median = max(
            float(statistics.median(ir_values[index : index + WINDOW_SAMPLES]))
            for index in range(WINDOW_SAMPLES, len(ir_values) - WINDOW_SAMPLES + 1)
        )
        response_delta = max_later_median - first_median
    if response_delta < MIN_IR_MEDIAN_DELTA:
        failures.append(
            f"IR median response delta {response_delta:.1f} < {MIN_IR_MEDIAN_DELTA:.1f}"
        )

    def value_range(field: str) -> dict | None:
        values = [sample[field] for sample in samples]
        if not values:
            return None
        return {"min": min(values), "max": max(values), "unique": len(set(values))}

    return {
        "status": (
            "AP3216C_BOUNDED_IR_RESPONSE_B1"
            if not failures
            else "AP3216C_CAPTURE_VALIDATION_FAILED"
        ),
        "capture_bytes": len(payload),
        "capture_sha256": hashlib.sha256(payload).hexdigest(),
        "sample_count": len(samples),
        "unique_line_count": unique_lines,
        "unique_raw_count": len({sample["raw"] for sample in samples}),
        "error_marker_count": sum("ERROR" in line for line in nonempty),
        "malformed_line_count": len(malformed),
        "ranges": {
            "als_lux": value_range("als"),
            "ps_raw": value_range("ps"),
            "ir_raw": value_range("ir"),
        },
        "first_window_samples": WINDOW_SAMPLES,
        "first_window_ir_median": first_median,
        "max_later_window_ir_median": max_later_median,
        "ir_median_response_delta": response_delta,
        "failures": failures,
        "evidence_scope": "AP3216C static B1 plus bounded IR stimulus-response B1",
        "not_evidence_for": [
            "quantitative ALS response",
            "quantitative proximity response",
            "interrupt-pin behavior",
            "sensor accuracy or calibration",
        ],
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--input", type=Path, required=True)
    parser.add_argument("--metadata", type=Path, required=True)
    parser.add_argument("--board", default="Pandora STM32L475VE V2.4")
    parser.add_argument("--firmware-commit", default="f320087cbcbe5d0ae107b27b96b42528438219e2")
    args = parser.parse_args()

    result = analyze_capture(args.input.read_bytes())
    result["board"] = args.board
    result["firmware_commit"] = args.firmware_commit
    result["recorded_at_utc"] = datetime.now(timezone.utc).isoformat()
    result["source"] = "user-assisted bounded optical stimulus capture"
    result["bus"] = "SOFT_I2C3 SCL=PC0 SDA=PC1"
    result["address_7bit"] = "0x1E"
    result["system_config"] = "0x03"
    args.metadata.parent.mkdir(parents=True, exist_ok=True)
    args.metadata.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n")
    print(result["status"])
    for failure in result["failures"]:
        print(f"- {failure}")
    return 0 if not result["failures"] else 1


if __name__ == "__main__":
    sys.exit(main())
