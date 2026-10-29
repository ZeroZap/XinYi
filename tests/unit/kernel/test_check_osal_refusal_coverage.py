#!/usr/bin/env python3
"""Cases for the refusal-coverage guard."""

from __future__ import annotations

import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import check_osal_refusal_coverage as guard  # noqa: E402

MEMPOOL_OK = """\
xy_os_status_t xy_os_mempool_free(xy_os_mempool_id_t mp, void *block)
{
    if (!in_pool(block)) return XY_OS_ERROR_PARAMETER;
    if (already_free(block)) return XY_OS_ERROR_RESOURCE;
    return XY_OS_OK;
}
"""

MEMPOOL_FLAT = """\
xy_os_status_t xy_os_mempool_free(xy_os_mempool_id_t mp, void *block)
{
    if (!in_pool(block)) return XY_OS_ERROR_PARAMETER;
    return XY_OS_OK;
}
"""

MUTEX_OK = """\
xy_os_status_t xy_os_mutex_delete(xy_os_mutex_id_t m)
{
    if (held(m)) return XY_OS_ERROR_RESOURCE;
    return XY_OS_OK;
}
"""

MUTEX_BLIND = """\
xy_os_status_t xy_os_mutex_delete(xy_os_mutex_id_t m)
{
    return XY_OS_OK;
}
"""


class CoverageTests(unittest.TestCase):
    def tree(self, tmp: str, files: dict[str, str]) -> Path:
        root = Path(tmp)
        for backend, source in files.items():
            d = root / backend
            d.mkdir(parents=True, exist_ok=True)
            (d / "xy_os_x.c").write_text(source)
        return root

    def run_guard(self, files: dict[str, str], pinned: dict | None = None) -> list[str]:
        with tempfile.TemporaryDirectory() as tmp:
            return guard.findings(self.tree(tmp, files), pinned or {})

    def test_everything_present_passes(self):
        self.assertEqual([], self.run_guard({"bm": MEMPOOL_OK + MUTEX_OK}))

    def test_a_flattened_pool_refusal_is_named(self):
        problems = self.run_guard({"bm": MEMPOOL_FLAT + MUTEX_OK})
        self.assertEqual(1, len(problems))
        self.assertIn("xy_os_mempool_free", problems[0])
        self.assertIn("XY_OS_ERROR_RESOURCE", problems[0])

    def test_deleting_a_held_mutex_without_saying_so_is_named(self):
        """The condition that was actually live in the bare-metal backend."""
        problems = self.run_guard({"bm": MEMPOOL_OK + MUTEX_BLIND})
        self.assertEqual(1, len(problems))
        self.assertIn("xy_os_mutex_delete", problems[0])
        self.assertIn("still held", problems[0])

    def test_a_pin_is_per_entry_not_per_backend(self):
        """Pinning one entry must not silence another entry in the same backend file."""
        files = {"bm": MEMPOOL_FLAT + MUTEX_BLIND}
        problems = self.run_guard(files, {("bm", "xy_os_mempool_free"): "no"})
        self.assertEqual(1, len(problems))
        self.assertIn("xy_os_mutex_delete", problems[0])

    def test_a_tree_with_none_of_the_entries_says_the_walk_is_broken(self):
        problems = self.run_guard({"bm": "int unrelated(void)\n{\n    return 0;\n}\n"})
        self.assertEqual(1, len(problems))
        self.assertIn("walk is broken", problems[0])

    def test_pinning_everything_is_reported(self):
        problems = self.run_guard({"bm": MEMPOOL_OK + MUTEX_OK},
                                  {("bm", "xy_os_mempool_free"): "x", ("bm", "xy_os_mutex_delete"): "x"})
        self.assertEqual(1, len(problems))
        self.assertIn("nothing was actually checked", problems[0])


class RealTreeTests(unittest.TestCase):
    def test_the_real_tree_passes(self):
        self.assertEqual([], guard.findings())


if __name__ == "__main__":
    unittest.main(verbosity=2)
