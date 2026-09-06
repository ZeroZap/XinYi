#!/usr/bin/env python3
import json
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
VALIDATOR = ROOT / "boards/pandora_stm32l475/validate_motor_capture.py"
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


GOOD = (
    "PANDORA MOTOR PA1=INA PA0=INB READY\r\n"
    f"FIRMWARE_COMMIT {COMMIT}\r\n"
    "PANDORA_MOTOR_PATTERN_START FORWARD_120_120_300\r\n"
    "PANDORA_MOTOR_PATTERN_DONE\r\n"
    "PANDORA_MOTOR_FINAL_STANDBY\r\n"
)
good = run(GOOD, 0)
assert good["status"] == "CONTROL_PATH_REVIEW_CANDIDATE"
assert good["schematic_mapping"] == "PA1_MOTOR_A_IA;PA0_MOTOR_B_IB"
assert good["driver_identification"] == "TC214B_USER_CONFIRMED;TC214B_SCHEMATIC_LABEL"
assert good["identification_provenance"] == (
    "USER_SUPPLIED_MODEL_AND_DATASHEET_TRUTH_TABLE;NOT_INDEPENDENTLY_FETCHED")
assert good["final_state"] == "STANDBY_LOW_LOW"
assert good["physical_confirmation"] == "USER_CONFIRMED_SHORT_SHORT_LONG_AND_FINAL_STOP"
assert run(GOOD.replace("PANDORA_MOTOR_PATTERN_DONE\r\n", ""), 1)["status"] == "FAILED"
assert run(GOOD.replace("PANDORA_MOTOR_PATTERN_DONE", "PANDORA_MOTOR_PATTERN_ERROR"), 1)[
    "status"] == "FAILED"
assert run(GOOD.replace("PANDORA_MOTOR_PATTERN_START FORWARD_120_120_300\r\n", "").replace(
    "PANDORA_MOTOR_PATTERN_DONE\r\n", "PANDORA_MOTOR_PATTERN_DONE\r\n"
    "PANDORA_MOTOR_PATTERN_START FORWARD_120_120_300\r\n"), 1)["status"] == "FAILED"