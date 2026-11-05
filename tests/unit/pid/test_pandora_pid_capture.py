#!/usr/bin/env python3
"""Fail-closed contracts for the Pandora PID board capture validator."""

from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "boards" / "pandora_stm32l475"))

from validate_pid_capture import analyze_capture  # noqa: E402

COMMIT = "1" * 40
BANNER = "PANDORA PID BOARD READY"
TERMINAL = "B1_PID_BOARD_RUNTIME_PASS"
CHECKS = (
    "PANDORA_PID_LOOP_OK_A",
    "PANDORA_PID_WINDUP_OK_A",
    "PANDORA_PID_CONTRACT_OK_A",
    "PANDORA_PID_RESET_OK_A",
)


def valid_capture() -> bytes:
    lines = [BANNER, f"FIRMWARE_COMMIT {COMMIT}", *CHECKS, TERMINAL]
    return ("\r\n".join(lines) + "\r\n").encode()


class PandoraPidCaptureContract(unittest.TestCase):
    def test_accepts_identity_bound_pass_capture(self) -> None:
        result = analyze_capture(valid_capture(), COMMIT)

        self.assertEqual(result["status"], "B1_PID_BOARD_RUNTIME_PASS")
        self.assertEqual(result["checks_passed"], 4)
        self.assertEqual(result["error_steps"], [])
        self.assertEqual(result["failures"], [])

    def test_rejects_a_capture_from_another_firmware(self) -> None:
        result = analyze_capture(valid_capture().replace(COMMIT.encode(), b"2" * 40), COMMIT)

        self.assertEqual(result["status"], "PID_VALIDATION_FAILED")
        self.assertIn("missing or out-of-order exact firmware identity", result["failures"])

    def test_rejects_a_missing_or_reordered_check_marker(self) -> None:
        dropped = valid_capture().replace(b"PANDORA_PID_WINDUP_OK_A\r\n", b"")
        result = analyze_capture(dropped, COMMIT)
        self.assertEqual(result["status"], "PID_VALIDATION_FAILED")
        self.assertIn("missing check marker: PANDORA_PID_WINDUP_OK_A", result["failures"])

        reordered = (
            "\r\n".join(
                [
                    BANNER,
                    f"FIRMWARE_COMMIT {COMMIT}",
                    CHECKS[1],
                    CHECKS[0],
                    CHECKS[2],
                    CHECKS[3],
                    TERMINAL,
                ]
            )
            + "\r\n"
        ).encode()
        result = analyze_capture(reordered, COMMIT)
        self.assertEqual(result["status"], "PID_VALIDATION_FAILED")
        self.assertIn("check markers are out of order", result["failures"])

    def test_rejects_a_truncated_capture_without_the_terminal_marker(self) -> None:
        truncated = ("\r\n".join([BANNER, f"FIRMWARE_COMMIT {COMMIT}", *CHECKS]) + "\r\n").encode()
        result = analyze_capture(truncated, COMMIT)

        self.assertEqual(result["status"], "PID_VALIDATION_FAILED")
        self.assertIn("missing or out-of-order completion marker", result["failures"])

    def test_reports_the_failing_step_of_an_error_capture(self) -> None:
        failing = (
            "\r\n".join(
                [
                    BANNER,
                    f"FIRMWARE_COMMIT {COMMIT}",
                    "PANDORA_PID_LOOP_ERROR_STEP_7",
                    "PANDORA_PID_WINDUP_OK_A",
                ]
            )
            + "\r\n"
        ).encode()
        result = analyze_capture(failing, COMMIT)

        self.assertEqual(result["status"], "PID_VALIDATION_FAILED")
        self.assertEqual(result["error_steps"], ["PANDORA_PID_LOOP_ERROR_STEP_7"])
        self.assertIn(
            "error marker present: PANDORA_PID_LOOP_ERROR_STEP_7",
            result["failures"],
        )

    def test_rejects_a_malformed_error_step(self) -> None:
        failing = (
            "\r\n".join(
                [BANNER, f"FIRMWARE_COMMIT {COMMIT}", "PANDORA_PID_LOOP_ERROR_STEP_x", TERMINAL]
            )
            + "\r\n"
        ).encode()
        result = analyze_capture(failing, COMMIT)

        self.assertEqual(result["status"], "PID_VALIDATION_FAILED")
        self.assertIn(
            "error marker 'PANDORA_PID_LOOP_ERROR_STEP_' has a malformed step field 'x'",
            result["failures"],
        )

    def test_rejects_a_crash_marker(self) -> None:
        crashed = valid_capture().replace(b"B1_PID_BOARD_RUNTIME_PASS", b"HardFault")
        result = analyze_capture(crashed, COMMIT)

        self.assertEqual(result["status"], "PID_VALIDATION_FAILED")
        self.assertIn("error marker present: HardFault", result["failures"])

    def test_rejects_an_empty_capture_and_a_bad_identity(self) -> None:
        result = analyze_capture(b"", COMMIT)
        self.assertEqual(result["status"], "PID_VALIDATION_FAILED")
        self.assertIn("missing firmware banner", result["failures"])

        result = analyze_capture(valid_capture(), "not-a-sha")
        self.assertEqual(result["status"], "PID_VALIDATION_FAILED")
        self.assertIn("invalid exact firmware commit", result["failures"])


if __name__ == "__main__":
    unittest.main(verbosity=1)
