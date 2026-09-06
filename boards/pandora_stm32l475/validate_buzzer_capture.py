#!/usr/bin/env python3
import argparse
import json
from pathlib import Path


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("capture", type=Path)
    parser.add_argument("--firmware-commit", required=True)
    parser.add_argument("--json", type=Path)
    args = parser.parse_args()
    text = args.capture.read_bytes().decode("ascii", errors="replace")
    required = [
        "PANDORA BUZZER PB2 READY",
        f"FIRMWARE_COMMIT {args.firmware_commit}",
        "PANDORA_BUZZER_PATTERN_START SHORT_SHORT_LONG",
        "PANDORA_BUZZER_PATTERN_DONE",
        "PANDORA_BUZZER_FINAL_OFF",
    ]
    positions = [text.find(marker) for marker in required]
    errors = [marker for marker, position in zip(required, positions) if position < 0]
    if positions != sorted(positions):
        errors.append("marker ordering")
    if "PANDORA_BUZZER_INIT_ERROR" in text or "PANDORA_BUZZER_PATTERN_ERROR" in text or \
       "PANDORA_BUZZER_FINAL_OFF_ERROR" in text:
        errors.append("firmware error marker")
    result = {
        "status": "CONTROL_PATH_REVIEW_CANDIDATE" if not errors else "FAILED",
        "firmware_commit": args.firmware_commit,
        "captured_bytes": len(text.encode("ascii", errors="replace")),
        "required_markers": required,
        "errors": errors,
        "audible_confirmation": "PENDING_HUMAN_CONFIRMATION",
        "claim_boundary": "UART markers and firmware readback do not prove audible output",
    }
    if args.json:
        args.json.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(result, sort_keys=True))
    return 0 if not errors else 1


if __name__ == "__main__":
    raise SystemExit(main())