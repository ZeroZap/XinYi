#!/usr/bin/env python3
"""Contract tests for the receive-overflow sweep's verdict.

The sweep is the only thing that says where the ring actually runs out. Its verdict has
to fail when the sweep proves nothing, otherwise the table is just numbers with a green
tick next to them.
"""

from __future__ import annotations

import sys
import unittest
from pathlib import Path

BOARD = Path(__file__).resolve().parents[3] / "boards" / "opench_ch32v307"
sys.path.insert(0, str(BOARD))

import sweep_rx_overflow as sweep  # noqa: E402


def row(period: int, overflow: int) -> tuple[int, dict[str, int]]:
    return (period, {"xy_probe_rx_overflow": overflow, "xy_probe_rx_irq_bytes": 1,
                     "xy_probe_rx_frames_ok": 1, "xy_probe_rx_frames_bad": 0})


class SweepVerdictTest(unittest.TestCase):
    def test_a_bracketed_threshold_passes(self) -> None:
        code, lines = sweep.verdict([row(2, 0), row(20, 0), row(30, 6086), row(50, 13388)])
        self.assertEqual(code, 0)
        self.assertTrue(any("overflow at 30 ms, clean at 20 ms" in line for line in lines), lines)

    def test_a_sweep_that_never_overflows_fails(self) -> None:
        # Clean everywhere means the sweep never reached the threshold; reporting that as
        # a pass would claim the ring is larger than anything tested.
        code, lines = sweep.verdict([row(2, 0), row(5, 0), row(10, 0)])
        self.assertEqual(code, 1)
        self.assertTrue(any("proved nothing" in line for line in lines), lines)

    def test_a_sweep_that_only_overflows_fails(self) -> None:
        code, lines = sweep.verdict([row(20, 5), row(30, 10)])
        self.assertEqual(code, 1)
        self.assertTrue(any("harness is the bottleneck" in line for line in lines), lines)

    def test_a_single_row_cannot_bracket(self) -> None:
        self.assertEqual(sweep.verdict([row(20, 0)])[0], 1)
        self.assertEqual(sweep.verdict([row(20, 3)])[0], 1)


if __name__ == "__main__":
    unittest.main()
