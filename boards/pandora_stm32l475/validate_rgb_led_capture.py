#!/usr/bin/env python3
"""Validate a bounded Pandora discrete RGB LED UART capture."""

import argparse
import json
import re
from pathlib import Path

ORDER = (
    "PANDORA RGB PE7/PE8/PE9 READY ACTIVE_LOW",
    "FIRMWARE_COMMIT {commit}",
    "PANDORA_MOTOR_SKIPPED TC214B_TRUTH_TABLE_UNVERIFIED",
    "PANDORA_RGB_PATTERN_START RED_GREEN_BLUE_WHITE",
    "PANDORA_RGB_PATTERN_DONE",
    "PANDORA_RGB_FINAL_OFF",
)
ERROR_MARKERS = ("_ERROR", "HardFault", "ASSERT")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("capture", type=Path)
    parser.add_argument("--firmware-commit", required=True)
    parser.add_argument("--json", required=True, type=Path)
    args = parser.parse_args()

    status = "FAILED"
    reasons = []
    text = ""
    if not re.fullmatch(r"[0-9a-f]{40}", args.firmware_commit):
        reasons.append("invalid exact firmware commit")
    try:
        text = args.capture.read_text(encoding="ascii")
    except (OSError, UnicodeError) as error:
        reasons.append(f"capture unreadable: {error}")

    position = -1
    for template in ORDER:
        marker = template.format(commit=args.firmware_commit)
        found = text.find(marker, position + 1)
        if found < 0:
            reasons.append(f"missing or out-of-order marker: {marker}")
        else:
            position = found
    for marker in ERROR_MARKERS:
        if marker in text:
            reasons.append(f"error marker present: {marker}")
    if not reasons:
        status = "CONTROL_PATH_REVIEW_CANDIDATE"

    record = {
        "status": status,
        "firmware_commit": args.firmware_commit,
        "bytes_captured": len(text.encode("ascii", errors="ignore")),
        "motor_drive": "REFUSED_UNVERIFIED_TC214B_TRUTH_TABLE",
        "visual_confirmation": "PENDING_HUMAN_CONFIRMATION",
        "expected_visual_pattern": "RED_GREEN_BLUE_WHITE_THEN_OFF",
        "reasons": reasons,
    }
    args.json.parent.mkdir(parents=True, exist_ok=True)
    args.json.write_text(json.dumps(record, indent=2) + "\n", encoding="utf-8")
    print(status)
    return 0 if status == "CONTROL_PATH_REVIEW_CANDIDATE" else 1


if __name__ == "__main__":
    raise SystemExit(main())
