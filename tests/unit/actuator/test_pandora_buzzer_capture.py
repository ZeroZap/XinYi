#!/usr/bin/env python3
import json
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
VALIDATOR = ROOT / "boards/pandora_stm32l475/validate_buzzer_capture.py"
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
    "PANDORA BUZZER PB2 READY\r\n"
    f"FIRMWARE_COMMIT {COMMIT}\r\n"
    "PANDORA_BUZZER_PATTERN_START SHORT_SHORT_LONG\r\n"
    "PANDORA_BUZZER_PATTERN_DONE\r\n"
    "PANDORA_BUZZER_FINAL_OFF\r\n", 0)
assert good["status"] == "CONTROL_PATH_REVIEW_CANDIDATE"
assert good["audible_confirmation"] == "PENDING_HUMAN_CONFIRMATION"
assert run("PANDORA BUZZER PB2 READY\nPANDORA_BUZZER_FINAL_OFF\n", 1)["status"] == "FAILED"
assert run(
    "PANDORA BUZZER PB2 READY\n"
    f"FIRMWARE_COMMIT {COMMIT}\n"
    "PANDORA_BUZZER_PATTERN_DONE\n"
    "PANDORA_BUZZER_PATTERN_START SHORT_SHORT_LONG\n"
    "PANDORA_BUZZER_FINAL_OFF\n", 1)["status"] == "FAILED"