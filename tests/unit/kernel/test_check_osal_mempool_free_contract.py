#!/usr/bin/env python3
"""Cases for the pool-free contract guard."""

from __future__ import annotations

import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import check_osal_mempool_free_contract as guard  # noqa: E402

BOTH = """\
xy_os_status_t xy_os_mempool_free(xy_os_mempool_id_t mp, void *block)
{
    if (!in_pool(block)) return XY_OS_ERROR_PARAMETER;
    if (already_free(block)) return XY_OS_ERROR_RESOURCE;
    return XY_OS_OK;
}
"""


class ContractTests(unittest.TestCase):
    def run_guard(self, source: str, backend: str = "bm", pinned: dict | None = None) -> list[str]:
        with tempfile.TemporaryDirectory() as tmp:
            d = Path(tmp) / backend
            d.mkdir()
            (d / "xy_os_bm.c").write_text(source)
            return guard.findings(Path(tmp), pinned or {})

    def test_both_refusals_pass(self):
        self.assertEqual([], self.run_guard(BOTH))

    def test_a_flattened_refusal_is_named(self):
        source = BOTH.replace("if (already_free(block)) return XY_OS_ERROR_RESOURCE;",
                              "if (already_free(block)) return XY_OS_ERROR_PARAMETER;")
        problems = self.run_guard(source)
        self.assertEqual(1, len(problems))
        self.assertIn("XY_OS_ERROR_RESOURCE", problems[0])

    def test_a_missing_pointer_check_is_named(self):
        source = BOTH.replace("if (!in_pool(block)) return XY_OS_ERROR_PARAMETER;", "")
        problems = self.run_guard(source)
        self.assertEqual(1, len(problems))
        self.assertIn("XY_OS_ERROR_PARAMETER", problems[0])

    def test_a_pinned_backend_is_skipped(self):
        """Pinned and flattened is accepted, but only while something else is still being checked."""
        source = BOTH.replace("if (already_free(block)) return XY_OS_ERROR_RESOURCE;", "")
        with tempfile.TemporaryDirectory() as tmp:
            for backend, text in (("rt", source), ("bm", BOTH)):
                d = Path(tmp) / backend
                d.mkdir()
                (d / "xy_os_bm.c").write_text(text)
            self.assertEqual([], guard.findings(Path(tmp), {"rt": "unbuildable"}))

    def test_a_backend_without_the_function_is_not_counted(self):
        self.assertEqual(1, len(self.run_guard("int unrelated(void)\n{\n    return 0;\n}\n")))

    def test_pinning_everything_is_reported(self):
        self.assertEqual(1, len(self.run_guard(BOTH, "bm", {"bm": "why not"})))


class RealTreeTests(unittest.TestCase):
    def test_the_real_tree_passes(self):
        self.assertEqual([], guard.findings())


if __name__ == "__main__":
    unittest.main(verbosity=2)
