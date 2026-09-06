#!/usr/bin/env python3
"""Validate a bounded Pandora motor/vibration UART capture."""

import argparse
import json
import re
from pathlib import Path


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("capture", type=Path)
    parser.add_argument("--firmware-commit", required=True)
    parser.add_argument("--json", required=True, type=Path)
    args = parser.parse_args()
    reasons = []
    text = ""
    if not re.fullmatch(r"[0-9a-f]{40}", args.firmware_commit):
        reasons.append("invalid exact firmware commit")
    try:
        text = args.capture.read_text(encoding="ascii")
    except (OSError, UnicodeError) as error:
        reasons.append(f"capture unreadable: {error}")
    required = (
        "PANDORA MOTOR PA1=INA PA0=INB READY",
        f"FIRMWARE_COMMIT {args.firmware_commit}",
        "PANDORA_MOTOR_PATTERN_START FORWARD_120_120_300",
        "PANDORA_MOTOR_PATTERN_DONE",
        "PANDORA_MOTOR_FINAL_STANDBY",
    )
    position = -1
    for marker in required:
        found = text.find(marker, position + 1)
        if found < 0:
            reasons.append(f"missing or out-of-order marker: {marker}")
        else:
            position = found
    for marker in ("_ERROR", "HardFault", "ASSERT"):
        if marker in text:
            reasons.append(f"error marker present: {marker}")
    status = "CONTROL_PATH_REVIEW_CANDIDATE" if not reasons else "FAILED"
    record = {
        "status": status,
        "firmware_commit": args.firmware_commit,
        "bytes_captured": len(text.encode("ascii", errors="ignore")),
        "schematic_mapping": "PA1_MOTOR_A_IA;PA0_MOTOR_B_IB",
        "driver_identification": "TC214B_USER_CONFIRMED;TC214B_SCHEMATIC_LABEL",
        "identification_provenance":
            "USER_SUPPLIED_MODEL_AND_DATASHEET_TRUTH_TABLE;NOT_INDEPENDENTLY_FETCHED",
        "pattern": "FORWARD_120MS;STANDBY_120MS;FORWARD_120MS;STANDBY_120MS;FORWARD_300MS",
        "final_state": "STANDBY_LOW_LOW",
        "physical_confirmation": "USER_CONFIRMED_SHORT_SHORT_LONG_AND_FINAL_STOP",
        "reasons": reasons,
    }
    args.json.parent.mkdir(parents=True, exist_ok=True)
    args.json.write_text(json.dumps(record, indent=2) + "\n", encoding="utf-8")
    print(status)
    return 0 if not reasons else 1


if __name__ == "__main__":
    raise SystemExit(main())