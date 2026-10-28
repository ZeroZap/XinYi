#!/usr/bin/env python3
"""Fail-closed contracts for Pandora AHT10 software-I2C4 captures."""

from pathlib import Path
import json
import os
import subprocess
import sys
import tempfile
import time
import unittest

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "boards" / "pandora_stm32l475"))

import validate_aht10_capture as aht  # noqa: E402
from validate_aht10_capture import analyze_capture  # noqa: E402

COMMIT = "1" * 40
SCRIPT = ROOT / "boards" / "pandora_stm32l475" / "validate_aht10_capture.py"
HEADER = "\r\n".join(
    (
        "PANDORA AHT10 SENSOR READY",
        f"FIRMWARE_COMMIT {COMMIT}",
        "AHT10_BUS=SOFT_I2C4 SCL=PD6 SDA=PC1 ADDR=0x38",
        "PANDORA_I2C4_INIT_DONE",
        "I2C4_ACK=0x38",
        "PANDORA_I2C4_SCAN_DONE count=1",
        "AHT10_DRIVER_READY ADDR=0x38",
    )
)


def sample(index: int) -> str:
    return f"AHT10 SAMPLE hum_centi={6000 + index} temp_centi={2000 + index}"


def body(count: int = 10) -> str:
    return "\r\n".join(sample(index) for index in range(count))


class PandoraAht10CaptureContract(unittest.TestCase):
    def test_accepts_ordered_identity_and_bounded_samples(self) -> None:
        payload = ((HEADER + "\r\n" + body()).encode() + b"\r\nPAND\xffPANDORA AHT10 SENSOR READY\r\n")

        result = analyze_capture(payload, COMMIT)

        self.assertEqual(result["status"], "AHT10_SOFT_I2C4_B1")
        self.assertEqual(result["sample_count"], 10)
        self.assertEqual(result["unique_sample_count"], 10)
        self.assertEqual(result["humidity_centi_range"], [6000, 6009])
        self.assertEqual(result["temperature_centi_range"], [2000, 2009])
        self.assertEqual(result["non_ascii_byte_count"], 1)
        self.assertEqual(result["failures"], [])

    def test_rejects_wrong_bus_or_error_marker(self) -> None:
        payload = (HEADER.replace("SOFT_I2C4", "HW_I2C3") + "\n" + body() +
                   "\nPANDORA_AHT10_READ_ERROR\n").encode()

        result = analyze_capture(payload, COMMIT)

        self.assertEqual(result["status"], "AHT10_SOFT_I2C4_VALIDATION_FAILED")
        self.assertTrue(any("SOFT_I2C4" in failure for failure in result["failures"]))
        self.assertTrue(any("error marker" in failure for failure in result["failures"]))

    def test_rejects_wrong_firmware_commit(self) -> None:
        result = analyze_capture((HEADER + "\n" + body()).encode(), "2" * 40)

        self.assertEqual(result["status"], "AHT10_SOFT_I2C4_VALIDATION_FAILED")
        self.assertTrue(
            any("FIRMWARE_COMMIT" in failure for failure in result["failures"])
        )

    def test_rejects_missing_or_out_of_order_driver_ready(self) -> None:
        samples = body()
        missing = HEADER.replace("AHT10_DRIVER_READY ADDR=0x38", "")
        out_of_order = HEADER.replace(
            "PANDORA_I2C4_SCAN_DONE count=1\r\nAHT10_DRIVER_READY ADDR=0x38",
            "AHT10_DRIVER_READY ADDR=0x38\r\nPANDORA_I2C4_SCAN_DONE count=1",
        )

        for payload in (missing, out_of_order):
            result = analyze_capture((payload + "\n" + samples).encode(), COMMIT)
            self.assertEqual(result["status"], "AHT10_SOFT_I2C4_VALIDATION_FAILED")
            self.assertTrue(
                any("DRIVER_READY" in failure for failure in result["failures"])
            )

    def test_rejects_short_malformed_or_out_of_range_samples(self) -> None:
        payload = (HEADER + "\n" + body(6) +
                   "\nAHT10 SAMPLE hum_centi=bad temp_centi=0" +
                   "\nAHT10 SAMPLE hum_centi=10001 temp_centi=2500" +
                   "\nAHT10 SAMPLE hum_centi=5000 temp_centi=8600").encode()

        result = analyze_capture(payload, COMMIT)

        self.assertEqual(result["status"], "AHT10_SOFT_I2C4_VALIDATION_FAILED")
        self.assertIn("malformed sample count 1", result["failures"])
        self.assertIn("sample outside AHT10 humidity range", result["failures"])
        self.assertIn("sample outside AHT10 temperature range", result["failures"])
        self.assertIn("sample count 8 < 10", result["failures"])

    def test_capture_resets_after_opening_and_judges_the_window(self) -> None:
        import pty
        import threading

        master, slave = pty.openpty()
        slave_path = os.ttyname(slave)
        window = (HEADER + "\r\n" + body()).encode()

        def writer() -> None:
            time.sleep(0.1)
            os.write(master, window)

        writer_thread = threading.Thread(target=writer)
        writer_thread.start()
        try:
            status, payload, error = aht.capture_window(slave_path, 0.5)
        finally:
            writer_thread.join()
            os.close(master)
            os.close(slave)

        self.assertEqual(status, "CAPTURED")
        self.assertIsNone(error)
        self.assertEqual(analyze_capture(payload, COMMIT)["status"], "AHT10_SOFT_I2C4_B1")

    def test_capture_window_reports_zero_bytes_instead_of_claiming_a_pass(self) -> None:
        import pty

        master, slave = pty.openpty()
        slave_path = os.ttyname(slave)
        try:
            status, payload, error = aht.capture_window(slave_path, 0.2)
        finally:
            os.close(master)
            os.close(slave)

        self.assertEqual(status, "NO_DATA_TIMEOUT")
        self.assertEqual(payload, b"")
        self.assertIsNone(error)

    def test_capture_cli_reports_a_missing_device(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            metadata = Path(directory) / "capture.json"
            result = subprocess.run(
                [
                    sys.executable,
                    str(SCRIPT),
                    "capture",
                    "--device",
                    str(Path(directory) / "does-not-exist"),
                    "--output",
                    str(Path(directory) / "capture.txt"),
                    "--metadata",
                    str(metadata),
                    "--firmware-commit",
                    COMMIT,
                ],
                capture_output=True,
                text=True,
                check=False,
            )
            record = json.loads(metadata.read_text())

        self.assertEqual(result.returncode, 3, result.stdout + result.stderr)
        self.assertIn("DEVICE_OPEN_FAILED", result.stdout)
        self.assertEqual(record["status"], "DEVICE_OPEN_FAILED")

    def test_analyze_cli_fails_closed_on_a_mismatched_commit(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            capture = Path(directory) / "capture.txt"
            capture.write_bytes((HEADER + "\r\n" + body()).encode())
            metadata = Path(directory) / "capture.json"
            result = subprocess.run(
                [
                    sys.executable,
                    str(SCRIPT),
                    "analyze",
                    "--input",
                    str(capture),
                    "--firmware-commit",
                    "2" * 40,
                    "--metadata",
                    str(metadata),
                ],
                capture_output=True,
                text=True,
                check=False,
            )

        self.assertEqual(result.returncode, 1)
        self.assertIn("AHT10_SOFT_I2C4_VALIDATION_FAILED", result.stdout)
        self.assertIn("FIRMWARE_COMMIT", result.stdout)


if __name__ == "__main__":
    unittest.main()