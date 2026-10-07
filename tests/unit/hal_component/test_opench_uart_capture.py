#!/usr/bin/env python3
"""Host contract for identity-bound openCH UART evidence capture."""

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
CAPTURE = ROOT / "boards" / "opench_ch32v307" / "capture_uart.py"
COMMIT = "0123456789abcdef0123456789abcdef01234567"


class OpenChUartCaptureTest(unittest.TestCase):
    def run_capture(self, device: str, output: Path, metadata: Path):
        return subprocess.Popen(
            [sys.executable, str(CAPTURE), device, "--firmware-commit", COMMIT,
             "--output", str(output), "--metadata", str(metadata), "--duration-ms", "300"],
            cwd=ROOT, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)

    def exercise(self, payload: bytes):
        master, slave = pty.openpty()
        with tempfile.TemporaryDirectory() as temporary:
            output = Path(temporary) / "uart.log"
            metadata = Path(temporary) / "capture.json"
            process = self.run_capture(os.ttyname(slave), output, metadata)
            time.sleep(0.2)
            if payload:
                os.write(master, payload)
            stdout, stderr = process.communicate(timeout=2)
            record = json.loads(metadata.read_text(encoding="utf-8"))
            captured = output.read_bytes()
        os.close(master)
        os.close(slave)
        return process.returncode, stdout, stderr, record, captured

    def test_accepts_exact_identity_and_ordered_runtime_chain(self):
        payload = (
            b"startup noise\r\nXINYI OPENCH CH32V307 UART1 READY\r\n"
            b"FIRMWARE_COMMIT " + COMMIT.encode() + b"\r\n"
            b"PA9=UART1_TX PA10=UART1_RX WCHLINK=CH549F\r\n"
            b"OPENCH_UART1_ALIVE\r\n"
        )
        code, stdout, stderr, record, captured = self.exercise(payload)
        self.assertEqual(code, 0, stderr)
        self.assertEqual(record["status"], "B1_REVIEW_CANDIDATE")
        self.assertEqual(record["firmware_commit"], COMMIT)
        self.assertEqual(captured, payload)
        self.assertIn("B1_REVIEW_CANDIDATE", stdout)

    def test_rejects_wrong_identity(self):
        payload = (
            b"XINYI OPENCH CH32V307 UART1 READY\r\n"
            b"FIRMWARE_COMMIT ffffffffffffffffffffffffffffffffffffffff\r\n"
            b"PA9=UART1_TX PA10=UART1_RX WCHLINK=CH549F\r\nOPENCH_UART1_ALIVE\r\n"
        )
        code, _, _, record, _ = self.exercise(payload)
        self.assertEqual(code, 1)
        self.assertEqual(record["status"], "CONTENT_MISMATCH")

    def test_rejects_out_of_order_markers(self):
        payload = (
            b"OPENCH_UART1_ALIVE\r\nXINYI OPENCH CH32V307 UART1 READY\r\n"
            b"FIRMWARE_COMMIT " + COMMIT.encode() + b"\r\n"
            b"PA9=UART1_TX PA10=UART1_RX WCHLINK=CH549F\r\n"
        )
        code, _, _, record, _ = self.exercise(payload)
        self.assertEqual(code, 1)
        self.assertEqual(record["status"], "CONTENT_MISMATCH")

    def test_skips_truncated_cycle_and_accepts_next_complete_cycle(self):
        payload = (
            b"XINYI OPENCH CH32V307 UART1 READY\r\nFIRMWARE_COMMIT truncated\r\n"
            b"PA9=UART1_TX PA10=UART1_RX WCHLINK=CH549F\r\nOPENCH_UART1_ALIVE\r\n"
            b"XINYI OPENCH CH32V307 UART1 READY\r\n"
            b"FIRMWARE_COMMIT " + COMMIT.encode() + b"\r\n"
            b"PA9=UART1_TX PA10=UART1_RX WCHLINK=CH549F\r\nOPENCH_UART1_ALIVE\r\n"
        )
        code, _, stderr, record, _ = self.exercise(payload)
        self.assertEqual(code, 0, stderr)
        self.assertEqual(record["status"], "B1_REVIEW_CANDIDATE")


if __name__ == "__main__":
    unittest.main()
