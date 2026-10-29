#!/usr/bin/env python3
"""Contract tests for the soak verdict.

The soak prints zeros when it passes and zeros when nobody looked. These tests make the
difference explicit: each case below is a failure shape the verdict has to name.
"""

from __future__ import annotations

import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[3] / "boards" / "opench_ch32v307"))

import soak_rx  # noqa: E402


def clean(frames: int) -> dict[str, int]:
    return {
        "xy_probe_rx_irq_bytes": frames * 8,
        "xy_probe_rx_frames_ok": frames,
        "xy_probe_rx_frames_bad": 0,
        "xy_probe_rx_overflow": 0,
        "xy_probe_rx_ore": 0,
    }


class SoakVerdictTest(unittest.TestCase):
    def test_a_clean_run_has_nothing_to_report(self) -> None:
        self.assertEqual(soak_rx.verdict(2000, clean(2000)), [])

    def test_a_broken_frame_fails_even_when_the_byte_count_matches(self) -> None:
        words = clean(2000)
        words["xy_probe_rx_frames_bad"] = 1
        errors = soak_rx.verdict(2000, words)
        self.assertTrue(any("broke continuity" in error for error in errors))

    def test_a_ring_overflow_fails(self) -> None:
        words = clean(2000)
        words["xy_probe_rx_overflow"] = 35
        errors = soak_rx.verdict(2000, words)
        self.assertTrue(any("overflowed" in error for error in errors))

    def test_a_receiver_overrun_fails(self) -> None:
        words = clean(2000)
        words["xy_probe_rx_ore"] = 1
        errors = soak_rx.verdict(2000, words)
        self.assertTrue(any("overran" in error for error in errors))

    def test_missing_bytes_fail(self) -> None:
        words = clean(2000)
        words["xy_probe_rx_irq_bytes"] = 2000 * 8 - 8
        errors = soak_rx.verdict(2000, words)
        self.assertTrue(any("bytes sent but" in error for error in errors))

    def test_a_lost_frame_fails_even_when_bytes_arrive(self) -> None:
        # Bytes in but frames missing: the parser swallowed them, which a byte counter
        # alone would call healthy.
        words = clean(2000)
        words["xy_probe_rx_frames_ok"] = 1999
        errors = soak_rx.verdict(2000, words)
        self.assertTrue(any("were intact" in error for error in errors))

    def test_extra_frames_fail_too(self) -> None:
        # More frames than were sent would mean the decoder invented one.
        words = clean(2000)
        words["xy_probe_rx_frames_ok"] = 2001
        errors = soak_rx.verdict(2000, words)
        self.assertTrue(any("were intact" in error for error in errors))


if __name__ == "__main__":
    unittest.main()
