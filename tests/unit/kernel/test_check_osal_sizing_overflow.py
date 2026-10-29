#!/usr/bin/env python3
"""Cases for the sizing-overflow guard."""

from __future__ import annotations

import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import check_osal_sizing_overflow as guard  # noqa: E402

GUARDED = """\
xy_os_mempool_id_t xy_os_mempool_new(uint32_t block_count, uint32_t block_size, const void *attr)
{
    if (block_count > UINT32_MAX / block_size) return NULL;
    return NULL;
}
"""


class SizingTests(unittest.TestCase):
    def run_guard(self, source: str, backend: str = "bm", pinned: dict | None = None) -> list[str]:
        with tempfile.TemporaryDirectory() as tmp:
            d = Path(tmp) / backend
            d.mkdir()
            (d / "xy_os_bm.c").write_text(source)
            return guard.findings(Path(tmp), pinned or {})

    def test_a_division_guard_passes(self):
        self.assertEqual([], self.run_guard(GUARDED))

    def test_a_product_without_a_guard_is_named(self):
        source = GUARDED.replace("if (block_count > UINT32_MAX / block_size) return NULL;",
                                 "uint32_t needed = block_count * block_size;")
        problems = self.run_guard(source)
        self.assertEqual(1, len(problems))
        self.assertIn("overflows", problems[0])

    def test_size_max_counts_too(self):
        source = GUARDED.replace("UINT32_MAX / block_size", "SIZE_MAX / block_size")
        self.assertEqual([], self.run_guard(source))

    def test_a_guard_in_a_helper_counts(self):
        source = """\
static int bm_sized(uint32_t n, uint32_t s)
{
    return n <= UINT32_MAX / s;
}
xy_os_msgqueue_id_t xy_os_msgqueue_new(uint32_t msg_count, uint32_t msg_size, const void *attr)
{
    if (!bm_sized(msg_count, msg_size)) return NULL;
    return NULL;
}
"""
        self.assertEqual([], self.run_guard(source))

    def test_a_pinned_constructor_is_skipped(self):
        source = GUARDED.replace("if (block_count > UINT32_MAX / block_size) return NULL;", "")
        self.assertEqual([], self.run_guard(source, "freertos",
                                            {("freertos", "xy_os_mempool_new"): "delegates"}))

    def test_a_walk_that_finds_nothing_says_so(self):
        problems = self.run_guard("int unrelated(void)\n{\n    return 0;\n}\n")
        self.assertEqual(1, len(problems))
        self.assertIn("walk is broken", problems[0])


class RealTreeTests(unittest.TestCase):
    def test_the_real_tree_passes(self):
        self.assertGreaterEqual(len(list(guard.BACKENDS.glob("*/xy_os_*.c"))), 4)
        self.assertEqual([], guard.findings())


if __name__ == "__main__":
    unittest.main(verbosity=2)
