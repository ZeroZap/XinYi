#!/usr/bin/env python3
"""Contracts for the Pandora Crypto runtime capture validator."""

import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[3]
VALIDATOR = ROOT / "boards" / "pandora_stm32l475" / "validate_crypto_capture.py"
COMMIT = "0123456789abcdef0123456789abcdef01234567"
FIRMWARE_SHA256 = "a" * 64


def valid_payload() -> bytes:
    return (
        b"PANDORA STM32L475VE CRYPTO SOFTWARE READY\r\n"
        + f"FIRMWARE_COMMIT {COMMIT}\r\n".encode()
        + b"CRYPTO_SHA256_KAT_PASS\r\n"
        + b"CRYPTO_HMAC_SHA256_KAT_PASS\r\n"
        + b"CRYPTO_AES128_KAT_PASS\r\n"
        + b"CRYPTO_REPEAT_1000_PASS\r\n"
        + b"B1_CRYPTO_SOFTWARE_KAT_PASS\r\n"
    )


class PandoraCryptoCaptureValidatorTest(unittest.TestCase):
    def run_validator(self, payload: bytes):
        with tempfile.TemporaryDirectory() as temporary:
            capture = Path(temporary) / "capture.raw"
            record = Path(temporary) / "record.json"
            capture.write_bytes(payload)
            result = subprocess.run(
                [sys.executable, str(VALIDATOR), "--capture", str(capture),
                 "--firmware-commit", COMMIT, "--firmware-sha256", FIRMWARE_SHA256,
                 "--readback-sha256", FIRMWARE_SHA256, "--probe-serial", "TEST-PROBE",
                 "--record", str(record)],
                cwd=ROOT, text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                check=False,
            )
            return result, json.loads(record.read_text(encoding="utf-8"))

    def test_accepts_ordered_identity_bound_capture(self):
        result, record = self.run_validator(valid_payload())
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(record["status"], "B1_CRYPTO_SOFTWARE_KAT_PASS")
        self.assertEqual(record["captured_bytes"], len(valid_payload()))
        self.assertEqual(record["error_marker_count"], 0)

    def test_rejects_empty_capture(self):
        result, record = self.run_validator(b"")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("capture is empty", record["failures"])

    def test_rejects_wrong_identity(self):
        result, record = self.run_validator(valid_payload().replace(COMMIT.encode(), b"f" * 40))
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("firmware identity count 0 != 1", record["failures"])

    def test_rejects_error_marker(self):
        result, record = self.run_validator(valid_payload() + b"CRYPTO_REPEAT_ERROR\r\n")
        self.assertNotEqual(result.returncode, 0)
        self.assertEqual(record["error_marker_count"], 1)

    def test_rejects_out_of_order_markers(self):
        payload = valid_payload().replace(
            b"CRYPTO_SHA256_KAT_PASS\r\nCRYPTO_HMAC_SHA256_KAT_PASS\r\n",
            b"CRYPTO_HMAC_SHA256_KAT_PASS\r\nCRYPTO_SHA256_KAT_PASS\r\n",
        )
        result, record = self.run_validator(payload)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("Crypto markers are not strictly ordered", record["failures"])


if __name__ == "__main__":
    unittest.main()
