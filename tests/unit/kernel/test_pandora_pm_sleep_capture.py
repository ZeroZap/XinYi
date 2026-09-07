#!/usr/bin/env python3
"""Contracts for the Pandora PM sleep/wakeup capture validator."""

import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[3]
VALIDATOR = ROOT / "boards" / "pandora_stm32l475" / "validate_pm_sleep_capture.py"
COMMIT = "0123456789abcdef0123456789abcdef01234567"


def valid_payload() -> bytes:
    return (
        b"PANDORA STM32L475VE XINYI OSAL FREERTOS READY\r\n"
        b"FIRMWARE_COMMIT 0123456789abcdef0123456789abcdef01234567\r\n"
        b"OSAL_PM_SLEEP_ENTER\r\n"
        b"OSAL_PM_WAKE_IRQ\r\n"
        b"OSAL_PM_SLEEP_WAKE_OK\r\n"
        b"OSAL_IPC_SEND\r\n"
    )


class PandoraPmSleepCaptureValidatorTest(unittest.TestCase):
    def run_validator(self, payload: bytes, commit: str = COMMIT):
        with tempfile.TemporaryDirectory() as temporary:
            capture = Path(temporary) / "capture.raw"
            record = Path(temporary) / "record.json"
            capture.write_bytes(payload)
            result = subprocess.run(
                [
                    sys.executable,
                    str(VALIDATOR),
                    "--capture",
                    str(capture),
                    "--firmware-commit",
                    commit,
                    "--record",
                    str(record),
                ],
                cwd=ROOT,
                text=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                check=False,
            )
            parsed = json.loads(record.read_text(encoding="utf-8")) if record.exists() else None
            self.assertIsNotNone(parsed, result.stderr)
            return result, parsed

    def test_accepts_ordered_identity_bound_sleep_wakeup_chain(self):
        result, record = self.run_validator(valid_payload())
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(record["status"], "B1_PM_SLEEP_WAKE_PASS")
        self.assertEqual(record["captured_bytes"], len(valid_payload()))
        self.assertEqual(record["marker_counts"]["OSAL_PM_SLEEP_WAKE_OK"], 1)
        self.assertEqual(record["error_marker_count"], 0)

    def test_rejects_empty_capture(self):
        result, record = self.run_validator(b"")
        self.assertNotEqual(result.returncode, 0)
        self.assertEqual(record["status"], "REJECTED")
        self.assertIn("capture is empty", record["failures"])

    def test_rejects_wrong_identity(self):
        payload = valid_payload().replace(COMMIT.encode(), b"f" * 40)
        result, record = self.run_validator(payload)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("firmware identity count 0 != 1", record["failures"])

    def test_rejects_out_of_order_wakeup_chain(self):
        payload = valid_payload().replace(
            b"OSAL_PM_SLEEP_ENTER\r\nOSAL_PM_WAKE_IRQ\r\n",
            b"OSAL_PM_WAKE_IRQ\r\nOSAL_PM_SLEEP_ENTER\r\n",
        )
        result, record = self.run_validator(payload)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("PM markers are not strictly ordered", record["failures"])

    def test_rejects_any_pm_error_marker(self):
        payload = valid_payload() + b"OSAL_PM_WAKE_ERROR\r\n"
        result, record = self.run_validator(payload)
        self.assertNotEqual(result.returncode, 0)
        self.assertEqual(record["error_marker_count"], 1)


if __name__ == "__main__":
    unittest.main()
