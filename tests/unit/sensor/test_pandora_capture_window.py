#!/usr/bin/env python3
"""Fail-closed contracts for the generic Pandora UART capture window.

The tool exists because two validators only judge a file they are handed, while the
images they judge print their banner immediately after reset: the port has to be open
and flushed before the reset. These tests hold that ordering, the flush, and the exit
codes, using a pty pair so no hardware is involved.
"""

from pathlib import Path
import json
import os
import subprocess
import sys
import tempfile
import threading
import time
import unittest

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "boards" / "pandora_stm32l475"))

import capture_window  # noqa: E402

BANNER = b"PANDORA AP3216C SENSOR READY\r\nFIRMWARE_COMMIT " + b"1" * 40 + b"\r\n"
STALE = b"STALE-BYTES-FROM-BEFORE-THE-RESET\r\n"


def write_after_delay(fd: int, payload: bytes, delay: float = 0.2) -> threading.Thread:
    def worker() -> None:
        time.sleep(delay)
        try:
            os.write(fd, payload)
        except OSError:
            pass

    thread = threading.Thread(target=worker, daemon=True)
    thread.start()
    return thread


class CaptureWindowTests(unittest.TestCase):
    def setUp(self) -> None:
        self._master, self._slave = os.openpty()
        self.device = os.ttyname(self._slave)
        self._tmp = tempfile.TemporaryDirectory()
        self.tmp = Path(self._tmp.name)

    def tearDown(self) -> None:
        os.close(self._master)
        os.close(self._slave)
        self._tmp.cleanup()

    def test_captures_banner_after_reset_and_drops_pre_reset_bytes(self) -> None:
        # Bytes sitting in the buffer before the port is opened must not be judged as part
        # of the window: the window starts at the reset.
        os.write(self._master, STALE)
        write_after_delay(self._master, BANNER)
        status, payload, error = capture_window.capture_window(self.device, 1.0, None)
        self.assertEqual(status, "CAPTURED")
        self.assertIsNone(error)
        self.assertIn(BANNER, payload)
        self.assertNotIn(STALE, payload)

    def test_port_is_flushed_before_the_reset_runs(self) -> None:
        calls: list[str] = []

        def opener(device: str, baudrate: int) -> int:
            calls.append("open")
            return capture_window.open_serial(device, baudrate)

        def runner(command, check, capture_output):  # noqa: ANN001, ARG001
            calls.append("reset")
            return subprocess.CompletedProcess(command, 0, b"", b"")

        write_after_delay(self._master, BANNER, delay=0.3)
        status, payload, error = capture_window.capture_window(
            self.device, 1.0, ["st-flash", "reset"], opener=opener, runner=runner
        )
        self.assertEqual(status, "CAPTURED", error)
        self.assertEqual(calls, ["open", "reset"])
        self.assertIn(BANNER, payload)

    def test_no_data_is_a_failure_and_still_writes_the_evidence_record(self) -> None:
        output = self.tmp / "capture.log"
        metadata = self.tmp / "capture.json"
        code = capture_window.main(
            ["--device", self.device, "--seconds", "0.3", "--output", str(output),
             "--metadata", str(metadata)]
        )
        self.assertEqual(code, 2)
        record = json.loads(metadata.read_text(encoding="utf-8"))
        self.assertEqual(record["capture_status"], "NO_DATA_TIMEOUT")
        self.assertEqual(record["bytes_captured"], 0)
        self.assertEqual(output.read_bytes(), b"")

    def test_missing_device_refuses_without_writing_a_capture(self) -> None:
        output = self.tmp / "capture.log"
        metadata = self.tmp / "capture.json"
        code = capture_window.main(
            ["--device", str(self.tmp / "absent"), "--seconds", "0.2",
             "--output", str(output), "--metadata", str(metadata)]
        )
        self.assertEqual(code, 3)
        record = json.loads(metadata.read_text(encoding="utf-8"))
        self.assertEqual(record["capture_status"], "DEVICE_OPEN_FAILED")
        self.assertFalse(output.exists())

    def test_a_failed_reset_command_is_not_a_silent_success(self) -> None:
        def runner(command, check, capture_output):  # noqa: ANN001, ARG001
            raise subprocess.CalledProcessError(1, command)

        status, _, error = capture_window.capture_window(
            self.device, 0.2, ["st-flash", "reset"], runner=runner
        )
        self.assertEqual(status, "RESET_FAILED")
        self.assertIn("returned non-zero", error or "")

    def test_successful_cli_run_records_hash_matching_the_file(self) -> None:
        write_after_delay(self._master, BANNER, delay=0.2)
        output = self.tmp / "capture.log"
        metadata = self.tmp / "capture.json"
        code = capture_window.main(
            ["--device", self.device, "--seconds", "1.0", "--output", str(output),
             "--metadata", str(metadata)]
        )
        self.assertEqual(code, 0)
        record = json.loads(metadata.read_text(encoding="utf-8"))
        self.assertEqual(record["capture_status"], "CAPTURED")
        self.assertEqual(record["bytes_captured"], len(output.read_bytes()))
        self.assertEqual(len(record["sha256"]), 64)
        self.assertIn(BANNER[:20], output.read_bytes())


if __name__ == "__main__":
    unittest.main()
