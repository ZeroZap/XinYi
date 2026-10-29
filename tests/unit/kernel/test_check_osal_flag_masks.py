#!/usr/bin/env python3
"""Cases for the event-group mask guard."""

from __future__ import annotations

import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import check_osal_flag_masks as guard  # noqa: E402

GOOD = """\
static int bm_event_mask_valid(uint32_t flags)
{
    return flags != 0U && (flags & XY_OS_EVENT_FLAGS_RESERVED) == 0U;
}

uint32_t xy_os_event_flags_set(xy_os_event_flags_id_t ef_id, uint32_t flags)
{
    if (!bm_event_mask_valid(flags)) return XY_OS_EVENT_FLAGS_ERROR;
    return flags;
}
"""


class BackendTests(unittest.TestCase):
    def run_guard(self, source: str) -> list[str]:
        import tempfile
        with tempfile.TemporaryDirectory() as tmp:
            backend = Path(tmp) / "bm"
            backend.mkdir()
            (backend / "xy_os_bm.c").write_text(source)
            return guard.findings(Path(tmp), {})

    def test_a_helper_that_validates_counts(self):
        self.assertEqual([], self.run_guard(GOOD))

    def test_a_raw_mask_is_named(self):
        source = GOOD.replace("if (!bm_event_mask_valid(flags)) return XY_OS_EVENT_FLAGS_ERROR;", "")
        problems = self.run_guard(source)
        self.assertEqual(1, len(problems))
        self.assertIn("XY_OS_EVENT_FLAGS_RESERVED", problems[0])

    def test_a_reserved_check_without_the_empty_case_is_named(self):
        source = GOOD.replace("flags != 0U && ", "")
        problems = self.run_guard(source)
        self.assertEqual(1, len(problems))
        self.assertIn("empty mask", problems[0])

    def test_an_inline_polarity_works_too(self):
        source = """\
uint32_t xy_os_event_flags_set(xy_os_event_flags_id_t ef_id, uint32_t flags)
{
    if ((flags & XY_OS_EVENT_FLAGS_RESERVED) != 0U) return XY_OS_EVENT_FLAGS_ERROR;
    if (flags == 0U) return XY_OS_EVENT_FLAGS_ERROR;
    return flags;
}
"""
        self.assertEqual([], self.run_guard(source))

    def test_a_pinned_backend_is_skipped(self):
        source = """\
uint32_t xy_os_event_flags_set(xy_os_event_flags_id_t ef_id, uint32_t flags)
{
    return flags;
}
"""
        import tempfile
        with tempfile.TemporaryDirectory() as tmp:
            backend = Path(tmp) / "rtthread"
            backend.mkdir()
            (backend / "xy_os_rtthread.c").write_text(source)
            self.assertEqual([], guard.findings(Path(tmp), {"rtthread": "unbuildable"}))

    def test_a_walk_that_finds_nothing_says_so(self):
        import tempfile
        with tempfile.TemporaryDirectory() as tmp:
            backend = Path(tmp) / "bm"
            backend.mkdir()
            (backend / "xy_os_bm.c").write_text("int not_an_entry(void)\n{\n    return 0;\n}\n")
            problems = guard.findings(Path(tmp), {})
            self.assertEqual(1, len(problems))
            self.assertIn("walk is broken", problems[0])


class RealTreeTests(unittest.TestCase):
    def test_the_real_tree_passes_and_the_walk_reaches_targets(self):
        self.assertGreaterEqual(len(list(guard.BACKENDS.glob("*/xy_os_*.c"))), 4)
        self.assertEqual([], guard.findings())


if __name__ == "__main__":
    unittest.main(verbosity=2)
