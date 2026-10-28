"""Host coverage for tools/pandora_hil_program.py (no debugger, no hardware)."""

import contextlib
import io
import json
import pathlib
import sys
import tempfile
import unittest
from unittest import mock

TOOLS = pathlib.Path(__file__).resolve().parent.parent
if str(TOOLS) not in sys.path:
    sys.path.insert(0, str(TOOLS))

import pandora_hil_program as prog  # noqa: E402
import pandora_hil_state as hil  # noqa: E402

REPO_ROOT = TOOLS.parent
APP = hil.region_by_name("application")
BOOT = hil.region_by_name("bootloader")


class FlatMemoryBackend:
    """Flat byte-addressed target that models st-flash page erase before a write."""

    def __init__(self) -> None:
        self.bytes: dict[int, int] = {}
        self.ops: list[tuple[str, int]] = []
        self.fail_reads: set[int] = set()
        self.corrupt_after_write = False
        self.mutate_after_reset: list[tuple[int, int]] = []
        self.resets = 0

    def seed(self, address: int, payload: bytes) -> None:
        for offset, value in enumerate(payload):
            self.bytes[address + offset] = value
        self.ops.append(("seed", address))

    def read(self, address: int, size: int, destination: pathlib.Path) -> None:
        if address in self.fail_reads:
            raise hil.BackendError(f"read at {hex(address)} failed")
        self.ops.append(("read", address))
        payload = bytes(self.bytes.get(address + offset, 0xFF) for offset in range(size))
        destination = pathlib.Path(destination)
        destination.parent.mkdir(parents=True, exist_ok=True)
        destination.write_bytes(payload)

    def write(self, source: pathlib.Path, address: int) -> None:
        payload = pathlib.Path(source).read_bytes()
        self.ops.append(("write", address))
        first_page = address // prog.FLASH_PAGE_SIZE
        last_page = (address + len(payload) - 1) // prog.FLASH_PAGE_SIZE
        for page in range(first_page, last_page + 1):
            base = page * prog.FLASH_PAGE_SIZE
            for offset in range(prog.FLASH_PAGE_SIZE):
                self.bytes[base + offset] = 0xFF
        for offset, value in enumerate(payload):
            self.bytes[address + offset] = (value ^ 0xFF) if self.corrupt_after_write else value

    def reset(self) -> None:
        self.resets += 1
        self.ops.append(("reset", 0))
        for address, value in self.mutate_after_reset:
            self.bytes[address] = value

    def region_bytes(self, name: str) -> bytes:
        region = hil.region_by_name(name)
        return bytes(
            self.bytes.get(region.address + offset, 0xFF) for offset in range(region.size)
        )


def seeded_backend() -> FlatMemoryBackend:
    backend = FlatMemoryBackend()
    for index, region in enumerate(hil.REGIONS):
        fill = 0xFF if region.name == "rtc_backup" else (0x20 + index * 7) & 0xFF
        backend.seed(region.address, bytes([fill]) * region.size)
    return backend


def snapshot_of(backend: FlatMemoryBackend, directory: pathlib.Path) -> pathlib.Path:
    hil.take_snapshot(backend, directory, label="test")
    return directory


def image_file(directory: pathlib.Path, size: int, name: str = "app.bin") -> pathlib.Path:
    path = pathlib.Path(directory) / name
    path.write_bytes(bytes((offset * 13 + 7) & 0xFF for offset in range(size)))
    return path


class ImageViewTests(unittest.TestCase):
    def setUp(self):
        self._tmp = tempfile.TemporaryDirectory()
        self.tmp = pathlib.Path(self._tmp.name)
        self.addCleanup(self._tmp.cleanup)

    def test_accepts_an_image_at_the_application_base(self):
        view = prog.image_view(image_file(self.tmp, 4096), APP.address)

        self.assertEqual(view.region.name, "application")
        self.assertEqual(view.size, 4096)
        self.assertEqual(view.page_end, APP.address + 4096)

    def test_rejects_a_missing_image(self):
        with self.assertRaises(prog.ProgramError):
            prog.image_view(self.tmp / "absent.bin", APP.address)

    def test_rejects_an_empty_image(self):
        path = self.tmp / "empty.bin"
        path.write_bytes(b"")

        with self.assertRaises(prog.ProgramError):
            prog.image_view(path, APP.address)

    def test_rejects_an_elf_so_the_written_span_is_what_was_hashed(self):
        path = self.tmp / "app.elf"
        path.write_bytes(b"\x7fELF" + bytes(64))

        with self.assertRaises(prog.ProgramError):
            prog.image_view(path, APP.address)

    def test_rejects_the_rtc_block(self):
        with self.assertRaises(prog.ProgramError):
            prog.image_view(image_file(self.tmp, 128), hil.RTC_BACKUP_BASE)

    def test_rejects_an_unmapped_address(self):
        with self.assertRaises(prog.ProgramError):
            prog.image_view(image_file(self.tmp, 128), 0x20000000)

    def test_rejects_an_unaligned_start(self):
        with self.assertRaises(prog.ProgramError):
            prog.image_view(image_file(self.tmp, 128), APP.address + 1)

    def test_rejects_an_image_bigger_than_its_region(self):
        with self.assertRaises(prog.ProgramError):
            prog.image_view(image_file(self.tmp, APP.size + prog.FLASH_PAGE_SIZE), APP.address)

    def test_rejects_a_span_that_crosses_the_region_end(self):
        # Starts inside the application but runs past 0x0807E000.
        start = APP.end - prog.FLASH_PAGE_SIZE

        with self.assertRaises(prog.ProgramError):
            prog.image_view(image_file(self.tmp, prog.FLASH_PAGE_SIZE * 2), start)

    def test_accepts_an_image_that_exactly_fills_a_region(self):
        view = prog.image_view(image_file(self.tmp, BOOT.size), BOOT.address)

        self.assertEqual(view.region.name, "bootloader")
        self.assertEqual(view.end, BOOT.end)


class ClassifyReadbackTests(unittest.TestCase):
    def setUp(self):
        self._tmp = tempfile.TemporaryDirectory()
        self.tmp = pathlib.Path(self._tmp.name)
        self.addCleanup(self._tmp.cleanup)
        self.view = prog.image_view(image_file(self.tmp, 1024), APP.address)
        self.before = bytes([0x11]) * APP.size

    def _observed(self) -> bytearray:
        observed = bytearray(self.before)
        observed[0:1024] = self.view.path.read_bytes()
        return observed

    def test_identical_span_with_untouched_page_tail_passes(self):
        truth = prog.classify_readback(self.view, bytes(self._observed()), self.before)

        self.assertTrue(truth.span_identical)
        self.assertEqual(truth.unexpected_changes, 0)
        self.assertEqual(truth.erased_tail, 0)  # tail still holds the pre-write bytes

    def test_page_erase_tail_is_reported_not_fatal(self):
        # st-flash erases whole pages, so the end of the image's page goes to 0xFF.
        observed = self._observed()
        tail_end = prog.FLASH_PAGE_SIZE - (self.view.address % prog.FLASH_PAGE_SIZE)
        observed[self.view.size : tail_end] = b"\xff" * (tail_end - self.view.size)

        truth = prog.classify_readback(self.view, bytes(observed), self.before)

        self.assertTrue(truth.span_identical)
        self.assertEqual(truth.unexpected_changes, 0)
        self.assertEqual(truth.erased_tail, prog.FLASH_PAGE_SIZE - self.view.size)

    def test_a_changed_image_byte_reports_the_offset(self):
        observed = self._observed()
        observed[42] ^= 0xFF

        truth = prog.classify_readback(self.view, bytes(observed), self.before)

        self.assertFalse(truth.span_identical)
        self.assertEqual(truth.first_diff, 42)

    def test_a_change_beyond_the_erase_page_is_unexpected(self):
        observed = self._observed()
        observed[prog.FLASH_PAGE_SIZE + 100] ^= 0xFF

        truth = prog.classify_readback(self.view, bytes(observed), self.before)

        self.assertEqual(truth.unexpected_changes, 1)
        self.assertEqual(truth.unexpected_first, prog.FLASH_PAGE_SIZE + 100)

    def test_a_short_read_back_is_an_error(self):
        with self.assertRaises(prog.ProgramError):
            prog.classify_readback(self.view, self.before[:100], self.before)


class ProgramTests(unittest.TestCase):
    def setUp(self):
        self._tmp = tempfile.TemporaryDirectory()
        self.tmp = pathlib.Path(self._tmp.name)
        self.addCleanup(self._tmp.cleanup)
        self.backend = seeded_backend()
        self.snapshot = snapshot_of(self.backend, self.tmp / "snap")
        self.image = prog.image_view(image_file(self.tmp, 1000), APP.address)

    def run_program(self, **kwargs):
        return prog.program(self.backend, self.image, self.snapshot, **kwargs)

    def test_refuses_without_confirmation_and_writes_nothing(self):
        with self.assertRaises(prog.WriteRefused):
            self.run_program(confirm=False)

        self.assertEqual([op for op in self.backend.ops if op[0] == "write"], [])

    def test_refuses_when_the_live_target_drifted_from_the_snapshot(self):
        self.backend.bytes[APP.address + 5000] ^= 0xFF

        with self.assertRaises(prog.PrecheckFailed) as caught:
            self.run_program(confirm=True)

        self.assertIn("snapshot", str(caught.exception))
        self.assertEqual([op for op in self.backend.ops if op[0] == "write"], [])

    def test_precheck_reads_happen_before_the_write(self):
        self.run_program(confirm=True)

        kinds = [op[0] for op in self.backend.ops]
        write_index = kinds.index("write")
        self.assertGreaterEqual(kinds[:write_index].count("read"), len(hil.REGIONS) - 1)

    def test_writes_the_exact_image_bytes_once_at_the_target_address(self):
        outcome = self.run_program(confirm=True)

        writes = [op for op in self.backend.ops if op[0] == "write"]
        self.assertEqual(writes, [("write", APP.address)])
        self.assertTrue(outcome.ok)
        self.assertEqual(
            self.backend.region_bytes("application")[: self.image.size],
            self.image.path.read_bytes(),
        )

    def test_never_writes_the_rtc_block(self):
        self.run_program(confirm=True)

        rtc = [op for op in self.backend.ops if op[0] == "write" and op[1] == hil.RTC_BACKUP_BASE]
        self.assertEqual(rtc, [])

    def test_a_corrupted_read_back_fails_and_names_the_restore_command(self):
        self.backend.corrupt_after_write = True

        outcome = self.run_program(confirm=True)

        self.assertFalse(outcome.ok)
        self.assertFalse(outcome.truth.span_identical)
        self.assertIn("restore", prog.restore_command(outcome.snapshot_dir))

    def test_a_change_outside_the_erase_page_fails_the_run(self):
        original_write = self.backend.write

        def write_then_corrupt(source, address):
            original_write(source, address)
            self.backend.bytes[APP.address + 3 * prog.FLASH_PAGE_SIZE] ^= 0xFF

        self.backend.write = write_then_corrupt

        outcome = self.run_program(confirm=True)

        self.assertFalse(outcome.ok)
        self.assertEqual(outcome.truth.unexpected_changes, 1)

    def test_page_tail_bytes_erased_by_programming_stay_a_pass(self):
        self.backend.bytes[APP.address + 1500] = 0x5A  # inside the image's last page

        # Re-snapshot so the pre-check sees this byte as the captured state.
        self.snapshot = snapshot_of(self.backend, self.tmp / "snap-tail")
        outcome = self.run_program(confirm=True)

        self.assertTrue(outcome.ok)
        self.assertEqual(outcome.truth.erased_tail, prog.FLASH_PAGE_SIZE - 1000)

    def test_reset_is_optional(self):
        self.run_program(confirm=True)

        self.assertEqual(self.backend.resets, 0)

    def test_reset_reports_regions_that_drifted_after_the_boot_decision(self):
        # The bootloader installing a candidate rewrites the application region.
        self.backend.mutate_after_reset = [(APP.address + 100, 0x00)]

        outcome = self.run_program(confirm=True, reset=True, settle=0, sleep=lambda _: None)

        self.assertEqual(self.backend.resets, 1)
        changed = [item.name for item in outcome.drift if not item.identical]
        self.assertIn("application", changed)
        self.assertNotIn("bootloader", changed)

    def test_reset_without_drift_reports_every_region_unchanged(self):
        outcome = self.run_program(confirm=True, reset=True, settle=0, sleep=lambda _: None)

        # The application was just programmed, so it is the one expected change.
        changed = [item.name for item in outcome.drift if not item.identical]
        self.assertEqual(changed, ["application"])

    def test_an_image_that_changed_after_hashing_is_not_written(self):
        self.image.path.write_bytes(b"\x00" * self.image.size)

        with self.assertRaises(prog.ProgramError):
            self.run_program(confirm=True)

        self.assertEqual([op for op in self.backend.ops if op[0] == "write"], [])

    def test_an_unreadable_optional_rtc_capture_does_not_veto_the_write(self):
        self.backend.fail_reads.add(hil.RTC_BACKUP_BASE)
        self.snapshot = snapshot_of(self.backend, self.tmp / "snap-no-rtc")

        outcome = self.run_program(confirm=True)

        self.assertTrue(outcome.ok)

    def test_an_unreadable_restorable_region_vetoes_the_write(self):
        self.backend.fail_reads.add(APP.address)

        with self.assertRaises(prog.PrecheckFailed):
            self.run_program(confirm=True)

        self.assertEqual([op for op in self.backend.ops if op[0] == "write"], [])

    def test_the_program_log_records_the_image_readback_and_restore_command(self):
        outcome = self.run_program(confirm=True)

        log = json.loads(outcome.log_path.read_text(encoding="utf-8"))
        self.assertEqual(log["image"]["sha256"], self.image.sha256)
        self.assertEqual(log["image"]["region"], "application")
        self.assertTrue(log["readback"]["span_identical"])
        self.assertIn("pandora_hil_state.py restore", log["restore_command"])


class CliTests(unittest.TestCase):
    def setUp(self):
        self._tmp = tempfile.TemporaryDirectory()
        self.tmp = pathlib.Path(self._tmp.name)
        self.addCleanup(self._tmp.cleanup)
        self.backend = seeded_backend()
        self.snapshot = snapshot_of(self.backend, self.tmp / "snap")
        self.image = image_file(self.tmp, 1000)

    def main(self, *arguments):
        stdout, stderr = io.StringIO(), io.StringIO()
        with mock.patch.object(hil, "StFlashBackend", lambda *a, **k: self.backend):
            with contextlib.redirect_stdout(stdout), contextlib.redirect_stderr(stderr):
                code = prog.main(list(arguments))
        return code, stdout.getvalue(), stderr.getvalue()

    def test_plan_writes_nothing(self):
        code, out, _ = self.main(
            "plan",
            "--image",
            str(self.image),
            "--address",
            hex(APP.address),
            "--snapshot",
            str(self.snapshot),
        )

        self.assertEqual(code, 0)
        self.assertIn("plan_ok writes=0", out)
        self.assertEqual([op for op in self.backend.ops if op[0] == "write"], [])

    def test_plan_requires_an_existing_snapshot(self):
        code, _, err = self.main(
            "plan",
            "--image",
            str(self.image),
            "--address",
            hex(APP.address),
            "--snapshot",
            str(self.tmp / "absent"),
        )

        self.assertEqual(code, 2)
        self.assertIn("error:", err)

    def test_run_without_yes_is_refused(self):
        code, _, err = self.main(
            "run",
            "--image",
            str(self.image),
            "--address",
            hex(APP.address),
            "--snapshot",
            str(self.snapshot),
        )

        self.assertEqual(code, 2)
        self.assertIn("refused", err)
        self.assertEqual([op for op in self.backend.ops if op[0] == "write"], [])

    def test_run_refuses_on_a_drifted_target(self):
        self.backend.bytes[APP.address + 10] ^= 0xFF

        code, _, err = self.main(
            "run",
            "--image",
            str(self.image),
            "--address",
            hex(APP.address),
            "--snapshot",
            str(self.snapshot),
            "--yes",
        )

        self.assertEqual(code, 2)
        self.assertIn("precheck_failed", err)

    def test_run_reports_success_only_after_a_matching_read_back(self):
        code, out, _ = self.main(
            "run",
            "--image",
            str(self.image),
            "--address",
            hex(APP.address),
            "--snapshot",
            str(self.snapshot),
            "--yes",
        )

        self.assertEqual(code, 0)
        self.assertIn("program_ok", out)
        self.assertNotIn("program_failed", out)

    def test_run_exits_one_when_the_read_back_mismatches(self):
        self.backend.corrupt_after_write = True

        code, out, _ = self.main(
            "run",
            "--image",
            str(self.image),
            "--address",
            hex(APP.address),
            "--snapshot",
            str(self.snapshot),
            "--yes",
        )

        self.assertEqual(code, 1)
        self.assertIn("program_failed", out)
        self.assertIn("restore_command=", out)


if __name__ == "__main__":
    unittest.main()
