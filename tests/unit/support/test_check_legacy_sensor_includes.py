#!/usr/bin/env python3
"""Self-test for check_legacy_sensor_includes.py.

The decisive case is the real one: the *pre-fix* `xy_bmi270.h` is pulled back out of git and must be
reported, while the current tree must be clean. Everything else pins down the edges that would
otherwise turn this into a rule that reports files it should not - a commented-out include (which
the tree actually contains), a misfiled bus shim, and a pin whose include has gone away.
"""

from __future__ import annotations

import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from check_legacy_sensor_includes import (  # noqa: E402
    LEGACY_FRAMEWORK,
    REPO_ROOT,
    findings,
    legacy_includes,
)

BMI270 = "components/drivers/sensor/motion/bmi270/xy_bmi270.h"


class IncludeExtraction(unittest.TestCase):
    def test_legacy_header_is_reported(self) -> None:
        self.assertEqual(["xy_sensor.h"], legacy_includes('#include "xy_sensor.h"\n'))

    def test_misfiled_bus_shim_is_not_the_framework(self) -> None:
        """`xy_i2c.h` lives in the same directory and is the only copy in the tree."""
        self.assertEqual([], legacy_includes('#include "xy_i2c.h"\n#include "xy_spi.h"\n'))

    def test_line_comment_is_not_an_include(self) -> None:
        self.assertEqual([], legacy_includes('// #include "xy_sensor.h"\n'))

    def test_block_comment_is_not_an_include(self) -> None:
        self.assertEqual([], legacy_includes('/* #include "xy_sensor.h" */\nint x;\n'))

    def test_include_through_a_subdirectory_is_reported(self) -> None:
        self.assertEqual(["xy_sensor_device.h"],
                         legacy_includes('#include "../inc/xy_sensor_device.h"\n'))

    def test_string_containing_an_include_is_not_an_include(self) -> None:
        self.assertEqual([], legacy_includes('static const char *s = "#include \\"xy_sensor.h\\"";\n'))

    def test_each_header_is_reported_once(self) -> None:
        text = '#include "xy_sensor.h"\n#include "xy_sensor.h"\n'
        self.assertEqual(["xy_sensor.h"], legacy_includes(text))


class Findings(unittest.TestCase):
    def _tree(self, files: dict[str, str]) -> Path:
        directory = Path(tempfile.mkdtemp())
        for relative, text in files.items():
            path = directory / relative
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(text, encoding="utf-8")
        sensor = directory / "components" / "sensor"
        for relatives in LEGACY_FRAMEWORK.values():
            path = sensor / relatives[0]
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text("#pragma once\n", encoding="utf-8")
        return directory

    def test_unpinned_consumer_is_reported(self) -> None:
        root = self._tree({"components/drivers/foo/xy_foo.h": '#include "xy_sensor.h"\n'})
        problems = findings(root, pins={})
        self.assertTrue(any("xy_foo.h" in p and "canonical" in p for p in problems), problems)

    def test_pinned_consumer_is_clean(self) -> None:
        root = self._tree({"components/drivers/foo/xy_foo.h": '#include "xy_sensor.h"\n'})
        self.assertEqual([], findings(root, pins={"components/drivers/foo/xy_foo.h": "why"}))

    def test_pin_without_a_reason_is_reported(self) -> None:
        root = self._tree({"components/drivers/foo/xy_foo.h": '#include "xy_sensor.h"\n'})
        problems = findings(root, pins={"components/drivers/foo/xy_foo.h": "  "})
        self.assertTrue(any("without stating why" in p for p in problems), problems)

    def test_expired_pin_is_reported(self) -> None:
        root = self._tree({"components/drivers/foo/xy_foo.h": "int x;\n"})
        problems = findings(root, pins={"components/drivers/foo/xy_foo.h": "why"})
        self.assertTrue(any("expired" in p for p in problems), problems)

    def test_pin_for_a_missing_file_is_reported(self) -> None:
        root = self._tree({"components/drivers/foo/xy_foo.h": "int x;\n"})
        problems = findings(root, pins={"components/drivers/foo/gone.h": "why"})
        self.assertTrue(any("no longer exists" in p for p in problems), problems)

    def test_the_framework_itself_is_exempt(self) -> None:
        root = self._tree({
            "components/sensor/inc/xy_sensor_extra.h": '#include "xy_sensor.h"\n',
            "components/drivers/foo/xy_foo.c": "int unrelated;\n",
        })
        self.assertEqual([], findings(root, pins={}))

    def test_a_renamed_legacy_header_is_reported(self) -> None:
        root = self._tree({})
        (root / "components" / "sensor" / "inc" / "xy_sensor.h").unlink()
        problems = findings(root, pins={})
        self.assertTrue(any("no longer exists under components/sensor" in p for p in problems),
                        problems)


class CurrentTree(unittest.TestCase):
    def test_current_tree_is_clean(self) -> None:
        proc = subprocess.run(
            [sys.executable, str(Path(__file__).resolve().parent /
                                 "check_legacy_sensor_includes.py")],
            capture_output=True, text=True, cwd=str(REPO_ROOT))
        self.assertEqual(0, proc.returncode, proc.stdout + proc.stderr)
        self.assertIn("legacy_sensor_includes ok", proc.stdout)

    def test_pre_fix_bmi270_header_is_flagged(self) -> None:
        """The rule's subject existed: some revision of the file includes the legacy framework."""
        if subprocess.run(["git", "rev-parse", "--git-dir"], cwd=str(REPO_ROOT),
                          capture_output=True, text=True).returncode != 0:
            # `canonical_host_clean_checkout` runs this suite from a `git archive` export, which
            # has no history to read. The evidence still runs in the repository.
            self.skipTest("no git metadata in this tree (clean source export)")
        log = subprocess.run(["git", "log", "--format=%H", "--", BMI270],
                             capture_output=True, text=True, cwd=str(REPO_ROOT))
        revisions = [line.strip() for line in log.stdout.splitlines() if line.strip()]
        self.assertTrue(revisions, f"no history for {BMI270}")
        flagged = 0
        for revision in revisions:
            show = subprocess.run(["git", "show", f"{revision}:{BMI270}"],
                                  capture_output=True, text=True, cwd=str(REPO_ROOT))
            if show.returncode != 0:
                continue
            if legacy_includes(show.stdout):
                flagged += 1
                break
        self.assertEqual(1, flagged, "no historical revision includes the legacy framework; the "
                                     "rule never had a real subject")


if __name__ == "__main__":
    unittest.main(verbosity=2)
