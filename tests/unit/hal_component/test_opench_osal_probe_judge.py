#!/usr/bin/env python3
"""Pin the openCH probe judgement: a capture that should not pass must not pass.

The validator reads RAM through the debugger, which makes the interesting part the
judging rather than the reading. Two of the checks only became meaningful once the
kernel owned the millisecond clock: the UART burst has to cost the wire time, and a
receive on a silent line has to spend its timeout and say so.
"""

from __future__ import annotations

import sys
import unittest
from pathlib import Path

BOARD = Path(__file__).resolve().parents[3] / "boards" / "opench_ch32v307"
sys.path.insert(0, str(BOARD))

import validate_osal_probe as probe  # noqa: E402


def good_words() -> dict[str, int]:
    return {
        "xy_probe_stage": probe.STAGE_DONE,
        "xy_probe_uart_burst_ms": 22,
        "xy_probe_uart_recv_ms": 10,
        "xy_probe_uart_recv_code": probe.RECV_TIMEOUT_CODE,
        "xy_probe_loops": probe.LOOPS_EXPECTED,
        "xy_probe_delay_ret": 0,
        "xy_probe_wait_ret": probe.EVENT_FLAGS_ERROR,
        "xy_probe_get_ret": probe.MSGQUEUE_TIMEOUT,
        "xy_probe_i2c_ops": 4,
        "xy_probe_i2c_fails": 0,
        "xy_probe_whoami": probe.WHOAMI_EXPECTED,
        "xy_probe_uart_sends": 6,
        "xy_probe_uart_fails": 0,
        "xTickCount": 100,
    }


def ticking(ticks: int = 120) -> dict[str, int]:
    """The second snapshot. A good run shows the tick count strictly larger."""
    return {"xTickCount": ticks}


class ProbeJudgeTests(unittest.TestCase):
    def test_a_complete_run_passes(self) -> None:
        self.assertEqual(probe.judge(good_words(), ticking()), [])

    def test_a_receive_that_returns_at_once_is_rejected(self) -> None:
        # The saw-tooth timebase fired every wait on the first loop, so this is
        # exactly the shape a regression would take.
        words = good_words()
        words["xy_probe_uart_recv_ms"] = 0
        errors = probe.judge(words, ticking())
        self.assertTrue(any("xy_probe_uart_recv_ms" in e for e in errors), errors)

    def test_a_receive_that_claims_success_is_rejected(self) -> None:
        words = good_words()
        words["xy_probe_uart_recv_code"] = 0
        errors = probe.judge(words, ticking())
        self.assertTrue(any("XY_HAL_ERROR_TIMEOUT" in e for e in errors), errors)

    def test_a_receive_that_never_gives_up_is_rejected(self) -> None:
        words = good_words()
        words["xy_probe_uart_recv_ms"] = probe.RECV_MS_MAX + 1
        errors = probe.judge(words, ticking())
        self.assertTrue(any("xy_probe_uart_recv_ms" in e for e in errors), errors)

    def test_a_frozen_burst_reading_is_rejected(self) -> None:
        words = good_words()
        words["xy_probe_uart_burst_ms"] = 0
        errors = probe.judge(words, ticking())
        self.assertTrue(any("xy_probe_uart_burst_ms" in e for e in errors), errors)

    def test_a_stale_image_still_at_the_old_stage_is_rejected(self) -> None:
        words = good_words()
        words["xy_probe_stage"] = 11
        errors = probe.judge(words, ticking())
        self.assertTrue(any("stage is" in e for e in errors), errors)

    def test_a_tick_that_stops_advancing_is_rejected(self) -> None:
        errors = probe.judge(good_words(), {"xTickCount": 100})
        self.assertEqual(errors, ["xTickCount did not advance: 100 -> 100"])

    def test_the_receive_window_stays_tight_enough_to_be_meaningful(self) -> None:
        # A 10 ms receive is asked for; a window that accepted anything from 0 up
        # would not distinguish "gave up" from "returned immediately".
        self.assertGreater(probe.RECV_MS_MIN, 0)
        self.assertLess(probe.RECV_MS_MAX, 1000)
        self.assertEqual(probe.RECV_TIMEOUT_CODE, 0xFFFFFFFD)


if __name__ == "__main__":
    unittest.main()
