#!/usr/bin/env python3
"""Cases for the OSAL backend-uniqueness guard.

One fixture that satisfies both rules; each case damages exactly one thing in it.
"""

from __future__ import annotations

import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import check_osal_backend_uniqueness as guard  # noqa: E402


def entries(count: int, prefix: str = "xy_os_thing") -> set[str]:
    return {f"{prefix}_{index}" for index in range(count)}


class PlacementTests(unittest.TestCase):
    def test_one_implementation_per_backend_passes(self):
        sources = {"backend/freertos/xy_os_freertos.c": entries(70)}
        self.assertEqual([], guard.placement_findings(sources))

    def test_a_second_implementation_outside_a_backend_is_named(self):
        sources = {
            "backend/freertos/xy_os_freertos.c": entries(70),
            "src/osal_baremetal.c": entries(70, "xy_os_other"),
        }
        problems = guard.placement_findings(sources)
        self.assertEqual(1, len(problems))
        self.assertIn("second implementation of a backend", problems[0])

    def test_a_pinned_shared_source_is_allowed(self):
        sources = {"src/xy_os_tick.c": entries(30)}
        self.assertEqual([], guard.placement_findings(sources))

    def test_a_small_helper_file_is_not_treated_as_a_backend(self):
        sources = {"src/helper.c": entries(5)}
        self.assertEqual([], guard.placement_findings(sources))


class DeclarationTests(unittest.TestCase):
    def test_a_declared_entry_passes(self):
        sources = {"backend/freertos/xy_os_freertos.c": {"xy_os_kernel_init"}}
        self.assertEqual([], guard.declaration_findings(sources, {"xy_os_kernel_init"}))

    def test_an_undeclared_entry_is_named(self):
        sources = {"backend/freertos/xy_os_freertos.c": {"xy_os_invented"}}
        problems = guard.declaration_findings(sources, {"xy_os_kernel_init"})
        self.assertEqual(1, len(problems))
        self.assertIn("declared in no header", problems[0])

    def test_a_pinned_undeclared_entry_passes(self):
        path = "backend/rtx5/xy_os_rtx5.c"
        pinned, _ = guard.PINNED_UNDECLARED[path]
        name = sorted(pinned)[0]
        sources = {path: {name}}
        self.assertEqual([], guard.declaration_findings(sources, set()))

    def test_a_pin_that_closes_is_named(self):
        """The pin has to fail in both directions or a gap closes silently."""
        path = "backend/rtx5/xy_os_rtx5.c"
        pinned, _ = guard.PINNED_UNDECLARED[path]
        name = sorted(pinned)[0]
        # The name is declared now, so it no longer belongs in the pinned set.
        problems = guard.stale_pin_findings({path: {name}}, {name})
        # Every pinned name is no longer undeclared there, so the whole pin is stale.
        self.assertEqual(len(pinned), len(problems))
        self.assertIn("the pin is stale", problems[0])

    def test_a_pin_still_open_is_not_named(self):
        path = "backend/rtx5/xy_os_rtx5.c"
        pinned, _ = guard.PINNED_UNDECLARED[path]
        self.assertEqual([], guard.stale_pin_findings({path: set(pinned)}, set()))


class RealTreeTests(unittest.TestCase):
    def test_the_walk_finds_the_backends(self):
        """A walk that finds nothing reports a clean tree - assert it found what is there."""
        sources = guard.implementation_sources()
        self.assertGreaterEqual(len(sources), 4)
        self.assertTrue(any(path.startswith("backend/") for path in sources))

    def test_the_deletion_is_gone(self):
        self.assertNotIn("src/osal_baremetal.c", guard.implementation_sources())

    def test_the_tree_passes(self):
        self.assertEqual(0, guard.main())


class UnlistedSourceTests(unittest.TestCase):
    """A .c nothing names is a file nothing verifies."""

    def make_tree(self, tmp: str, cmake: str, files: dict[str, str]) -> Path:
        root = Path(tmp) / "osal"
        (root / "src").mkdir(parents=True)
        (root / "CMakeLists.txt").write_text(cmake)
        for name, text in files.items():
            target = root / name
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_text(text)
        return root

    def test_an_unlisted_source_is_named(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = self.make_tree(tmp, "src/xy_os_tick.c\n", {"src/xy_os_tick.c": "", "src/dup.c": ""})
            problems = guard.unlisted_source_findings(root, {})
            self.assertEqual(1, len(problems))
            self.assertIn("src/dup.c", problems[0])

    def test_a_listed_source_passes(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = self.make_tree(tmp, "src/xy_os_tick.c\n", {"src/xy_os_tick.c": ""})
            self.assertEqual([], guard.unlisted_source_findings(root, {}))

    def test_a_pinned_source_passes(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = self.make_tree(tmp, "src/xy_os_tick.c\n",
                                  {"src/xy_os_tick.c": "", "backend/rtx5/x.c": ""})
            self.assertEqual([], guard.unlisted_source_findings(root, {"backend/rtx5/x.c": "orphaned"}))

    def test_an_empty_walk_says_so(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp) / "osal"
            root.mkdir()
            (root / "CMakeLists.txt").write_text("")
            problems = guard.unlisted_source_findings(root, {})
            self.assertEqual(1, len(problems))
            self.assertIn("walk is broken", problems[0])


class SharedSourcePinTests(unittest.TestCase):
    """A pin for a file that is gone, or defines nothing, is a stale claim."""

    def test_a_missing_file_is_reported(self):
        problems = guard.shared_source_pin_findings({}, {"src/gone.c": "was shared"})
        self.assertEqual(1, len(problems))
        self.assertIn("does not exist", problems[0])

    def test_a_file_defining_nothing_is_reported(self):
        problems = guard.shared_source_pin_findings({"src/xy_os_tick.c": set()},
                                                    {"src/xy_os_tick.c": "tick accounting"})
        self.assertEqual(1, len(problems))
        self.assertIn("defines no OSAL entry", problems[0])

    def test_a_real_shared_source_passes(self):
        problems = guard.shared_source_pin_findings({"src/xy_os_tick.c": {"xy_os_tick_get"}},
                                                    {"src/xy_os_tick.c": "tick accounting"})
        self.assertEqual([], problems)


if __name__ == "__main__":
    unittest.main(verbosity=2)
