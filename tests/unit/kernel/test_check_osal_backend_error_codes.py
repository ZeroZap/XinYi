#!/usr/bin/env python3
"""Cases for the backend error-code census."""

from __future__ import annotations

import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import check_osal_backend_error_codes as guard  # noqa: E402

# A realistic backend carries at least one flag-word entry, and the guard treats a tree with none as a
# broken walk rather than as a clean result.
FLAG_ENTRY = """\
uint32_t xy_os_event_flags_get(xy_os_event_flags_id_t ef)
{
    if (!ef) return XY_OS_EVENT_FLAGS_ERROR;
    return 0U;
}

"""

SAME = FLAG_ENTRY + """\
static xy_os_status_t helper(BaseType_t s)
{
    if (s == 0) return XY_OS_ERROR_TIMEOUT;
    return XY_OS_OK;
}

xy_os_status_t xy_os_msgqueue_get(xy_os_msgqueue_id_t q, void *m, uint32_t *p, uint32_t t)
{
    if (!q) return XY_OS_ERROR_PARAMETER;
    return helper(1);
}
"""

DIFFERENT = FLAG_ENTRY + """\
xy_os_status_t xy_os_msgqueue_get(xy_os_msgqueue_id_t q, void *m, uint32_t *p, uint32_t t)
{
    if (!q) return XY_OS_ERROR_PARAMETER;
    return XY_OS_OK;
}
"""


class CensusTests(unittest.TestCase):
    def tree(self, tmp: str, left: str, right: str) -> Path:
        root = Path(tmp)
        for backend, source in (("baremetal", left), ("freertos", right)):
            d = root / backend
            d.mkdir(parents=True)
            (d / "xy_os_x.c").write_text(source)
        return root

    def run_guard(self, left: str, right: str, pinned: dict | None = None) -> list[str]:
        with tempfile.TemporaryDirectory() as tmp:
            return guard.findings(self.tree(tmp, left, right), pinned=pinned if pinned is not None else {})

    def test_matching_backends_pass(self):
        self.assertEqual([], self.run_guard(SAME, SAME))

    def test_a_difference_is_named_with_the_entry(self):
        problems = self.run_guard(SAME, DIFFERENT)
        self.assertEqual(1, len(problems))
        self.assertIn("xy_os_msgqueue_get", problems[0])
        self.assertIn("XY_OS_ERROR_TIMEOUT", problems[0])

    def test_a_helper_is_followed_one_level(self):
        """One side says TIMEOUT through a local helper, the other inline. Without following the helper the
        two look different when they are the same answer."""
        inline = SAME.replace("return helper(1);", "return XY_OS_ERROR_TIMEOUT;")
        self.assertEqual([], self.run_guard(SAME, inline))

    def test_a_refusal_only_one_side_can_express_is_still_reported(self):
        no_helper = SAME.replace("return helper(1);", "return XY_OS_OK;")
        problems = self.run_guard(SAME, no_helper)
        self.assertEqual(1, len(problems))
        self.assertIn("XY_OS_ERROR_TIMEOUT", problems[0])

    def test_a_pin_silences_the_entry(self):
        self.assertEqual([], self.run_guard(SAME, DIFFERENT, {"xy_os_msgqueue_get": "no scheduler"}))

    def test_a_pin_for_an_entry_that_does_not_exist_is_stale(self):
        problems = self.run_guard(SAME, SAME, {"xy_os_nonexistent": "x"})
        self.assertEqual(1, len(problems))
        self.assertIn("stale", problems[0])

    def test_a_missing_backend_says_the_walk_is_broken(self):
        with tempfile.TemporaryDirectory() as tmp:
            d = Path(tmp) / "baremetal"
            d.mkdir(parents=True)
            (d / "xy_os_x.c").write_text(SAME)
            problems = guard.findings(Path(tmp), ("baremetal", "freertos"), pinned={})
            self.assertEqual(1, len(problems))
            self.assertIn("walk is broken", problems[0])


SENTINEL_OK = """\
uint32_t xy_os_event_flags_get(xy_os_event_flags_id_t ef)
{
    if (!ef) return XY_OS_EVENT_FLAGS_ERROR;
    return 0U;
}
"""

SENTINEL_VENDOR = """\
uint32_t xy_os_event_flags_get(xy_os_event_flags_id_t ef)
{
    return vendor_get(ef);
}
"""


class SentinelTests(unittest.TestCase):
    def tree(self, tmp: str, files: dict[str, str]) -> Path:
        root = Path(tmp)
        for backend, source in files.items():
            d = root / backend
            d.mkdir(parents=True, exist_ok=True)
            (d / "xy_os_x.c").write_text(source)
        return root

    def test_a_backend_that_uses_the_sentinel_passes(self):
        with tempfile.TemporaryDirectory() as tmp:
            self.assertEqual([], guard.sentinel_findings(self.tree(tmp, {"bm": SENTINEL_OK}), pinned={}))

    def test_a_vendor_error_encoding_is_named(self):
        with tempfile.TemporaryDirectory() as tmp:
            problems = guard.sentinel_findings(self.tree(tmp, {"bm": SENTINEL_VENDOR}), pinned={})
            self.assertEqual(1, len(problems))
            self.assertIn("xy_os_event_flags_get", problems[0])

    def test_a_pinned_backend_is_skipped_but_still_counted(self):
        with tempfile.TemporaryDirectory() as tmp:
            tree = self.tree(tmp, {"bm": SENTINEL_OK, "vendor": SENTINEL_VENDOR})
            self.assertEqual([], guard.sentinel_findings(tree, pinned={"vendor": "unbuildable"}))

    def test_a_tree_with_no_flag_entries_says_the_walk_is_broken(self):
        with tempfile.TemporaryDirectory() as tmp:
            problems = guard.sentinel_findings(self.tree(tmp, {"bm": "int f(void)\n{\n    return 0;\n}\n"}),
                                               pinned={})
            self.assertEqual(1, len(problems))
            self.assertIn("walk is broken", problems[0])


class RealTreeTests(unittest.TestCase):
    def test_the_real_tree_passes(self):
        self.assertEqual([], guard.findings())


if __name__ == "__main__":
    unittest.main(verbosity=2)
