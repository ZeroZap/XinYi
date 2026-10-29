#!/usr/bin/env python3
"""Host contract for the Pandora resident-bootloader capture validator."""

import json
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import time
import unittest

ROOT = Path(__file__).resolve().parents[3]
BOARDS = ROOT / "boards" / "pandora_stm32l475"
sys.path.insert(0, str(BOARDS))

import validate_fota_bootloader_capture as boot  # noqa: E402

COMMIT = "0123456789abcdef0123456789abcdef01234567"
SCRIPT = BOARDS / "validate_fota_bootloader_capture.py"


def frame(install: str | None, decision: str, jump: bool = True) -> str:
    lines = ["PANDORA FOTA BOOTLOADER READY", f"BOOTLOADER_COMMIT {COMMIT}"]
    if install is not None:
        lines.append(install)
    lines.append(decision)
    if jump:
        lines.append("PANDORA_BOOT_JUMP_APP")
    return "".join(line + "\r\n" for line in lines)


def attempt_frame(install=boot.INSTALLED) -> str:
    return frame(install, boot.ATTEMPT)


def rollback_frame() -> str:
    return frame(boot.ALREADY_INSTALLED, boot.ROLLBACK, jump=False)


def install_error_frame() -> str:
    return frame(None, "PANDORA_BOOT_INSTALL_ERROR", jump=False)


def encode(*frames: str) -> bytes:
    return "".join(frames).encode("ascii")


def analyze(payload: bytes, expect: str = "auto") -> dict:
    return boot.analyze_capture(payload, COMMIT, expect)


def journal_record(
    generation: int,
    state: int,
    attempts: int,
    version: int = 3,
    size: int = 39424,
    image_crc: int = 0xA81835BB,
    corrupt_crc: bool = False,
) -> bytes:
    head = struct.pack(
        "<IIIIII", boot.JOURNAL_MAGIC, generation, state, version, size, image_crc
    )
    record_crc = boot._crc32(head)
    if corrupt_crc:
        record_crc ^= 0x00000001
    tail = struct.pack("<IIQ", record_crc, attempts, boot.JOURNAL_COMMIT)
    return head + tail


def journal_dump(*records: bytes, slot_size: int = 2048) -> bytes:
    slots = [
        record.ljust(slot_size, b"\xff") if record else b"\xff" * slot_size for record in records
    ]
    while len(slots) < 2:
        slots.append(b"\xff" * slot_size)
    return b"".join(slots)


class AttemptChainTests(unittest.TestCase):
    def test_single_attempt_frame_is_accepted(self):
        result = analyze(encode(attempt_frame()), "attempt")
        self.assertEqual(result["status"], "BOOTLOADER_CHAIN_REVIEW_CANDIDATE")
        self.assertEqual(result["chain"], "ATTEMPT")
        self.assertEqual(result["terminal"], "jump")
        self.assertEqual(result["attempt_markers"], 1)
        self.assertEqual(result["failures"], [])
        self.assertTrue(result["expect_matched"])
        self.assertEqual(result["capture_sha256"], __import__("hashlib").sha256(
            encode(attempt_frame())).hexdigest())

    def test_repeated_attempts_accumulate(self):
        result = analyze(
            encode(attempt_frame(), attempt_frame(boot.ALREADY_INSTALLED), attempt_frame(
                boot.ALREADY_INSTALLED)), "attempt"
        )
        self.assertEqual(result["status"], "BOOTLOADER_CHAIN_REVIEW_CANDIDATE")
        self.assertEqual(result["boot_count"], 3)
        self.assertEqual(result["attempt_markers"], 3)
        self.assertEqual(result["install_markers"], [boot.INSTALLED] + [boot.ALREADY_INSTALLED] * 2)

    def test_confirm_frame_reports_confirm_chain(self):
        result = analyze(
            encode(frame(boot.ALREADY_INSTALLED, boot.CONFIRMED)), "confirm"
        )
        self.assertEqual(result["chain"], "CONFIRM")
        self.assertEqual(result["confirm_markers"], 1)
        self.assertEqual(result["attempt_markers"], 0)

    def test_attempts_before_a_confirm_report_a_confirm_chain(self):
        # The board only reaches the confirm branch after the install boot and the
        # application's request boot, so the earliest confirm capture is
        # ATTEMPT -> ATTEMPT -> CONFIRM. Labelling that "ATTEMPT" because the earlier
        # frames recorded attempts mis-described the one run that proved the branch.
        result = analyze(
            encode(
                attempt_frame(),
                attempt_frame(boot.ALREADY_INSTALLED),
                frame(boot.ALREADY_INSTALLED, boot.CONFIRMED),
            ),
            "confirm",
        )
        self.assertEqual(result["status"], "BOOTLOADER_CHAIN_REVIEW_CANDIDATE")
        self.assertEqual(result["chain"], "CONFIRM")
        self.assertEqual(result["terminal"], "jump")
        self.assertEqual(result["boot_count"], 3)
        self.assertEqual(result["attempt_markers"], 2)
        self.assertEqual(result["confirm_markers"], 1)
        self.assertTrue(result["expect_matched"])

    def test_attempts_without_a_confirm_stay_an_attempt_chain(self):
        result = analyze(
            encode(attempt_frame(), attempt_frame(boot.ALREADY_INSTALLED)), "attempt"
        )
        self.assertEqual(result["chain"], "ATTEMPT")

    def test_restage_marker_is_recorded_not_required(self):
        restaged = encode(attempt_frame()).replace(
            (f"BOOTLOADER_COMMIT {COMMIT}\r\n").encode("ascii"),
            (f"BOOTLOADER_COMMIT {COMMIT}\r\n{boot.RESTAGE_AUTHORIZED}\r\n").encode("ascii"),
        )
        result = analyze(restaged, "attempt")
        self.assertEqual(result["status"], "BOOTLOADER_CHAIN_REVIEW_CANDIDATE")
        self.assertTrue(result["frames"][0]["restage"])


class RollbackChainTests(unittest.TestCase):
    def test_attempts_then_rollback_is_a_valid_rollback_chain(self):
        result = analyze(
            encode(attempt_frame(), attempt_frame(boot.ALREADY_INSTALLED), rollback_frame()),
            "rollback",
        )
        self.assertEqual(result["status"], "BOOTLOADER_CHAIN_REVIEW_CANDIDATE")
        self.assertEqual(result["chain"], "ROLLBACK")
        self.assertEqual(result["terminal"], "halt_rollback")
        self.assertEqual(result["attempt_markers"], 2)
        self.assertEqual(result["failures"], [])

    def test_rollback_then_install_error_is_reported_as_halt_error(self):
        result = analyze(
            encode(attempt_frame(), rollback_frame(), install_error_frame()), "install_error"
        )
        self.assertEqual(result["status"], "BOOTLOADER_CHAIN_REVIEW_CANDIDATE")
        self.assertEqual(result["chain"], "HALT_ERROR")
        self.assertEqual(result["terminal"], "halt_error")
        self.assertEqual(result["boot_count"], 3)

    def test_rollback_requires_an_install_marker(self):
        payload = encode(frame(None, boot.ROLLBACK, jump=False))
        result = analyze(payload, "rollback")
        self.assertEqual(result["status"], "BOOTLOADER_CHAIN_VALIDATION_FAILED")
        self.assertTrue(any("without an install marker" in item for item in result["failures"]))

    def test_jump_after_rollback_is_rejected(self):
        payload = encode(frame(boot.ALREADY_INSTALLED, boot.ROLLBACK, jump=True))
        result = analyze(payload)
        self.assertEqual(result["status"], "BOOTLOADER_CHAIN_VALIDATION_FAILED")
        self.assertTrue(any("jump after halting decision" in item for item in result["failures"]))
        self.assertTrue(any("without a jump marker" not in item for item in result["failures"]))

    def test_a_new_boot_frame_after_a_halt_is_accepted(self):
        payload = encode(rollback_frame(), install_error_frame())
        result = analyze(payload, "install_error")
        self.assertEqual(result["status"], "BOOTLOADER_CHAIN_REVIEW_CANDIDATE")
        self.assertEqual(result["boot_count"], 2)
        self.assertEqual(result["frames"][0]["decision"], boot.ROLLBACK)
        self.assertFalse(result["frames"][0]["jump"])


class StructuralFailureTests(unittest.TestCase):
    def test_attempt_without_jump_is_rejected(self):
        result = analyze(encode(frame(boot.INSTALLED, boot.ATTEMPT, jump=False)))
        self.assertEqual(result["status"], "BOOTLOADER_CHAIN_VALIDATION_FAILED")
        self.assertTrue(any("without a jump marker" in item for item in result["failures"]))

    def test_attempt_without_install_marker_is_rejected(self):
        result = analyze(encode(frame(None, boot.ATTEMPT)))
        self.assertEqual(result["status"], "BOOTLOADER_CHAIN_VALIDATION_FAILED")
        self.assertTrue(any("without an install marker" in item for item in result["failures"]))

    def test_duplicate_decision_marker_is_rejected(self):
        payload = encode(frame(boot.INSTALLED, boot.ATTEMPT)) + (
            f"{boot.ATTEMPT}\r\n"
        ).encode("ascii")
        result = analyze(payload)
        self.assertEqual(result["status"], "BOOTLOADER_CHAIN_VALIDATION_FAILED")
        self.assertTrue(any("duplicate decision marker" in item for item in result["failures"]))

    def test_duplicate_install_marker_is_rejected(self):
        payload = encode(frame(boot.INSTALLED, boot.ATTEMPT)).replace(
            f"{boot.INSTALLED}\r\n".encode("ascii"),
            f"{boot.INSTALLED}\r\n{boot.INSTALLED}\r\n".encode("ascii"),
        )
        result = analyze(payload)
        self.assertEqual(result["status"], "BOOTLOADER_CHAIN_VALIDATION_FAILED")
        self.assertTrue(any("duplicate install marker" in item for item in result["failures"]))

    def test_decision_before_commit_marker_is_rejected(self):
        payload = (
            f"{boot.READY}\r\n{boot.INSTALLED}\r\n{boot.ATTEMPT}\r\n{boot.JUMP}\r\n"
        ).encode("ascii")
        result = analyze(payload)
        self.assertEqual(result["status"], "BOOTLOADER_CHAIN_VALIDATION_FAILED")
        self.assertTrue(any("decision before the commit marker" in item for item in result["failures"]))
        self.assertTrue(any("identity marker missing or mismatched" in item for item in result["failures"]))

    def test_unknown_marker_is_rejected(self):
        payload = encode(attempt_frame()) + b"PANDORA_BOOT_UNEXPECTED\r\n"
        result = analyze(payload)
        self.assertEqual(result["status"], "BOOTLOADER_CHAIN_VALIDATION_FAILED")
        self.assertTrue(any("unknown boot marker" in item for item in result["failures"]))

    def test_wrong_commit_is_rejected(self):
        payload = encode(attempt_frame()).replace(
            COMMIT.encode("ascii"), b"f" * 40
        )
        result = analyze(payload)
        self.assertEqual(result["status"], "BOOTLOADER_CHAIN_VALIDATION_FAILED")
        self.assertTrue(any("identity marker missing" in item for item in result["failures"]))

    def test_frame_without_decision_is_rejected(self):
        payload = f"{boot.READY}\r\nBOOTLOADER_COMMIT {COMMIT}\r\n".encode("ascii")
        result = analyze(payload)
        self.assertEqual(result["status"], "BOOTLOADER_CHAIN_VALIDATION_FAILED")
        self.assertTrue(any("no decision marker" in item for item in result["failures"]))

    def test_empty_capture_is_rejected(self):
        result = analyze(b"")
        self.assertEqual(result["status"], "BOOTLOADER_CHAIN_VALIDATION_FAILED")
        self.assertIn("no boot frame captured", result["failures"])

    def test_application_output_before_the_first_frame_is_counted(self):
        result = analyze(b"ICM20608_REGS_HEX=01,00,08,08,04,04,01\r\n" + encode(attempt_frame()))
        self.assertEqual(result["status"], "BOOTLOADER_CHAIN_REVIEW_CANDIDATE")
        self.assertEqual(result["application_lines"], 1)
        self.assertEqual(result["frames"][0]["decision"], boot.ATTEMPT)

    def test_application_output_after_the_jump_is_counted(self):
        payload = (
            encode(attempt_frame())
            + b"PANDORA STM32L475VE XINYI SMOKE OK\r\nAHT10 0x38 ACK\r\n"
        )
        result = analyze(payload, "attempt")
        self.assertEqual(result["status"], "BOOTLOADER_CHAIN_REVIEW_CANDIDATE")
        self.assertEqual(result["application_lines"], 2)

    def test_foreign_output_inside_a_boot_frame_is_rejected(self):
        payload = encode(attempt_frame()).replace(
            f"{boot.INSTALLED}\r\n".encode("ascii"),
            f"{boot.INSTALLED}\r\nunexpected noise\r\n".encode("ascii"),
        )
        result = analyze(payload)
        self.assertEqual(result["status"], "BOOTLOADER_CHAIN_VALIDATION_FAILED")
        self.assertTrue(any("foreign line inside the boot frame" in item for item in result["failures"]))

    def test_no_valid_image_halt_is_accepted(self):
        result = analyze(encode(frame(None, boot.NO_VALID_IMAGE, jump=False)))
        self.assertEqual(result["status"], "BOOTLOADER_CHAIN_REVIEW_CANDIDATE")
        self.assertEqual(result["chain"], "NO_IMAGE")

    def test_install_error_alone_has_no_install_marker_requirement(self):
        result = analyze(encode(install_error_frame()), "install_error")
        self.assertEqual(result["status"], "BOOTLOADER_CHAIN_REVIEW_CANDIDATE")


class ExpectationTests(unittest.TestCase):
    def test_attempt_expectation_rejects_a_rollback_chain(self):
        result = analyze(encode(attempt_frame(), rollback_frame()), "attempt")
        self.assertEqual(result["status"], "BOOTLOADER_CHAIN_VALIDATION_FAILED")
        self.assertFalse(result["expect_matched"])

    def test_rollback_expectation_rejects_an_attempt_chain(self):
        result = analyze(encode(attempt_frame()), "rollback")
        self.assertEqual(result["status"], "BOOTLOADER_CHAIN_VALIDATION_FAILED")
        self.assertIn("chain does not match the expected outcome 'rollback'", result["failures"])

    def test_install_error_expectation_requires_the_install_error_marker(self):
        result = analyze(encode(attempt_frame(), install_error_frame()), "install_error")
        self.assertTrue(result["status"] == "BOOTLOADER_CHAIN_REVIEW_CANDIDATE")
        self.assertEqual(result["chain"], "HALT_ERROR")
        self.assertEqual(result["expect"], "install_error")

    def test_unknown_expectation_is_rejected(self):
        with self.assertRaises(ValueError):
            analyze(encode(attempt_frame()), "reboot")

    def test_commit_must_be_an_exact_lowercase_sha(self):
        with self.assertRaises(ValueError):
            boot.analyze_capture(encode(attempt_frame()), "ABC")
        with self.assertRaises(ValueError):
            boot.analyze_capture(encode(attempt_frame()), COMMIT.upper())


class JournalTests(unittest.TestCase):
    def test_blank_journal_reports_no_valid_record(self):
        result = boot.decode_journal(b"\xff" * 4096)
        self.assertEqual(result["status"], "JOURNAL_INVALID")
        self.assertIsNone(result["active"])
        self.assertEqual(result["failures"], ["neither journal slot holds a valid record"])
        self.assertFalse(result["slots"][0]["valid"])
        self.assertTrue(result["slots"][0]["blank"])

    def test_single_valid_slot_is_selected(self):
        result = boot.decode_journal(
            journal_dump(b"", journal_record(generation=3, state=2, attempts=1))
        )
        self.assertEqual(result["status"], "JOURNAL_VALID")
        self.assertEqual(result["active_slot"], 1)
        self.assertEqual(result["active"]["state"], "INSTALLED")
        self.assertEqual(result["active"]["boot_attempts"], 1)
        self.assertEqual(result["active"]["image_size"], 39424)
        self.assertEqual(result["active"]["image_crc32"], "0xa81835bb")
        self.assertTrue(result["active"]["record_crc32_ok"])

    def test_newer_generation_wins(self):
        result = boot.decode_journal(
            journal_dump(
                journal_record(generation=8, state=2, attempts=1),
                journal_record(generation=9, state=4, attempts=3),
            )
        )
        self.assertEqual(result["status"], "JOURNAL_VALID")
        self.assertEqual(result["active_slot"], 1)
        self.assertEqual(result["active"]["state"], "ROLLED_BACK")
        self.assertEqual(result["active"]["generation"], 9)
        self.assertEqual(result["active"]["boot_attempts"], 3)

    def test_older_generation_loses(self):
        result = boot.decode_journal(
            journal_dump(
                journal_record(generation=9, state=4, attempts=3),
                journal_record(generation=8, state=2, attempts=2),
            )
        )
        self.assertEqual(result["active_slot"], 0)
        self.assertEqual(result["active"]["state"], "ROLLED_BACK")

    def test_corrupt_crc_makes_the_record_invalid(self):
        result = boot.decode_journal(
            journal_dump(
                journal_record(generation=3, state=2, attempts=1, corrupt_crc=True),
                journal_record(generation=4, state=4, attempts=3),
            )
        )
        self.assertEqual(result["status"], "JOURNAL_VALID")
        self.assertFalse(result["slots"][0]["valid"])
        self.assertFalse(result["slots"][0]["record_crc32_ok"])
        self.assertEqual(result["active_slot"], 1)

    def test_both_valid_with_one_generation_is_rejected(self):
        result = boot.decode_journal(
            journal_dump(
                journal_record(generation=4, state=2, attempts=1),
                journal_record(generation=4, state=4, attempts=3),
            )
        )
        self.assertEqual(result["status"], "JOURNAL_INVALID")
        self.assertEqual(result["failures"], ["both journal slots are valid with the same generation"])

    def test_short_dump_is_rejected(self):
        with self.assertRaises(ValueError):
            boot.decode_journal(b"\xff" * 32)

    def test_slot_size_must_fit_a_record(self):
        with self.assertRaises(ValueError):
            boot.decode_journal(b"\xff" * 4096, slot_size=16)

    def test_every_journal_state_name_is_reachable(self):
        expected = {
            1: "INSTALLING",
            2: "INSTALLED",
            3: "CONFIRMED",
            4: "ROLLED_BACK",
            5: "RESTAGE_AUTHORIZED",
        }
        for value, name in expected.items():
            blob = journal_dump(journal_record(generation=2, state=value, attempts=0))
            self.assertEqual(boot.decode_journal(blob)["active"]["state"], name)


class CommandLineTests(unittest.TestCase):
    def run_tool(self, *arguments: str) -> subprocess.CompletedProcess:
        return subprocess.run(
            [sys.executable, str(SCRIPT), *arguments],
            capture_output=True,
            text=True,
            cwd=ROOT,
            check=False,
        )

    def test_analyze_cli_accepts_and_writes_metadata(self):
        with tempfile.TemporaryDirectory() as directory:
            capture = Path(directory) / "attempt.txt"
            metadata = Path(directory) / "attempt.json"
            capture.write_bytes(encode(attempt_frame()))
            result = self.run_tool(
                "analyze",
                "--input",
                str(capture),
                "--firmware-commit",
                COMMIT,
                "--expect",
                "attempt",
                "--metadata",
                str(metadata),
            )
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            self.assertIn("BOOTLOADER_CHAIN_REVIEW_CANDIDATE", result.stdout)
            record = json.loads(metadata.read_text())
            self.assertEqual(record["chain"], "ATTEMPT")
            self.assertEqual(record["firmware_commit"], COMMIT)

    def test_analyze_cli_fails_on_a_rollback_expectation_mismatch(self):
        with tempfile.TemporaryDirectory() as directory:
            capture = Path(directory) / "attempt.txt"
            capture.write_bytes(encode(attempt_frame()))
            result = self.run_tool(
                "analyze", "--input", str(capture), "--firmware-commit", COMMIT,
                "--expect", "rollback",
            )
            self.assertEqual(result.returncode, 1)
            self.assertIn("BOOTLOADER_CHAIN_VALIDATION_FAILED", result.stdout)

    def test_journal_cli_reports_the_active_record(self):
        with tempfile.TemporaryDirectory() as directory:
            dump = Path(directory) / "journal.bin"
            metadata = Path(directory) / "journal.json"
            dump.write_bytes(
                journal_dump(
                    journal_record(generation=8, state=2, attempts=2),
                    journal_record(generation=9, state=4, attempts=3),
                )
            )
            result = self.run_tool(
                "journal", "--input", str(dump), "--metadata", str(metadata)
            )
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            self.assertIn("JOURNAL_VALID", result.stdout)
            self.assertIn("state=ROLLED_BACK", result.stdout)
            self.assertIn("attempts=3", result.stdout)
            self.assertEqual(json.loads(metadata.read_text())["active_slot"], 1)

    def test_journal_cli_fails_on_a_blank_dump(self):
        with tempfile.TemporaryDirectory() as directory:
            dump = Path(directory) / "journal.bin"
            dump.write_bytes(b"\xff" * 4096)
            result = self.run_tool("journal", "--input", str(dump))
            self.assertEqual(result.returncode, 1)
            self.assertIn("JOURNAL_INVALID", result.stdout)

    def test_capture_cli_reports_a_missing_device(self):
        with tempfile.TemporaryDirectory() as directory:
            metadata = Path(directory) / "capture.json"
            result = self.run_tool(
                "capture",
                "--device",
                str(Path(directory) / "does-not-exist"),
                "--output",
                str(Path(directory) / "capture.txt"),
                "--metadata",
                str(metadata),
                "--firmware-commit",
                COMMIT,
            )
            self.assertEqual(result.returncode, 3)
            record = json.loads(metadata.read_text())
            self.assertEqual(record["status"], "DEVICE_OPEN_FAILED")
            self.assertFalse((Path(directory) / "capture.txt").exists())

    def test_capture_reads_a_pseudo_terminal(self):
        import pty
        import threading

        master, slave = pty.openpty()
        slave_path = os.ttyname(slave)

        def writer() -> None:
            time.sleep(0.1)
            os.write(master, encode(attempt_frame()))

        writer_thread = threading.Thread(target=writer)
        writer_thread.start()
        try:
            status, payload, error = boot.capture(slave_path, 0.4)
        finally:
            writer_thread.join()
            os.close(master)
            os.close(slave)
        self.assertEqual(status, "CAPTURED")
        self.assertIsNone(error)
        result = analyze(payload, "attempt")
        self.assertEqual(result["status"], "BOOTLOADER_CHAIN_REVIEW_CANDIDATE")


if __name__ == "__main__":
    unittest.main(verbosity=2)
