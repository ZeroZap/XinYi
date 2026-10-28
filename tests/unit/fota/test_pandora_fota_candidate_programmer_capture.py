#!/usr/bin/env python3
"""Host contract for the Pandora W25Q128 candidate-programmer capture validator."""

import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import time
import unittest

ROOT = Path(__file__).resolve().parents[3]
BOARDS = ROOT / "boards" / "pandora_stm32l475"
sys.path.insert(0, str(BOARDS))

import validate_fota_candidate_programmer_capture as prog  # noqa: E402

COMMIT = "0123456789abcdef0123456789abcdef01234567"
SOURCE = "fedcba9876543210fedcba9876543210fedcba98"
SCRIPT = BOARDS / "validate_fota_candidate_programmer_capture.py"


def complete_frame(commit: str = COMMIT, source: str = SOURCE) -> str:
    return "".join(
        line + "\r\n"
        for line in (
            prog.READY,
            prog.COMMIT_PREFIX + commit,
            prog.SOURCE_COMMIT_PREFIX + source,
            prog.VERIFIED,
        )
    )


def truncated_frame(commit: str = COMMIT, source: str = SOURCE) -> str:
    return "".join(
        line + "\r\n"
        for line in (prog.READY, prog.COMMIT_PREFIX + commit, prog.SOURCE_COMMIT_PREFIX + source)
    )


def encode(*frames: str) -> bytes:
    return "".join(frames).encode("ascii")


def analyze(payload: bytes, commit: str = COMMIT, source: str = SOURCE) -> dict:
    return prog.analyze_capture(payload, commit, source)


class AcceptTests(unittest.TestCase):
    def test_single_complete_frame_is_accepted(self):
        result = analyze(encode(complete_frame()))
        self.assertEqual(result["status"], "PROGRAMMER_CAPTURE_REVIEW_CANDIDATE")
        self.assertEqual(result["frame_count"], 1)
        self.assertEqual(result["complete_frames"], 1)
        self.assertEqual(result["partial_frames"], 0)
        self.assertEqual(result["verified_markers"], 1)
        self.assertEqual(result["failures"], [])

    def test_previous_firmware_output_before_the_banner_is_counted(self):
        payload = b"PANDORA_BOOT_JUMP_APP\r\nICM20608_REGS_HEX=01,00\r\n" + encode(
            complete_frame()
        )
        result = analyze(payload)
        self.assertEqual(result["status"], "PROGRAMMER_CAPTURE_REVIEW_CANDIDATE")
        self.assertEqual(result["foreign_lines"], 2)
        self.assertEqual(result["complete_frames"], 1)

    def test_interrupted_run_then_complete_run_is_accepted(self):
        payload = encode(truncated_frame(), complete_frame())
        result = analyze(payload)
        self.assertEqual(result["status"], "PROGRAMMER_CAPTURE_REVIEW_CANDIDATE")
        self.assertEqual(result["frame_count"], 2)
        self.assertEqual(result["partial_frames"], 1)
        self.assertEqual(result["complete_frames"], 1)

    def test_scope_declares_what_it_does_not_prove(self):
        result = analyze(encode(complete_frame()))
        self.assertIn("W25Q128 erase/program endurance", " ".join(result["not_evidence_for"]))


class RejectTests(unittest.TestCase):
    def assert_rejected(self, payload: bytes, fragment: str) -> dict:
        result = analyze(payload)
        self.assertEqual(result["status"], "PROGRAMMER_CAPTURE_VALIDATION_FAILED")
        self.assertTrue(
            any(fragment in failure for failure in result["failures"]),
            f"{fragment!r} not in {result['failures']}",
        )
        return result

    def test_empty_capture_is_rejected(self):
        self.assert_rejected(b"", "no programmer banner")

    def test_truncated_final_frame_is_rejected(self):
        self.assert_rejected(
            encode(truncated_frame()), "final frame never printed the verification marker"
        )

    def test_missing_identity_line_is_rejected(self):
        payload = encode(prog.READY + "\r\n" + prog.VERIFIED + "\r\n")
        self.assert_rejected(payload, "missing an identity line")

    def test_program_error_marker_is_rejected(self):
        payload = encode(
            prog.READY
            + "\r\n"
            + prog.COMMIT_PREFIX
            + COMMIT
            + "\r\n"
            + prog.SOURCE_COMMIT_PREFIX
            + SOURCE
            + "\r\n"
            + "PANDORA_FOTA_CANDIDATE_PROGRAM_ERROR\r\n"
        )
        self.assert_rejected(payload, "PANDORA_FOTA_CANDIDATE_PROGRAM_ERROR")

    def test_verify_error_marker_is_rejected(self):
        payload = encode(complete_frame() + "PANDORA_FOTA_CANDIDATE_VERIFY_ERROR\r\n")
        result = self.assert_rejected(payload, "PANDORA_FOTA_CANDIDATE_VERIFY_ERROR")
        self.assertIn("PANDORA_FOTA_CANDIDATE_VERIFY_ERROR", result["error_markers"])

    def test_wrong_programmer_commit_is_rejected(self):
        payload = encode(complete_frame(commit="0" * 40))
        self.assert_rejected(payload, "PROGRAMMER_COMMIT")

    def test_wrong_candidate_source_commit_is_rejected(self):
        payload = encode(complete_frame(source="0" * 40))
        self.assert_rejected(payload, "CANDIDATE_SOURCE_COMMIT")

    def test_unknown_programmer_marker_is_rejected(self):
        payload = encode(complete_frame() + "PANDORA_FOTA_CANDIDATE_SOMETHING_NEW\r\n")
        self.assert_rejected(payload, "unknown programmer marker")

    def test_foreign_output_inside_the_frame_is_rejected(self):
        payload = encode(
            prog.READY
            + "\r\n"
            + prog.COMMIT_PREFIX
            + COMMIT
            + "\r\n"
            + "W25Q128 erase in progress\r\n"
            + prog.SOURCE_COMMIT_PREFIX
            + SOURCE
            + "\r\n"
            + prog.VERIFIED
            + "\r\n"
        )
        self.assert_rejected(payload, "foreign output inside the programmer frame")

    def test_identity_lines_without_a_banner_are_rejected(self):
        payload = encode(
            prog.COMMIT_PREFIX + COMMIT + "\r\n" + prog.SOURCE_COMMIT_PREFIX + SOURCE + "\r\n"
        )
        result = self.assert_rejected(payload, "no programmer banner")
        self.assertEqual(result["foreign_lines"], 2)

    def test_bad_commit_arguments_are_refused(self):
        with self.assertRaises(ValueError):
            prog.validate_firmware_commit("not-a-sha")
        with self.assertRaises(ValueError):
            prog.validate_candidate_source_commit("F" * 40)


class CliTests(unittest.TestCase):
    def run_cli(self, *args: str) -> subprocess.CompletedProcess:
        return subprocess.run(
            [sys.executable, str(SCRIPT), *args],
            cwd=ROOT,
            capture_output=True,
            text=True,
            timeout=60,
        )

    def test_analyze_accepts_complete_capture(self):
        with tempfile.TemporaryDirectory() as temp:
            capture = Path(temp) / "capture.log"
            capture.write_bytes(encode(complete_frame()))
            metadata = Path(temp) / "meta.json"
            result = self.run_cli(
                "analyze",
                "--input",
                str(capture),
                "--firmware-commit",
                COMMIT,
                "--candidate-source-commit",
                SOURCE,
                "--metadata",
                str(metadata),
            )
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            record = json.loads(metadata.read_text())
            self.assertEqual(record["status"], "PROGRAMMER_CAPTURE_REVIEW_CANDIDATE")
            self.assertEqual(record["frame_count"], 1)

    def test_analyze_rejects_truncated_capture(self):
        with tempfile.TemporaryDirectory() as temp:
            capture = Path(temp) / "capture.log"
            capture.write_bytes(encode(truncated_frame()))
            result = self.run_cli(
                "analyze",
                "--input",
                str(capture),
                "--firmware-commit",
                COMMIT,
                "--candidate-source-commit",
                SOURCE,
            )
            self.assertEqual(result.returncode, 1)
            self.assertIn("final frame never printed", result.stdout)

    def test_capture_reads_a_real_pty_before_judging(self):
        import pty
        import threading

        master, slave = pty.openpty()
        slave_path = os.ttyname(slave)

        def writer() -> None:
            time.sleep(0.1)
            os.write(master, encode(complete_frame()))

        writer_thread = threading.Thread(target=writer)
        writer_thread.start()
        try:
            status, payload, error = prog.capture(slave_path, 0.4)
        finally:
            writer_thread.join()
            os.close(master)
            os.close(slave)
        self.assertEqual(status, "CAPTURED")
        self.assertIsNone(error)
        self.assertEqual(analyze(payload)["status"], "PROGRAMMER_CAPTURE_REVIEW_CANDIDATE")

    def test_capture_cli_reports_zero_bytes_as_no_data(self):
        import pty

        master, slave = pty.openpty()
        slave_path = os.ttyname(slave)
        with tempfile.TemporaryDirectory() as temp:
            try:
                result = self.run_cli(
                    "capture",
                    "--device",
                    slave_path,
                    "--timeout",
                    "0.3",
                    "--firmware-commit",
                    COMMIT,
                    "--candidate-source-commit",
                    SOURCE,
                    "--output",
                    str(Path(temp) / "capture.log"),
                    "--metadata",
                    str(Path(temp) / "meta.json"),
                )
            finally:
                os.close(master)
                os.close(slave)
            self.assertEqual(result.returncode, 2, result.stdout + result.stderr)
            self.assertIn("NO_DATA_TIMEOUT", result.stdout)


if __name__ == "__main__":
    unittest.main()
