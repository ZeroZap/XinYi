#!/usr/bin/env python3
"""Contracts for the Pandora TF-card runtime capture validator."""

import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[3]
VALIDATOR = ROOT / "boards" / "pandora_stm32l475" / "validate_tf_capture.py"
COMMIT = "0123456789abcdef0123456789abcdef01234567"
FIRMWARE_SHA256 = "a" * 64


def valid_payload() -> bytes:
    return (
        b"PANDORA TF SPI1 PROBE\r\n"
        + f"FIRMWARE_COMMIT {COMMIT}\r\n".encode()
        + b"PANDORA_TF_READY type=SDHC blocks=62521344 bytes=32010928128\r\n"
        + b"PANDORA_TF_BLOCK0_OK signature=55AA\r\n"
        + b"PANDORA_TF_PROBE_DONE\r\n"
    )


class PandoraTfCaptureValidatorTest(unittest.TestCase):
    def run_validator(self, payload: bytes, readback_sha256: str = FIRMWARE_SHA256):
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
                    COMMIT,
                    "--firmware-sha256",
                    FIRMWARE_SHA256,
                    "--readback-sha256",
                    readback_sha256,
                    "--probe-serial",
                    "TEST-PROBE",
                    "--record",
                    str(record),
                ],
                cwd=ROOT,
                text=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                check=False,
            )
            self.assertTrue(record.exists(), result.stderr)
            parsed = json.loads(record.read_text(encoding="utf-8"))
            return result, parsed

    def test_accepts_ordered_identity_bound_read_only_capture(self):
        result, record = self.run_validator(valid_payload())
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(record["status"], "B1_PANDORA_TF_READ_PASS")
        self.assertEqual(record["card_type"], "SDHC")
        self.assertEqual(record["block_count"], 62521344)
        self.assertEqual(record["capacity_bytes"], 32010928128)
        self.assertTrue(record["readback_byte_identical"])
        self.assertEqual(record["error_marker_count"], 0)

    def test_rejects_empty_capture(self):
        result, record = self.run_validator(b"")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("capture is empty", record["failures"])

    def test_rejects_wrong_identity(self):
        payload = valid_payload().replace(COMMIT.encode(), b"f" * 40)
        result, record = self.run_validator(payload)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("firmware identity count 0 != 1", record["failures"])

    def test_rejects_out_of_order_markers(self):
        payload = valid_payload().replace(
            b"PANDORA_TF_BLOCK0_OK signature=55AA\r\nPANDORA_TF_PROBE_DONE\r\n",
            b"PANDORA_TF_PROBE_DONE\r\nPANDORA_TF_BLOCK0_OK signature=55AA\r\n",
        )
        result, record = self.run_validator(payload)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("TF-card markers are not strictly ordered", record["failures"])

    def test_rejects_error_marker(self):
        result, record = self.run_validator(valid_payload() + b"PANDORA_TF_READ_ERROR error=3\r\n")
        self.assertNotEqual(result.returncode, 0)
        self.assertEqual(record["error_marker_count"], 1)

    def test_rejects_flash_readback_mismatch(self):
        result, record = self.run_validator(valid_payload(), "b" * 64)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("Flash read-back SHA-256 differs from firmware SHA-256", record["failures"])


if __name__ == "__main__":
    unittest.main()
