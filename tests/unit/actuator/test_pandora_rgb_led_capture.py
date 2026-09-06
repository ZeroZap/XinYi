#!/usr/bin/env python3
import json
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
VALIDATOR = ROOT / "boards/pandora_stm32l475/validate_rgb_led_capture.py"
COMMIT = "0123456789abcdef0123456789abcdef01234567"


def run(text: str, expected: int) -> dict:
    with tempfile.TemporaryDirectory() as directory:
        capture = Path(directory) / "capture.txt"
        record = Path(directory) / "record.json"
        capture.write_text(text, encoding="ascii")
        completed = subprocess.run(
            ["python3", str(VALIDATOR), str(capture), "--firmware-commit", COMMIT,
             "--json", str(record)], check=False)
        assert completed.returncode == expected
        return json.loads(record.read_text(encoding="utf-8"))


good = run(
    "PANDORA RGB PE7/PE8/PE9 READY ACTIVE_LOW\r\n"
    f"FIRMWARE_COMMIT {COMMIT}\r\n"
    "PANDORA_MOTOR_SKIPPED TC214B_TRUTH_TABLE_UNVERIFIED\r\n"
    "PANDORA_RGB_PATTERN_START RED_GREEN_BLUE_WHITE\r\n"
    "PANDORA_RGB_PATTERN_DONE\r\n"
    "PANDORA_RGB_FINAL_OFF\r\n", 0)
assert good["status"] == "CONTROL_PATH_REVIEW_CANDIDATE"
assert good["motor_drive"] == "REFUSED_UNVERIFIED_TC214B_TRUTH_TABLE"
assert good["visual_confirmation"] == "PENDING_HUMAN_CONFIRMATION"
assert run("PANDORA RGB PE7/PE8/PE9 READY ACTIVE_LOW\nPANDORA_RGB_FINAL_OFF\n", 1)[
    "status"] == "FAILED"
assert run(
    "PANDORA RGB PE7/PE8/PE9 READY ACTIVE_LOW\n"
    f"FIRMWARE_COMMIT {COMMIT}\n"
    "PANDORA_MOTOR_SKIPPED TC214B_TRUTH_TABLE_UNVERIFIED\n"
    "PANDORA_RGB_PATTERN_DONE\n"
    "PANDORA_RGB_PATTERN_START RED_GREEN_BLUE_WHITE\n"
    "PANDORA_RGB_FINAL_OFF\n", 1)["status"] == "FAILED"
assert run(
    "PANDORA RGB PE7/PE8/PE9 READY ACTIVE_LOW\n"
    f"FIRMWARE_COMMIT {COMMIT}\n"
    "PANDORA_MOTOR_SKIPPED TC214B_TRUTH_TABLE_UNVERIFIED\n"
    "PANDORA_RGB_PATTERN_START RED_GREEN_BLUE_WHITE\n"
    "PANDORA_RGB_PATTERN_ERROR\n"
    "PANDORA_RGB_PATTERN_DONE\n"
    "PANDORA_RGB_FINAL_OFF\n", 1)["status"] == "FAILED"
