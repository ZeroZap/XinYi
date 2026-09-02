#!/usr/bin/env python3
"""Host contract for the bounded Pandora UART capture helper."""

import json
import os
from pathlib import Path
import pty
import subprocess
import sys
import tempfile
import time
import unittest

ROOT = Path(__file__).resolve().parents[3]
CAPTURE = ROOT / "boards" / "pandora_stm32l475" / "capture_uart.py"
FIRMWARE_COMMIT = "0123456789abcdef0123456789abcdef01234567"


class PandoraUartCaptureTest(unittest.TestCase):
    def run_capture(
        self,
        device: str,
        output: Path,
        metadata: Path,
        timeout: float = 0.5,
        firmware_commit: str = FIRMWARE_COMMIT,
    ):
        command = [
            sys.executable,
            str(CAPTURE),
            "--device",
            device,
            "--output",
            str(output),
            "--metadata",
            str(metadata),
            "--timeout",
            str(timeout),
        ]
        if firmware_commit:
            command.extend(["--firmware-commit", firmware_commit])
        return subprocess.Popen(
            command,
            cwd=ROOT,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )

    def test_captures_uart_bytes_and_records_source_identity(self):
        master, slave = pty.openpty()
        device = os.ttyname(slave)
        with tempfile.TemporaryDirectory() as temporary:
            output = Path(temporary) / "uart.log"
            metadata = Path(temporary) / "capture.json"
            process = self.run_capture(device, output, metadata)
            time.sleep(0.1)
            payload = (
                b"PANDORA STM32L475VE XINYI SMOKE OK\r\n"
                b"AHT10 0x38 ACK\r\n"
                b"AHT10 RH_milli_percent=50234 T_milli_c=23125\r\n"
            )
            os.write(master, payload)
            stdout, stderr = process.communicate(timeout=2)

            self.assertEqual(process.returncode, 0, stderr)
            self.assertEqual(output.read_bytes(), payload)
            record = json.loads(metadata.read_text(encoding="utf-8"))
            self.assertEqual(record["status"], "CAPTURED")
            self.assertEqual(record["device"], device)
            self.assertEqual(record["bytes_captured"], len(payload))
            self.assertEqual(record["firmware_commit"], FIRMWARE_COMMIT)
            self.assertEqual(record["required_ack_marker"], "AHT10 0x38 ACK")
            self.assertEqual(record["runtime_evidence"], "B1_REVIEW_CANDIDATE")
            self.assertIn("CAPTURED", stdout)
        os.close(master)
        os.close(slave)

    def test_unrecognized_bytes_do_not_grant_successful_capture_status(self):
        master, slave = pty.openpty()
        device = os.ttyname(slave)
        with tempfile.TemporaryDirectory() as temporary:
            output = Path(temporary) / "uart.log"
            metadata = Path(temporary) / "capture.json"
            process = self.run_capture(device, output, metadata)
            time.sleep(0.1)
            payload = b"bootloader noise\r\n"
            os.write(master, payload)
            stdout, stderr = process.communicate(timeout=2)

            self.assertEqual(process.returncode, 3, stderr)
            self.assertEqual(output.read_bytes(), payload)
            record = json.loads(metadata.read_text(encoding="utf-8"))
            self.assertEqual(record["status"], "CAPTURE_CONTENT_MISMATCH")
            self.assertEqual(record["bytes_captured"], len(payload))
            self.assertEqual(record["required_marker"], "PANDORA STM32L475VE XINYI SMOKE OK")
            self.assertIn("Pandora banner", record["error"])
            self.assertIn("AHT10 measurement", record["error"])
            self.assertEqual(record["runtime_evidence"], "NONE")
            self.assertIn("CAPTURE_CONTENT_MISMATCH", stdout)
        os.close(master)
        os.close(slave)

    def test_banner_without_measurement_remains_review_ineligible(self):
        master, slave = pty.openpty()
        device = os.ttyname(slave)
        with tempfile.TemporaryDirectory() as temporary:
            output = Path(temporary) / "uart.log"
            metadata = Path(temporary) / "capture.json"
            process = self.run_capture(device, output, metadata)
            time.sleep(0.1)
            os.write(master, b"PANDORA STM32L475VE XINYI SMOKE OK\r\nAHT10 0x38 ACK\r\n")
            stdout, stderr = process.communicate(timeout=2)

            self.assertEqual(process.returncode, 3, stderr)
            record = json.loads(metadata.read_text(encoding="utf-8"))
            self.assertEqual(record["status"], "CAPTURE_CONTENT_MISMATCH")
            self.assertEqual(record["runtime_evidence"], "NONE")
            self.assertIn("AHT10 measurement", record["error"])
            self.assertIn("CAPTURE_CONTENT_MISMATCH", stdout)
        os.close(master)
        os.close(slave)

    def test_out_of_range_measurement_remains_review_ineligible(self):
        master, slave = pty.openpty()
        device = os.ttyname(slave)
        with tempfile.TemporaryDirectory() as temporary:
            output = Path(temporary) / "uart.log"
            metadata = Path(temporary) / "capture.json"
            process = self.run_capture(device, output, metadata)
            time.sleep(0.1)
            payload = (
                b"PANDORA STM32L475VE XINYI SMOKE OK\r\n"
                b"AHT10 RH_milli_percent=999999 T_milli_c=999999\r\n"
            )
            os.write(master, payload)
            stdout, stderr = process.communicate(timeout=2)

            self.assertEqual(process.returncode, 3, stderr)
            record = json.loads(metadata.read_text(encoding="utf-8"))
            self.assertEqual(record["status"], "CAPTURE_CONTENT_MISMATCH")
            self.assertEqual(record["runtime_evidence"], "NONE")
            self.assertIn("AHT10 measurement", record["error"])
            self.assertIn("CAPTURE_CONTENT_MISMATCH", stdout)
        os.close(master)
        os.close(slave)

    def test_measurement_without_ack_remains_review_ineligible(self):
        master, slave = pty.openpty()
        device = os.ttyname(slave)
        with tempfile.TemporaryDirectory() as temporary:
            output = Path(temporary) / "uart.log"
            metadata = Path(temporary) / "capture.json"
            process = self.run_capture(device, output, metadata)
            time.sleep(0.1)
            payload = (
                b"PANDORA STM32L475VE XINYI SMOKE OK\r\n"
                b"AHT10 RH_milli_percent=50234 T_milli_c=23125\r\n"
            )
            os.write(master, payload)
            stdout, stderr = process.communicate(timeout=2)

            self.assertEqual(process.returncode, 3, stderr)
            record = json.loads(metadata.read_text(encoding="utf-8"))
            self.assertEqual(record["status"], "CAPTURE_CONTENT_MISMATCH")
            self.assertEqual(record["runtime_evidence"], "NONE")
            self.assertIn("AHT10 ACK", record["error"])
            self.assertIn("CAPTURE_CONTENT_MISMATCH", stdout)
        os.close(master)
        os.close(slave)

    def test_ack_after_measurement_remains_review_ineligible(self):
        master, slave = pty.openpty()
        device = os.ttyname(slave)
        with tempfile.TemporaryDirectory() as temporary:
            output = Path(temporary) / "uart.log"
            metadata = Path(temporary) / "capture.json"
            process = self.run_capture(device, output, metadata)
            time.sleep(0.1)
            payload = (
                b"PANDORA STM32L475VE XINYI SMOKE OK\r\n"
                b"AHT10 RH_milli_percent=50234 T_milli_c=23125\r\n"
                b"AHT10 0x38 ACK\r\n"
            )
            os.write(master, payload)
            stdout, stderr = process.communicate(timeout=2)

            self.assertEqual(process.returncode, 3, stderr)
            record = json.loads(metadata.read_text(encoding="utf-8"))
            self.assertEqual(record["status"], "CAPTURE_CONTENT_MISMATCH")
            self.assertEqual(record["runtime_evidence"], "NONE")
            self.assertIn("ordered Pandora/AHT10 startup", record["error"])
            self.assertIn("CAPTURE_CONTENT_MISMATCH", stdout)
        os.close(master)
        os.close(slave)

    def test_nack_then_ack_and_measurement_is_b2_review_candidate(self):
        master, slave = pty.openpty()
        device = os.ttyname(slave)
        with tempfile.TemporaryDirectory() as temporary:
            output = Path(temporary) / "uart.log"
            metadata = Path(temporary) / "capture.json"
            process = self.run_capture(device, output, metadata)
            time.sleep(0.1)
            payload = (
                b"PANDORA STM32L475VE XINYI SMOKE OK\r\n"
                b"AHT10 0x38 NACK\r\n"
                b"AHT10 0x38 ACK\r\n"
                b"AHT10 RH_milli_percent=50234 T_milli_c=23125\r\n"
            )
            os.write(master, payload)
            stdout, stderr = process.communicate(timeout=2)

            self.assertEqual(process.returncode, 0, stderr)
            record = json.loads(metadata.read_text(encoding="utf-8"))
            self.assertEqual(record["status"], "CAPTURED")
            self.assertEqual(record["required_nack_marker"], "AHT10 0x38 NACK")
            self.assertEqual(record["runtime_evidence"], "B2_REVIEW_CANDIDATE")
            self.assertIn("CAPTURED", stdout)
        os.close(master)
        os.close(slave)

    def test_nack_before_firmware_banner_does_not_grant_b2_candidate(self):
        master, slave = pty.openpty()
        device = os.ttyname(slave)
        with tempfile.TemporaryDirectory() as temporary:
            output = Path(temporary) / "uart.log"
            metadata = Path(temporary) / "capture.json"
            process = self.run_capture(device, output, metadata)
            time.sleep(0.1)
            payload = (
                b"AHT10 0x38 NACK\r\n"
                b"PANDORA STM32L475VE XINYI SMOKE OK\r\n"
                b"AHT10 0x38 ACK\r\n"
                b"AHT10 RH_milli_percent=50234 T_milli_c=23125\r\n"
            )
            os.write(master, payload)
            stdout, stderr = process.communicate(timeout=2)

            self.assertEqual(process.returncode, 0, stderr)
            record = json.loads(metadata.read_text(encoding="utf-8"))
            self.assertEqual(record["status"], "CAPTURED")
            self.assertEqual(record["runtime_evidence"], "B1_REVIEW_CANDIDATE")
            self.assertIn("CAPTURED", stdout)
        os.close(master)
        os.close(slave)

    def test_nack_after_recovered_measurement_does_not_grant_b2_candidate(self):
        master, slave = pty.openpty()
        device = os.ttyname(slave)
        with tempfile.TemporaryDirectory() as temporary:
            output = Path(temporary) / "uart.log"
            metadata = Path(temporary) / "capture.json"
            process = self.run_capture(device, output, metadata)
            time.sleep(0.1)
            payload = (
                b"PANDORA STM32L475VE XINYI SMOKE OK\r\n"
                b"AHT10 0x38 ACK\r\n"
                b"AHT10 RH_milli_percent=50234 T_milli_c=23125\r\n"
                b"AHT10 0x38 NACK\r\n"
            )
            os.write(master, payload)
            stdout, stderr = process.communicate(timeout=2)

            self.assertEqual(process.returncode, 0, stderr)
            record = json.loads(metadata.read_text(encoding="utf-8"))
            self.assertEqual(record["status"], "CAPTURED")
            self.assertEqual(record["runtime_evidence"], "B1_REVIEW_CANDIDATE")
            self.assertIn("CAPTURED", stdout)
        os.close(master)
        os.close(slave)

    def test_recovery_after_an_earlier_successful_cycle_does_not_grant_b2_candidate(self):
        master, slave = pty.openpty()
        device = os.ttyname(slave)
        with tempfile.TemporaryDirectory() as temporary:
            output = Path(temporary) / "uart.log"
            metadata = Path(temporary) / "capture.json"
            process = self.run_capture(device, output, metadata)
            time.sleep(0.1)
            payload = (
                b"PANDORA STM32L475VE XINYI SMOKE OK\r\n"
                b"AHT10 0x38 ACK\r\n"
                b"AHT10 RH_milli_percent=50234 T_milli_c=23125\r\n"
                b"PANDORA STM32L475VE XINYI SMOKE OK\r\n"
                b"AHT10 0x38 NACK\r\n"
                b"PANDORA STM32L475VE XINYI SMOKE OK\r\n"
                b"AHT10 0x38 ACK\r\n"
                b"AHT10 RH_milli_percent=50100 T_milli_c=23000\r\n"
            )
            os.write(master, payload)
            stdout, stderr = process.communicate(timeout=2)

            self.assertEqual(process.returncode, 0, stderr)
            record = json.loads(metadata.read_text(encoding="utf-8"))
            self.assertEqual(record["status"], "CAPTURED")
            self.assertEqual(record["runtime_evidence"], "B1_REVIEW_CANDIDATE")
            self.assertIn("CAPTURED", stdout)
        os.close(master)
        os.close(slave)

    def test_no_data_timeout_is_bounded_and_fail_closed(self):
        master, slave = pty.openpty()
        device = os.ttyname(slave)
        with tempfile.TemporaryDirectory() as temporary:
            output = Path(temporary) / "uart.log"
            metadata = Path(temporary) / "capture.json"
            started = time.monotonic()
            process = self.run_capture(device, output, metadata, timeout=0.2)
            stdout, stderr = process.communicate(timeout=2)
            elapsed = time.monotonic() - started

            self.assertEqual(process.returncode, 2, stderr)
            self.assertLess(elapsed, 1.5)
            self.assertEqual(output.read_bytes(), b"")
            record = json.loads(metadata.read_text(encoding="utf-8"))
            self.assertEqual(record["status"], "NO_DATA_TIMEOUT")
            self.assertEqual(record["bytes_captured"], 0)
            self.assertIn("NO_DATA_TIMEOUT", stdout)
        os.close(master)
        os.close(slave)

    def test_device_disconnect_is_not_reported_as_no_data_timeout(self):
        master, slave = pty.openpty()
        device = os.ttyname(slave)
        with tempfile.TemporaryDirectory() as temporary:
            output = Path(temporary) / "uart.log"
            metadata = Path(temporary) / "capture.json"
            process = self.run_capture(device, output, metadata, timeout=1.0)
            time.sleep(0.1)
            os.close(master)
            stdout, stderr = process.communicate(timeout=2)

            self.assertEqual(process.returncode, 3, stderr)
            self.assertEqual(output.read_bytes(), b"")
            record = json.loads(metadata.read_text(encoding="utf-8"))
            self.assertEqual(record["status"], "CAPTURE_IO_FAILED")
            self.assertEqual(record["bytes_captured"], 0)
            self.assertIn("device returned EOF", record["error"])
            self.assertIn("CAPTURE_IO_FAILED", stdout)
        os.close(slave)

    def test_missing_device_fails_without_creating_false_runtime_evidence(self):
        with tempfile.TemporaryDirectory() as temporary:
            output = Path(temporary) / "uart.log"
            metadata = Path(temporary) / "capture.json"
            process = self.run_capture("/dev/xinyi-missing-uart", output, metadata, timeout=0.1)
            stdout, stderr = process.communicate(timeout=2)

            self.assertEqual(process.returncode, 3, stderr)
            self.assertFalse(output.exists())
            record = json.loads(metadata.read_text(encoding="utf-8"))
            self.assertEqual(record["status"], "DEVICE_OPEN_FAILED")
            self.assertEqual(record["bytes_captured"], 0)
            self.assertIn("DEVICE_OPEN_FAILED", stdout)

    def test_missing_firmware_commit_refuses_capture(self):
        master, slave = pty.openpty()
        with tempfile.TemporaryDirectory() as temporary:
            output = Path(temporary) / "uart.log"
            metadata = Path(temporary) / "capture.json"
            process = self.run_capture(
                os.ttyname(slave), output, metadata, timeout=0.1, firmware_commit=""
            )
            stdout, stderr = process.communicate(timeout=2)

            self.assertNotEqual(process.returncode, 0, stdout)
            self.assertIn("--firmware-commit", stderr)
            self.assertFalse(output.exists())
            self.assertFalse(metadata.exists())
        os.close(master)
        os.close(slave)

    def test_non_exact_firmware_commit_refuses_capture(self):
        master, slave = pty.openpty()
        with tempfile.TemporaryDirectory() as temporary:
            output = Path(temporary) / "uart.log"
            metadata = Path(temporary) / "capture.json"
            process = self.run_capture(
                os.ttyname(slave), output, metadata, timeout=0.1, firmware_commit="HEAD"
            )
            stdout, stderr = process.communicate(timeout=2)

            self.assertNotEqual(process.returncode, 0, stdout)
            self.assertIn("exact 40-character lowercase Git SHA", stderr)
            self.assertFalse(output.exists())
            self.assertFalse(metadata.exists())
        os.close(master)
        os.close(slave)


if __name__ == "__main__":
    unittest.main()
