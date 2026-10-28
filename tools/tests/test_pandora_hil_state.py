"""Host coverage for tools/pandora_hil_state.py (no debugger, no hardware)."""

import datetime
import hashlib
import json
import pathlib
import sys
import tempfile
import unittest

TOOLS = pathlib.Path(__file__).resolve().parent.parent
if str(TOOLS) not in sys.path:
    sys.path.insert(0, str(TOOLS))

import pandora_hil_state as hil  # noqa: E402

REPO_ROOT = TOOLS.parent


def blank_regions(seed: dict[str, bytes] | None = None) -> hil.MemoryBackend:
    backend = hil.MemoryBackend()
    for region in hil.REGIONS:
        payload = b"\xff" * region.size
        if seed and region.name in seed:
            payload = seed[region.name]
        backend.seed(region.address, payload)
    return backend


class RegionMapTests(unittest.TestCase):
    def test_region_map_matches_board_linker_scripts(self):
        problems = hil.check_region_map(REPO_ROOT)

        self.assertEqual(problems, [])

    def test_flash_regions_tile_the_full_512k_part(self):
        flash = sorted(
            (region for region in hil.REGIONS if region.name != "rtc_backup"),
            key=lambda region: region.address,
        )

        self.assertEqual(flash[0].address, hil.FLASH_BASE)
        for previous, current in zip(flash, flash[1:]):
            self.assertEqual(previous.end, current.address)
        self.assertEqual(flash[-1].end, hil.FLASH_BASE + hil.FLASH_SIZE)

    def test_rtc_backup_block_covers_the_fota_handshake_slots(self):
        rtc = hil.region_by_name("rtc_backup")

        self.assertEqual(rtc.address, 0x40002850)
        self.assertEqual(rtc.size, 0x80)
        for slot in (1, 2):
            self.assertLessEqual((slot + 1) * 4, rtc.size)

    def test_rtc_backup_is_captured_but_never_restored(self):
        rtc = hil.region_by_name("rtc_backup")

        self.assertFalse(rtc.restorable)
        self.assertTrue(rtc.optional)

    def test_layout_check_reports_an_invented_region_shift(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = pathlib.Path(temporary)
            board = root / "boards" / hil.BOARD
            board.mkdir(parents=True)
            for name in (
                "STM32L475VETX_BOOTLOADER.ld",
                "STM32L475VETX_FLASH.ld",
                "STM32L475VETX_FOTA_APP.ld",
                "pandora_fota_flash.h",
                "pandora_fota_install_flash.h",
                "pandora_fota_boot_flow.h",
            ):
                (board / name).write_text(
                    (REPO_ROOT / "boards" / hil.BOARD / name).read_text(encoding="utf-8"),
                    encoding="utf-8",
                )
            app_ld = board / "STM32L475VETX_FOTA_APP.ld"
            app_ld.write_text(
                app_ld.read_text(encoding="utf-8").replace(
                    "ORIGIN = 0x08008000, LENGTH = 472K",
                    "ORIGIN = 0x08004000, LENGTH = 472K",
                ),
                encoding="utf-8",
            )

            problems = hil.check_region_map(root)

            self.assertTrue(
                any("application" in problem for problem in problems), problems
            )


class SnapshotTests(unittest.TestCase):
    def test_snapshot_records_hash_and_nonzero_accounting(self):
        backend = blank_regions({"bootloader": b"\x00\x01" + b"\xff" * (0x8000 - 2)})
        with tempfile.TemporaryDirectory() as temporary:
            out = pathlib.Path(temporary) / "state"

            manifest = hil.take_snapshot(
                backend,
                out,
                label="unit",
                git_head="deadbeef",
                utc=datetime.datetime(2026, 10, 28, 9, 0, tzinfo=datetime.timezone.utc),
            )

            self.assertTrue((out / hil.MANIFEST_NAME).exists())
            entries = {entry["name"]: entry for entry in manifest["regions"]}
            self.assertEqual(entries["bootloader"]["size"], 0x8000)
            self.assertEqual(entries["bootloader"]["nonzero_bytes"], 0x7FFF)
            self.assertEqual(
                entries["bootloader"]["sha256"],
                hashlib.sha256(b"\x00\x01" + b"\xff" * (0x8000 - 2)).hexdigest(),
            )
            # Erased flash reads as 0xFF, so an untouched region is 100% nonzero;
            # the counter is there to spot a region that was blanked to 0x00 by a
            # failed write, not to detect erasure.
            self.assertEqual(entries["application"]["nonzero_bytes"], 0x76000)
            self.assertTrue((out / entries["application"]["file"]).exists())
            self.assertEqual(manifest["git_head"], "deadbeef")

    def test_snapshot_captures_every_declared_region_in_address_order(self):
        backend = blank_regions()
        with tempfile.TemporaryDirectory() as temporary:
            hil.take_snapshot(backend, pathlib.Path(temporary) / "state")

            self.assertEqual(
                [address for address, _ in backend.reads],
                [region.address for region in hil.REGIONS],
            )

    def test_unreadable_optional_region_is_recorded_not_fatal(self):
        backend = blank_regions()
        backend.fail_reads.add(hil.region_by_name("rtc_backup").address)
        with tempfile.TemporaryDirectory() as temporary:
            manifest = hil.take_snapshot(backend, pathlib.Path(temporary) / "state")

            entries = {entry["name"]: entry for entry in manifest["regions"]}
            self.assertFalse(entries["rtc_backup"]["captured"])
            self.assertIn("failed", entries["rtc_backup"]["error"])
            self.assertTrue(entries["application"]["captured"])

    def test_unreadable_flash_region_fails_closed_without_manifest(self):
        backend = blank_regions()
        backend.fail_reads.add(hil.region_by_name("fota_metadata").address)
        with tempfile.TemporaryDirectory() as temporary:
            out = pathlib.Path(temporary) / "state"

            with self.assertRaises(hil.SnapshotError):
                hil.take_snapshot(backend, out)

            self.assertFalse((out / hil.MANIFEST_NAME).exists())

    def test_short_capture_is_rejected(self):
        backend = blank_regions()
        backend.short_reads[hil.region_by_name("board_reserved").address] = 16
        with tempfile.TemporaryDirectory() as temporary:
            with self.assertRaises(hil.SnapshotError):
                hil.take_snapshot(backend, pathlib.Path(temporary) / "state")


class CompareRegionTests(unittest.TestCase):
    def test_equal_payloads_compare_identically(self):
        self.assertEqual(hil.compare_region(b"\xff" * 8, b"\xff" * 8), (True, None))

    def test_difference_is_located_to_the_first_byte(self):
        self.assertEqual(
            hil.compare_region(b"\xff\x00\xff", b"\xff\x01\xff"), (False, 1)
        )

    def test_truncated_read_is_a_mismatch_not_an_index_error(self):
        identical, offset = hil.compare_region(b"\xff" * 8, b"\xff" * 4)

        self.assertFalse(identical)
        self.assertEqual(offset, 4)

    def test_overlong_read_is_a_mismatch(self):
        identical, offset = hil.compare_region(b"\xff" * 4, b"\xff" * 8 + b"\x00")

        self.assertFalse(identical)
        self.assertEqual(offset, 4)


class VerifyTests(unittest.TestCase):
    def test_identical_target_verifies(self):
        with tempfile.TemporaryDirectory() as temporary:
            out = pathlib.Path(temporary) / "state"
            hil.take_snapshot(blank_regions(), out)

            comparisons = hil.verify_snapshot(
                blank_regions(), out, pathlib.Path(temporary) / "scratch"
            )

            self.assertTrue(all(comparison.identical for comparison in comparisons))
            self.assertEqual(len(comparisons), len(hil.REGIONS))

    def test_single_flipped_byte_is_located(self):
        with tempfile.TemporaryDirectory() as temporary:
            out = pathlib.Path(temporary) / "state"
            hil.take_snapshot(blank_regions(), out)
            drifted = blank_regions(
                {"application": b"\xff" * 0x1000 + b"\x00" + b"\xff" * (0x76000 - 0x1001)}
            )

            comparisons = hil.verify_snapshot(drifted, out, pathlib.Path(temporary) / "scratch")

            failed = [comparison for comparison in comparisons if not comparison.identical]
            self.assertEqual([comparison.name for comparison in failed], ["application"])
            self.assertEqual(failed[0].first_diff, 0x1000)

    def test_flash_erased_to_blank_is_reported_as_mismatch(self):
        with tempfile.TemporaryDirectory() as temporary:
            out = pathlib.Path(temporary) / "state"
            hil.take_snapshot(blank_regions({"bootloader": b"\x11" * 0x8000}), out)

            comparisons = hil.verify_snapshot(
                blank_regions(), out, pathlib.Path(temporary) / "scratch"
            )

            boot = {comparison.name: comparison for comparison in comparisons}["bootloader"]
            self.assertFalse(boot.identical)
            self.assertEqual(boot.first_diff, 0)

    def test_unreadable_region_on_verify_is_a_failure_not_a_crash(self):
        with tempfile.TemporaryDirectory() as temporary:
            out = pathlib.Path(temporary) / "state"
            hil.take_snapshot(blank_regions(), out)
            offline = blank_regions()
            offline.fail_reads.add(hil.region_by_name("application").address)

            comparisons = hil.verify_snapshot(offline, out, pathlib.Path(temporary) / "scratch")

            app = {comparison.name: comparison for comparison in comparisons}["application"]
            self.assertFalse(app.identical)
            self.assertIn("re-read failed", app.detail)

    def test_missing_manifest_is_rejected(self):
        with tempfile.TemporaryDirectory() as temporary:
            with self.assertRaises(hil.SnapshotError):
                hil.verify_snapshot(blank_regions(), pathlib.Path(temporary) / "absent")

    def test_manifest_with_wrong_board_is_rejected(self):
        with tempfile.TemporaryDirectory() as temporary:
            out = pathlib.Path(temporary) / "state"
            hil.take_snapshot(blank_regions(), out)
            manifest_path = out / hil.MANIFEST_NAME
            manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
            manifest["board"] = "some_other_board"
            manifest_path.write_text(json.dumps(manifest), encoding="utf-8")

            with self.assertRaises(hil.SnapshotError):
                hil.verify_snapshot(blank_regions(), out, pathlib.Path(temporary) / "scratch")


class RestoreTests(unittest.TestCase):
    def test_restore_refuses_without_confirmation_and_writes_nothing(self):
        with tempfile.TemporaryDirectory() as temporary:
            out = pathlib.Path(temporary) / "state"
            hil.take_snapshot(blank_regions(), out)
            target = blank_regions({"application": b"\x00" * 0x76000})

            with self.assertRaises(hil.RestoreRefused):
                hil.restore_snapshot(target, out, confirm=False)

            self.assertEqual(target.writes, [])

    def test_restore_writes_flash_regions_in_address_order_and_verifies_readback(self):
        with tempfile.TemporaryDirectory() as temporary:
            out = pathlib.Path(temporary) / "state"
            hil.take_snapshot(blank_regions({"application": b"\x42" * 0x76000}), out)
            target = blank_regions({"application": b"\x00" * 0x76000})

            results = hil.restore_snapshot(
                target, out, confirm=True, scratch_dir=pathlib.Path(temporary) / "scratch"
            )

            self.assertEqual(
                [address for address, _ in target.writes],
                [
                    hil.region_by_name("bootloader").address,
                    hil.region_by_name("application").address,
                    hil.region_by_name("fota_metadata").address,
                    hil.region_by_name("board_reserved").address,
                ],
            )
            self.assertTrue(all(result.identical for result in results))
            self.assertEqual(len(results), 4)

    def test_restore_never_writes_the_rtc_backup_block(self):
        with tempfile.TemporaryDirectory() as temporary:
            out = pathlib.Path(temporary) / "state"
            hil.take_snapshot(blank_regions(), out)

            steps = hil.restore_plan(out)

            self.assertNotIn("rtc_backup", [step.name for step in steps])

    def test_restore_stops_and_reports_when_readback_differs(self):
        with tempfile.TemporaryDirectory() as temporary:
            out = pathlib.Path(temporary) / "state"
            hil.take_snapshot(blank_regions({"application": b"\x42" * 0x76000}), out)
            target = blank_regions({"application": b"\x00" * 0x76000})
            target.corrupt_after_write.add(hil.region_by_name("application").address)

            results = hil.restore_snapshot(
                target, out, confirm=True, scratch_dir=pathlib.Path(temporary) / "scratch"
            )

            self.assertEqual([result.name for result in results], ["bootloader", "application"])
            self.assertFalse(results[-1].identical)

    def test_restore_rejects_a_snapshot_file_that_drifted_from_its_hash(self):
        with tempfile.TemporaryDirectory() as temporary:
            out = pathlib.Path(temporary) / "state"
            manifest = hil.take_snapshot(blank_regions(), out)
            application = next(
                entry for entry in manifest["regions"] if entry["name"] == "application"
            )
            (out / application["file"]).write_bytes(b"\x00" * application["size"])

            with self.assertRaises(hil.SnapshotError):
                hil.restore_snapshot(blank_regions(), out, confirm=True)

    def test_restore_requires_at_least_one_restorable_region(self):
        with tempfile.TemporaryDirectory() as temporary:
            out = pathlib.Path(temporary) / "state"
            hil.take_snapshot(blank_regions(), out, regions=(hil.region_by_name("rtc_backup"),))

            with self.assertRaises(hil.SnapshotError):
                hil.restore_snapshot(blank_regions(), out, confirm=True)


def _completed(returncode: int = 0, stdout: str = "", stderr: str = "") -> object:
    class Completed:
        pass

    completed = Completed()
    completed.returncode = returncode  # type: ignore[attr-defined]
    completed.stdout = stdout  # type: ignore[attr-defined]
    completed.stderr = stderr  # type: ignore[attr-defined]
    return completed


class BackendCommandTests(unittest.TestCase):
    def test_read_uses_st_flash_read_with_hex_address_and_size(self):
        calls = []

        def runner(argv, capture_output, text, timeout):
            calls.append(argv)
            pathlib.Path(argv[2]).write_bytes(b"\xff" * 0x10)
            return _completed()

        backend = hil.StFlashBackend(runner=runner)
        with tempfile.TemporaryDirectory() as temporary:
            destination = pathlib.Path(temporary) / "region.bin"

            backend.read(0x08000000, 0x10, destination)

            self.assertEqual(calls[0][:2], ["st-flash", "read"])
            # st-flash 1.8.0 wants the destination path before addr/size.
            self.assertEqual(calls[0][2], str(destination))
            self.assertEqual(calls[0][3], "0x8000000")
            self.assertEqual(calls[0][4], "0x10")
            self.assertEqual(destination.read_bytes(), b"\xff" * 0x10)

    def test_read_failure_raises_backend_error(self):
        def runner(argv, capture_output, text, timeout):
            return _completed(returncode=1, stderr="unknown chip id")

        backend = hil.StFlashBackend(runner=runner)
        with tempfile.TemporaryDirectory() as temporary:
            with self.assertRaises(hil.BackendError):
                backend.read(0x08000000, 0x10, pathlib.Path(temporary) / "region.bin")

    def test_short_read_is_a_backend_error(self):
        def runner(argv, capture_output, text, timeout):
            pathlib.Path(argv[2]).write_bytes(b"\xff" * 4)
            return _completed()

        backend = hil.StFlashBackend(runner=runner)
        with tempfile.TemporaryDirectory() as temporary:
            with self.assertRaises(hil.BackendError):
                backend.read(0x08000000, 0x10, pathlib.Path(temporary) / "region.bin")

    def test_write_argv_puts_the_address_last(self):
        calls = []

        def runner(argv, capture_output, text, timeout):
            calls.append(argv)
            return _completed()

        backend = hil.StFlashBackend(runner=runner)
        with tempfile.TemporaryDirectory() as temporary:
            source = pathlib.Path(temporary) / "region.bin"
            source.write_bytes(b"\xff" * 8)

            backend.write(source, 0x08008000)

            self.assertEqual(calls[0][:2], ["st-flash", "write"])
            self.assertEqual(calls[0][2], str(source))
            self.assertEqual(calls[0][3], "0x8008000")


class CliTests(unittest.TestCase):
    def _snapshot(self, temporary: str, label: str = "cli") -> pathlib.Path:
        out = pathlib.Path(temporary) / "state"
        hil.take_snapshot(blank_regions(), out, label=label)
        return out

    def test_verify_cli_returns_zero_on_identical_state(self):
        with tempfile.TemporaryDirectory() as temporary:
            out = self._snapshot(temporary)
            original = hil.StFlashBackend
            hil.StFlashBackend = lambda: blank_regions()  # type: ignore[assignment]
            try:
                self.assertEqual(
                    hil.main(["verify", "--snapshot", str(out)], repo_root=REPO_ROOT), 0
                )
            finally:
                hil.StFlashBackend = original  # type: ignore[assignment]

    def test_verify_cli_returns_one_on_drift(self):
        with tempfile.TemporaryDirectory() as temporary:
            out = self._snapshot(temporary)
            original = hil.StFlashBackend
            hil.StFlashBackend = lambda: blank_regions(  # type: ignore[assignment]
                {"application": b"\x00" * 0x76000}
            )
            try:
                self.assertEqual(
                    hil.main(["verify", "--snapshot", str(out)], repo_root=REPO_ROOT), 1
                )
            finally:
                hil.StFlashBackend = original  # type: ignore[assignment]

    def test_restore_cli_returns_two_without_yes(self):
        with tempfile.TemporaryDirectory() as temporary:
            out = self._snapshot(temporary)
            original = hil.StFlashBackend
            target = blank_regions()
            hil.StFlashBackend = lambda: target  # type: ignore[assignment]
            try:
                self.assertEqual(
                    hil.main(["restore", "--snapshot", str(out)], repo_root=REPO_ROOT), 2
                )
                self.assertEqual(target.writes, [])
            finally:
                hil.StFlashBackend = original  # type: ignore[assignment]

    def test_check_layout_cli_passes_on_the_real_repo(self):
        self.assertEqual(hil.main(["check-layout"], repo_root=REPO_ROOT), 0)


if __name__ == "__main__":
    unittest.main()
