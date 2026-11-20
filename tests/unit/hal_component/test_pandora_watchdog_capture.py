#!/usr/bin/env python3
"""Fail-closed contracts for the Pandora watchdog refresh-policy capture validator.

The validator is the thing that turns board bytes into a verdict, so the tests
here are about the validator itself: every rejection rule has to be shown able to
fire. A rule that no test exercises is a rule that has never been observed to
reject anything, which is indistinguishable from a rule that inspects nothing.
"""

from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "boards" / "pandora_stm32l475"))

from validate_watchdog_capture import analyze_capture  # noqa: E402

COMMIT = "1" * 40
BANNER = "PANDORA WATCHDOG REFRESH READY"
TERMINAL = "WATCHDOG_REFRESH_POLICY_PASS"
WINDOW_MS = 125
INTERVAL_MS = 40
CYCLES = 40


def feed_boot_lines() -> list[str]:
    lines = [
        BANNER,
        f"FIRMWARE_COMMIT {COMMIT}",
        "SYS_RESET_KIND POWER_ON",
        "WATCHDOG_PHASE FEED",
        f"WATCHDOG_ARMED nominal_window_ms={WINDOW_MS} feed_interval_ms={INTERVAL_MS} "
        f"cycles={CYCLES}",
    ]
    for cycle in range(1, CYCLES + 1):
        lines.append(f"WATCHDOG_FEED cycle={cycle} elapsed_ms={cycle * INTERVAL_MS}")
    lines.extend(
        [
            f"WATCHDOG_REFRESH_SURVIVED cycles={CYCLES} elapsed_ms={CYCLES * INTERVAL_MS}",
            "WATCHDOG_MARKER_SET",
            "WATCHDOG_FEED_STOPPED",
        ]
    )
    return lines


def report_boot_lines() -> list[str]:
    return [
        BANNER,
        f"FIRMWARE_COMMIT {COMMIT}",
        "SYS_RESET_KIND WATCHDOG",
        "WATCHDOG_PHASE REPORT",
        "WATCHDOG_EXPIRY_RESET_OK",
        TERMINAL,
    ]


def valid_capture() -> bytes:
    """A capture of a run that fed the watchdog, survived, then died of it."""
    lines = feed_boot_lines() + report_boot_lines()
    return ("\r\n".join(lines) + "\r\n").encode()


class PandoraWatchdogCaptureContract(unittest.TestCase):
    def test_accepts_identity_bound_pass_capture(self) -> None:
        result = analyze_capture(valid_capture(), COMMIT)

        self.assertEqual(result["status"], "B1_WATCHDOG_REFRESH_POLICY_PASS")
        self.assertEqual(result["boots_seen"], 2)
        self.assertEqual(result["error_steps"], [])
        self.assertEqual(result["failures"], [])

    def test_rejects_a_capture_from_another_firmware(self) -> None:
        result = analyze_capture(valid_capture().replace(COMMIT.encode(), b"2" * 40), COMMIT)

        self.assertEqual(result["status"], "WATCHDOG_VALIDATION_FAILED")
        self.assertIn(
            "missing or out-of-order exact firmware identity in the feed phase",
            result["failures"],
        )

    def test_rejects_a_capture_without_the_report_phase(self) -> None:
        """The negative control: a capture that only shows survival proves nothing,
        because an inert watchdog would produce the same bytes."""
        lines = feed_boot_lines()
        result = analyze_capture(("\r\n".join(lines) + "\r\n").encode(), COMMIT)

        self.assertEqual(result["status"], "WATCHDOG_VALIDATION_FAILED")
        self.assertIn("missing report phase marker: WATCHDOG_PHASE REPORT", result["failures"])

    def test_rejects_a_report_phase_naming_the_wrong_reset_cause(self) -> None:
        lines = feed_boot_lines() + [
            line.replace("SYS_RESET_KIND WATCHDOG", "SYS_RESET_KIND POWER_ON")
            for line in report_boot_lines()
        ]
        result = analyze_capture(("\r\n".join(lines) + "\r\n").encode(), COMMIT)

        self.assertEqual(result["status"], "WATCHDOG_VALIDATION_FAILED")
        self.assertTrue(
            any("did not die of a watchdog reset" in failure for failure in result["failures"]),
            result["failures"],
        )

    def test_rejects_a_survival_shorter_than_the_armed_window(self) -> None:
        """A board that did not even outlive one window was not kept alive by the
        refreshes."""
        lines = feed_boot_lines()
        lines = [
            line.replace(
                f"elapsed_ms={CYCLES * INTERVAL_MS}", f"elapsed_ms={WINDOW_MS // 2}"
            )
            for line in lines
        ]
        result = analyze_capture(("\r\n".join(lines) + "\r\n").encode(), COMMIT)

        self.assertEqual(result["status"], "WATCHDOG_VALIDATION_FAILED")
        self.assertTrue(
            any("shorter than the armed window" in failure for failure in result["failures"]),
            result["failures"],
        )

    def test_rejects_a_feed_interval_at_or_beyond_the_window(self) -> None:
        """A feed interval that is not shorter than the window races the expiry
        instead of keeping the board alive, so it proves nothing."""
        lines = [
            line.replace(
                f"feed_interval_ms={INTERVAL_MS}", f"feed_interval_ms={WINDOW_MS}"
            )
            for line in feed_boot_lines()
        ] + report_boot_lines()
        result = analyze_capture(("\r\n".join(lines) + "\r\n").encode(), COMMIT)

        self.assertEqual(result["status"], "WATCHDOG_VALIDATION_FAILED")
        self.assertTrue(
            any("not shorter than the armed window" in failure for failure in result["failures"]),
            result["failures"],
        )

    def test_rejects_a_missing_or_reordered_feed_marker(self) -> None:
        """The armed marker declares the cycle count, so a capture that shows
        fewer feed markers is a truncated run rather than a shorter one."""
        lines = feed_boot_lines()
        first_feed = next(
            index for index, line in enumerate(lines) if line.startswith("WATCHDOG_FEED cycle=")
        )
        dropped = lines[:first_feed] + lines[first_feed + 1 :]
        result = analyze_capture(
            ("\r\n".join(dropped + report_boot_lines()) + "\r\n").encode(), COMMIT
        )

        self.assertEqual(result["status"], "WATCHDOG_VALIDATION_FAILED")
        self.assertTrue(
            any("the armed marker declared" in failure for failure in result["failures"]),
            result["failures"],
        )

    def test_rejects_repeated_feed_markers(self) -> None:
        lines = feed_boot_lines()
        first_feed = next(
            index for index, line in enumerate(lines) if line.startswith("WATCHDOG_FEED cycle=")
        )
        repeated = lines[:first_feed] + [lines[first_feed]] + lines[first_feed:]
        result = analyze_capture(
            ("\r\n".join(repeated + report_boot_lines()) + "\r\n").encode(), COMMIT
        )

        self.assertEqual(result["status"], "WATCHDOG_VALIDATION_FAILED")
        self.assertTrue(
            any("out of order or repeated" in failure for failure in result["failures"]),
            result["failures"],
        )

    def test_rejects_an_error_marker(self) -> None:
        lines = feed_boot_lines() + ["WATCHDOG_FEED_ERROR"] + report_boot_lines()
        result = analyze_capture(("\r\n".join(lines) + "\r\n").encode(), COMMIT)

        self.assertEqual(result["status"], "WATCHDOG_VALIDATION_FAILED")
        self.assertIn("error marker present: WATCHDOG_FEED_ERROR", result["failures"])
        self.assertIn("WATCHDOG_FEED_ERROR", result["error_steps"])

    def test_rejects_a_truncated_report_phase_without_the_terminal_marker(self) -> None:
        lines = feed_boot_lines() + report_boot_lines()[:-1]
        result = analyze_capture(("\r\n".join(lines) + "\r\n").encode(), COMMIT)

        self.assertEqual(result["status"], "WATCHDOG_VALIDATION_FAILED")
        self.assertIn("missing completion marker: WATCHDOG_REFRESH_POLICY_PASS", result["failures"])

    def test_rejects_a_capture_with_no_banner_at_all(self) -> None:
        result = analyze_capture(b"garbage\r\n", COMMIT)

        self.assertEqual(result["status"], "WATCHDOG_VALIDATION_FAILED")
        self.assertIn("missing firmware banner", result["failures"])

    def test_rejects_a_capture_whose_feed_phase_never_stopped(self) -> None:
        """The feed phase has to end, otherwise the board never gave the watchdog
        the chance to expire."""
        lines = [line for line in feed_boot_lines() if not line.startswith("WATCHDOG_FEED_STOPPED")]
        result = analyze_capture(("\r\n".join(lines) + "\r\n").encode(), COMMIT)

        self.assertEqual(result["status"], "WATCHDOG_VALIDATION_FAILED")
        self.assertIn("missing marker: WATCHDOG_FEED_STOPPED", result["failures"])

    def test_rejects_a_capture_whose_marker_was_never_recorded(self) -> None:
        """Without the retained marker the next boot cannot know the feed phase
        happened, so the capture cannot show the two halves belong together."""
        lines = [
            line for line in feed_boot_lines() if not line.startswith("WATCHDOG_MARKER_SET")
        ] + report_boot_lines()
        result = analyze_capture(("\r\n".join(lines) + "\r\n").encode(), COMMIT)

        self.assertEqual(result["status"], "WATCHDOG_VALIDATION_FAILED")
        self.assertIn("missing marker: WATCHDOG_MARKER_SET", result["failures"])

    def test_rejects_a_capture_with_a_malformed_armed_marker(self) -> None:
        lines = [
            line.replace(
                f"WATCHDOG_ARMED nominal_window_ms={WINDOW_MS} feed_interval_ms={INTERVAL_MS} "
                f"cycles={CYCLES}",
                "WATCHDOG_ARMED nominal_window_ms=abc feed_interval_ms=40 cycles=40",
            )
            for line in feed_boot_lines()
        ] + report_boot_lines()
        result = analyze_capture(("\r\n".join(lines) + "\r\n").encode(), COMMIT)

        self.assertEqual(result["status"], "WATCHDOG_VALIDATION_FAILED")
        self.assertTrue(
            any("no usable nominal_window_ms" in failure for failure in result["failures"]),
            result["failures"],
        )

    def test_rejects_an_invalid_firmware_commit(self) -> None:
        result = analyze_capture(valid_capture(), "not-a-sha")

        self.assertEqual(result["status"], "WATCHDOG_VALIDATION_FAILED")
        self.assertIn("invalid exact firmware commit", result["failures"])

    def test_a_capture_whose_report_phase_precedes_the_feed_phase_is_rejected(self) -> None:
        """The phases have to run in order: the report only makes sense after a
        feed phase that stopped."""
        lines = report_boot_lines() + feed_boot_lines()
        result = analyze_capture(("\r\n".join(lines) + "\r\n").encode(), COMMIT)

        self.assertEqual(result["status"], "WATCHDOG_VALIDATION_FAILED")
        self.assertTrue(
            any("report phase began before the feed phase" in failure for failure in result["failures"]),
            result["failures"],
        )


if __name__ == "__main__":
    unittest.main(verbosity=2)
