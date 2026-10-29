#!/usr/bin/env python3
"""Cases for the attribute-flag correspondence guard."""

from __future__ import annotations

import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import check_osal_attr_flag_values as guard  # noqa: E402

OSAL = """\
#define XY_OS_THREAD_JOINABLE 0x00000001U
#define XY_OS_THREAD_DETACHED \\
    0x00000000U
#define XY_OS_MUTEX_RECURSIVE    0x00000001U
#define XY_OS_MUTEX_PRIO_INHERIT 0x00000002U
#define XY_OS_MUTEX_ROBUST       0x00000008U
"""

CMSIS = """\
#define osThreadDetached      0x00000000U
#define osThreadJoinable      0x00000001U
#define osMutexRecursive      0x00000001U
#define osMutexPrioInherit    0x00000002U
#define osMutexRobust         0x00000008U
"""


class FlagValueTests(unittest.TestCase):
    def make_tree(self, tmp: str, osal: str = OSAL, cmsis: list[str] | None = None) -> Path:
        root = Path(tmp)
        (root / "components" / "kernel" / "osal").mkdir(parents=True)
        (root / "components" / "kernel" / "osal" / "xy_os.h").write_text(osal)
        for index, text in enumerate(cmsis if cmsis is not None else [CMSIS]):
            d = root / "vendor" / f"copy{index}"
            d.mkdir(parents=True)
            (d / "cmsis_os2.h").write_text(text)
        return root

    def test_matching_values_pass(self):
        with tempfile.TemporaryDirectory() as tmp:
            self.assertEqual([], guard.findings(self.make_tree(tmp)))

    def test_a_drifted_osal_flag_is_named(self):
        osal = OSAL.replace("0x00000008U", "0x00000010U")
        with tempfile.TemporaryDirectory() as tmp:
            problems = guard.findings(self.make_tree(tmp, osal=osal))
            self.assertEqual(1, len(problems))
            self.assertIn("XY_OS_MUTEX_ROBUST", problems[0])

    def test_a_continuation_line_is_still_read(self):
        """XY_OS_THREAD_DETACHED is written across two lines; missing it reports a live flag as stale."""
        with tempfile.TemporaryDirectory() as tmp:
            problems = guard.findings(self.make_tree(tmp))
            self.assertEqual([], problems)

    def test_vendor_copies_that_disagree_are_named_before_anything_else(self):
        other = CMSIS.replace("0x00000008U", "0x00000004U")
        with tempfile.TemporaryDirectory() as tmp:
            problems = guard.findings(self.make_tree(tmp, cmsis=[CMSIS, other]))
            self.assertEqual(1, len(problems))
            self.assertIn("disagree", problems[0])

    def test_a_missing_cmsis_header_says_the_walk_is_broken(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = self.make_tree(tmp, cmsis=[])
            problems = guard.findings(root)
            self.assertEqual(1, len(problems))
            self.assertIn("walk is broken", problems[0])

    def test_a_flag_that_disappeared_is_a_stale_pin(self):
        osal = OSAL.replace("#define XY_OS_MUTEX_ROBUST       0x00000008U\n", "")
        with tempfile.TemporaryDirectory() as tmp:
            problems = guard.findings(self.make_tree(tmp, osal=osal))
            self.assertEqual(1, len(problems))
            self.assertIn("stale", problems[0])


class RealTreeTests(unittest.TestCase):
    def test_the_real_tree_passes(self):
        self.assertEqual([], guard.findings())


if __name__ == "__main__":
    unittest.main(verbosity=2)
