#!/usr/bin/env python3
"""Contracts for the Pandora runtime-timeline capture/analysis tool.

The tool exists because the runtime image cannot time itself without changing the marker
vocabulary that the other Pandora validators pin. These tests fix the measurement
semantics on synthetic timelines where the true wall times are known, and prove the
capture path against a real PTY pair instead of a mock.
"""

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
TOOL = ROOT / "boards" / "pandora_stm32l475" / "validate_runtime_timeline.py"
COMMIT = "0123456789abcdef0123456789abcdef01234567"
BANNER = "PANDORA STM32L475VE XINYI OSAL FREERTOS READY"
IDENTITY = f"FIRMWARE_COMMIT {COMMIT}"

MARKERS = (
    "OSAL_MUTEX_FAST",
    "OSAL_TASK_FAST",
    "OSAL_QUEUE_SEND",
    "OSAL_EVENT_SET",
    "OSAL_IPC_SEND",
    "OSAL_PM_TICK",
)


def build_lines(cycles: int, cycle_ms: float, delay_ms: float):
    """Reproduce the fast/slow task marker order with exact wall times."""
    lines = [
        (0.0, BANNER),
        (1.0, IDENTITY),
        (2.0, "OSAL_STRESS_READY"),
    ]
    now = 2.0
    for _ in range(cycles):
        line_ms = 1.7
        for marker in MARKERS:
            lines.append((round(now, 3), marker))
            now += line_ms
        lines.append((round(now, 3), "OSAL_SEM_TAKE"))
        lines.append((round(now + 1.0, 3), "OSAL_EVENT_WAIT"))
        lines.append((round(now + 2.0, 3), "OSAL_QUEUE_RECV"))
        lines.append((round(now + 3.0, 3), "OSAL_MUTEX_SLOW"))
        lines.append((round(now + 4.0, 3), "OSAL_TASK_SLOW"))
        # One OSAL_TASK_FAST -> OSAL_TASK_SLOW iteration spans the requested delay of
        # *this* iteration plus the logging of the next one, so pacing it on cycle_ms
        # keeps cycles and delay windows distinguishable in the fixtures.
        now += delay_ms
    return lines


def timeline_record(lines):
    return {
        "record_type": "xinyi-pandora-stm32l475-runtime-timeline-capture",
        "status": "CAPTURED",
        "device": "/dev/ttyUSB0",
        "requested_seconds": 70.0,
        "captured_bytes": 2048,
        "capture_sha256": "0" * 64,
        "captured_at_utc": "2026-10-29T00:00:00+00:00",
        "lines": [{"t_ms": t, "text": text} for t, text in lines],
    }


class RuntimeTimelineAnalysisTest(unittest.TestCase):
    def run_analyze(self, record, *extra):
        with tempfile.TemporaryDirectory() as temporary:
            timeline = Path(temporary) / "run.timeline.json"
            output = Path(temporary) / "run.record.json"
            timeline.write_text(json.dumps(record), encoding="utf-8")
            result = subprocess.run(
                [
                    sys.executable,
                    str(TOOL),
                    "analyze",
                    "--timeline",
                    str(timeline),
                    "--firmware-commit",
                    COMMIT,
                    "--record",
                    str(output),
                    *extra,
                ],
                capture_output=True,
                text=True,
            )
            if not output.exists():
                self.fail(f"analyze wrote no record: {result.stdout}{result.stderr}")
            written: dict = json.loads(output.read_text(encoding="utf-8"))
        return result, written

    def test_three_cycles_within_bound(self):
        result, record = self.run_analyze(timeline_record(build_lines(3, 520.0, 500.0)))
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertEqual(record["status"], "TIMELINE_MEASURED")
        self.assertEqual(record["verdict"], "CYCLE_WALL_WITHIN_BOUND")
        self.assertEqual(record["cycle_count"], 3)
        self.assertEqual(record["delay_window_count"], 2)
        self.assertLessEqual(record["slowest_cycle_ms"], 1500.0)
        self.assertEqual(record["failures"], [])

    def test_cycle_wall_exceeding_bound_still_measures(self):
        result, record = self.run_analyze(timeline_record(build_lines(2, 5000.0, 4000.0)))
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertEqual(record["status"], "TIMELINE_MEASURED")
        self.assertEqual(record["verdict"], "CYCLE_WALL_EXCEEDS_BOUND")
        self.assertGreater(record["slowest_cycle_ms"], 1500.0)

    def test_delay_window_excess_is_computed(self):
        result, record = self.run_analyze(timeline_record(build_lines(3, 520.0, 4000.0)))
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertGreaterEqual(record["worst_delay_excess_ms"], 3000.0)
        window = record["delay_windows"][0]
        self.assertEqual(window["requested_ms"], 500.0)
        self.assertAlmostEqual(window["excess_ms"], window["wall_ms"] - 500.0, places=3)

    def test_insufficient_cycles_is_rejected(self):
        result, record = self.run_analyze(timeline_record(build_lines(1, 520.0, 500.0)))
        self.assertEqual(result.returncode, 1)
        self.assertEqual(record["status"], "REJECTED")
        self.assertTrue(any("measured cycles" in failure for failure in record["failures"]))

    def test_identity_mismatch_is_rejected(self):
        lines = [(t, "FIRMWARE_COMMIT " + "f" * 40 if text == IDENTITY else text)
                 for t, text in build_lines(3, 520.0, 500.0)]
        result, record = self.run_analyze(timeline_record(lines))
        self.assertEqual(result.returncode, 1)
        self.assertTrue(
            any("firmware identity" in failure for failure in record["failures"]),
            record["failures"],
        )

    def test_non_monotonic_timestamps_are_rejected(self):
        lines = build_lines(3, 520.0, 500.0)
        lines[5] = (lines[5][0] - 50.0, lines[5][1])
        result, record = self.run_analyze(timeline_record(lines))
        self.assertEqual(result.returncode, 1)
        self.assertTrue(
            any("monotonic" in failure for failure in record["failures"]), record["failures"]
        )

    def test_empty_capture_is_rejected(self):
        record_in = timeline_record([])
        record_in["status"] = "NO_DATA_TIMEOUT"
        result, record = self.run_analyze(record_in)
        self.assertEqual(result.returncode, 1)
        self.assertEqual(record["status"], "REJECTED")

    def test_largest_gap_is_ranked_first(self):
        lines = [(0.0, BANNER), (1.0, IDENTITY)]
        now = 1.0
        for index in range(3):
            for marker in MARKERS:
                lines.append((round(now, 3), marker))
                now += 1.7
            lines.append((round(now, 3), "OSAL_TASK_SLOW"))
            now += 60000.0 if index == 1 else 500.0
        result, record = self.run_analyze(timeline_record(lines))
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertEqual(record["largest_gaps"][0]["gap_ms"], 60000.0)
        self.assertEqual(record["largest_gaps"][0]["after"], "OSAL_TASK_SLOW")

    def test_custom_bounds_are_honoured(self):
        result, record = self.run_analyze(
            timeline_record(build_lines(2, 520.0, 500.0)), "--max-cycle-ms", "100"
        )
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertEqual(record["verdict"], "CYCLE_WALL_EXCEEDS_BOUND")
        self.assertEqual(record["max_cycle_ms"], 100.0)


class RuntimeTimelineCaptureTest(unittest.TestCase):
    def test_missing_device_is_rejected_without_a_timeline_claim(self):
        with tempfile.TemporaryDirectory() as temporary:
            output = Path(temporary) / "run.log"
            timeline = Path(temporary) / "run.timeline.json"
            result = subprocess.run(
                [
                    sys.executable,
                    str(TOOL),
                    "capture",
                    "--device",
                    str(Path(temporary) / "absent-tty"),
                    "--seconds",
                    "0.2",
                    "--output",
                    str(output),
                    "--timeline",
                    str(timeline),
                ],
                capture_output=True,
                text=True,
            )
            self.assertEqual(result.returncode, 2, result.stdout + result.stderr)
            record = json.loads(timeline.read_text(encoding="utf-8"))
            self.assertEqual(record["status"], "REJECTED")
            self.assertIn("device open failed", record["error"])

    def test_pty_capture_records_monotonic_arrival_offsets(self):
        master, slave = pty.openpty()
        device = os.ttyname(slave)
        try:
            with tempfile.TemporaryDirectory() as temporary:
                output = Path(temporary) / "run.log"
                timeline = Path(temporary) / "run.timeline.json"
                process = subprocess.Popen(
                    [
                        sys.executable,
                        str(TOOL),
                        "capture",
                        "--device",
                        device,
                        "--seconds",
                        "1.5",
                        "--output",
                        str(output),
                        "--timeline",
                        str(timeline),
                    ],
                    stdout=subprocess.PIPE,
                    stderr=subprocess.PIPE,
                    text=True,
                )
                try:
                    # The capture process configures and flushes the port first; writing
                    # earlier would be discarded by that flush.
                    time.sleep(0.35)
                    for line in (BANNER, IDENTITY, "OSAL_TASK_FAST", "OSAL_TASK_SLOW"):
                        os.write(master, (line + "\r\n").encode("ascii"))
                        time.sleep(0.05)
                    stdout, stderr = process.communicate(timeout=10)
                finally:
                    if process.poll() is None:
                        process.kill()
                        process.communicate()
                self.assertEqual(process.returncode, 0, stdout + stderr)
                record = json.loads(timeline.read_text(encoding="utf-8"))
                self.assertEqual(record["status"], "CAPTURED")
                self.assertTrue(record["banner_seen"])
                self.assertEqual(
                    [line["text"] for line in record["lines"]],
                    [BANNER, IDENTITY, "OSAL_TASK_FAST", "OSAL_TASK_SLOW"],
                )
                stamps = [line["t_ms"] for line in record["lines"]]
                self.assertEqual(stamps, sorted(stamps))
                self.assertGreater(stamps[-1], stamps[0])
                self.assertEqual(output.read_bytes()[: len(BANNER)], BANNER.encode("ascii"))
        finally:
            os.close(master)
            os.close(slave)


if __name__ == "__main__":
    unittest.main()
