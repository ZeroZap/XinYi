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


class PandoraUartCaptureTest(unittest.TestCase):
    def run_capture(self, device: str, output: Path, metadata: Path, timeout: float = 0.5):
        return subprocess.Popen(
            [
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
            ],
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
            payload = b"PANDORA STM32L475VE XINYI SMOKE OK\r\nAHT10 0x38 ACK\r\n"
            os.write(master, payload)
            stdout, stderr = process.communicate(timeout=2)

            self.assertEqual(process.returncode, 0, stderr)
            self.assertEqual(output.read_bytes(), payload)
            record = json.loads(metadata.read_text(encoding="utf-8"))
            self.assertEqual(record["status"], "CAPTURED")
            self.assertEqual(record["device"], device)
            self.assertEqual(record["bytes_captured"], len(payload))
            self.assertRegex(record["source_commit"], r"^[0-9a-f]{40}$")
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


if __name__ == "__main__":
    unittest.main()
