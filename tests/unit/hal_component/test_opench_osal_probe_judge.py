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
        "xy_probe_i2c_nak_code": probe.I2C_NOT_FOUND_CODE,
        "xy_probe_i2c_recover_code": 0,
        "xy_probe_i2c_recover_value": probe.I2C_RECOVER_VALUE,
        "xy_probe_rx_irq_bytes": 10,
        "xy_probe_rx_frames_ok": 5,
        "xy_probe_rx_frames_bad": 0,
        "xy_probe_rx_overflow": 0,
        "xy_probe_rx_ore": 0,
        "xTickCount": 100,
    }


def ticking(ticks: int = 120, rx: int = 20) -> dict[str, int]:
    """The second snapshot: a good run shows the tick count and the receive count
    strictly larger, because both are supposed to still be moving."""
    return {"xTickCount": ticks, "xy_probe_rx_irq_bytes": rx,
            "xy_probe_rx_frames_ok": rx // 2}


# The receive-interrupt judgement only exists when a writer supplied bytes.
UART = "/dev/ttyACM0"


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

    def test_a_silent_receive_interrupt_is_rejected(self) -> None:
        words = good_words()
        words["xy_probe_rx_irq_bytes"] = 0
        errors = probe.judge(words, ticking(rx=0), UART)
        self.assertTrue(any("no byte arrived" in e for e in errors), errors)

    def test_a_receive_interrupt_that_stopped_is_rejected(self) -> None:
        # Same total twice: the value is stale, which is not the same as alive.
        errors = probe.judge(good_words(), ticking(rx=10), UART)
        self.assertTrue(any("stopped" in e for e in errors), errors)

    def test_the_receive_judgement_is_skipped_without_a_writer(self) -> None:
        words = good_words()
        words["xy_probe_rx_irq_bytes"] = 0
        self.assertEqual(probe.judge(words, ticking(rx=0), None), [])

    def test_a_lost_byte_is_rejected_even_though_bytes_still_arrive(self) -> None:
        # The whole point of framed counting: the byte counter looks fine.
        words = good_words()
        words["xy_probe_rx_frames_bad"] = 3
        errors = probe.judge_integrity(words)
        self.assertTrue(any("continuity" in e for e in errors), errors)

    def test_a_ring_overflow_is_rejected(self) -> None:
        words = good_words()
        words["xy_probe_rx_overflow"] = 2
        errors = probe.judge_integrity(words)
        self.assertTrue(any("overflowed" in e for e in errors), errors)

    def test_a_receiver_overrun_is_rejected(self) -> None:
        words = good_words()
        words["xy_probe_rx_ore"] = 1
        errors = probe.judge_integrity(words)
        self.assertTrue(any("overran" in e for e in errors), errors)

    def test_no_complete_frame_is_rejected(self) -> None:
        words = good_words()
        words["xy_probe_rx_frames_ok"] = 0
        errors = probe.judge_integrity(words)
        self.assertTrue(any("no complete frame" in e for e in errors), errors)

    def test_a_clean_quiet_window_passes(self) -> None:
        self.assertEqual(probe.judge_integrity(good_words()), [])

    def test_continuity_is_not_judged_across_a_debugger_halt(self) -> None:
        # Reading memory halts the core and the hardware loses whatever arrives then, so
        # breaks in the growth snapshots must not be read as firmware loss - that is what
        # the quiet window is for.
        words = good_words()
        words["xy_probe_rx_frames_bad"] = 4
        self.assertEqual(probe.judge(words, ticking(), UART), [])

    def test_a_nak_reported_as_success_is_rejected(self) -> None:
        words = good_words()
        words["xy_probe_i2c_nak_code"] = 0
        errors = probe.judge(words, ticking())
        self.assertTrue(any("unanswered I2C address" in e for e in errors), errors)

    def test_a_bus_that_stays_stuck_after_a_nak_is_rejected(self) -> None:
        # The field failure: AF latched, every later transfer fails.
        words = good_words()
        words["xy_probe_i2c_recover_code"] = 0xFFFFFFFD
        errors = probe.judge(words, ticking())
        self.assertTrue(any("bus stayed stuck" in e for e in errors), errors)

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

    def test_the_reporting_path_survives_a_good_run(self) -> None:
        # Extracting judge() once left main()'s summary printing locals that no
        # longer existed, and nothing noticed because only judge() was tested. Run
        # main() against stubbed reads so the whole path has to execute.
        import contextlib
        import io

        snapshots = iter([dict(good_words()), dict(good_words(), xTickCount=140,
                                                  xy_probe_rx_irq_bytes=40)])
        originals = (probe.symbol_addresses, probe.read_words, probe.subprocess.run)
        probe.symbol_addresses = lambda nm, elf: {"xy_probe_stage": 0x20000000}
        probe.read_words = lambda *a, **k: next(snapshots)
        probe.subprocess.run = lambda *a, **k: None
        argv = sys.argv
        sys.argv = ["validate_osal_probe.py", "--openocd", "oc", "--config", "cfg",
                    "--elf", "elf", "--nm", "nm", "--settle-ms", "0"]
        out = io.StringIO()
        try:
            with contextlib.redirect_stdout(out):
                code = probe.main()
        finally:
            probe.symbol_addresses, probe.read_words, probe.subprocess.run = originals
            sys.argv = argv
        self.assertEqual(code, 0, out.getvalue())
        self.assertIn("osal_stall_probe_ok", out.getvalue())
        self.assertIn("nak=0xfffffffa", out.getvalue())

    def test_the_receive_window_stays_tight_enough_to_be_meaningful(self) -> None:
        # A 10 ms receive is asked for; a window that accepted anything from 0 up
        # would not distinguish "gave up" from "returned immediately".
        self.assertGreater(probe.RECV_MS_MIN, 0)
        self.assertLess(probe.RECV_MS_MAX, 1000)
        self.assertEqual(probe.RECV_TIMEOUT_CODE, 0xFFFFFFFD)


if __name__ == "__main__":
    unittest.main()
