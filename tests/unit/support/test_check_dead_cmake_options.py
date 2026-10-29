#!/usr/bin/env python3
"""Cases for the dead CMake option lint."""

from __future__ import annotations

import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import check_dead_cmake_options as guard  # noqa: E402

DEAD = 'option(BUILD_EXAMPLES "Build FOTA examples" ON)\nadd_executable(x x.c)\n'
REAL = 'option(BUILD_EXAMPLES "Build FOTA examples" ON)\nif(BUILD_EXAMPLES)\nadd_executable(x x.c)\nendif()\n'


class OptionTests(unittest.TestCase):
    def run_guard(self, cmake: str, extra: str = "", pinned: dict | None = None) -> list[str]:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            declaring = root / "CMakeLists.txt"
            declaring.write_text(cmake)
            referencing = [declaring]
            if extra:
                other = root / "presets.json"
                other.write_text(extra)
                referencing.append(other)
            return guard.findings(declaring=[declaring], referencing=referencing,
                                  pinned=pinned if pinned is not None else {})

    def test_an_option_nothing_reads_is_named(self):
        problems = self.run_guard(DEAD)
        self.assertEqual(1, len(problems))
        self.assertIn("BUILD_EXAMPLES", problems[0])
        self.assertIn("wire it up or delete it", problems[0])

    def test_an_option_that_gates_something_passes(self):
        self.assertEqual([], self.run_guard(REAL))

    def test_a_reference_from_a_preset_file_counts(self):
        """A preset, workflow or script reading it is a legitimate consumer."""
        self.assertEqual([], self.run_guard(DEAD, extra='{"cacheVariables": {"BUILD_EXAMPLES": "OFF"}}\n'))

    def test_a_pinned_option_passes(self):
        self.assertEqual([], self.run_guard(DEAD, pinned={"BUILD_EXAMPLES": "reserved"}))

    def test_an_empty_walk_is_reported(self):
        problems = guard.findings(declaring=[], referencing=[])
        self.assertEqual(1, len(problems))
        self.assertIn("walk is broken", problems[0])

    def test_the_real_tree_has_no_dead_option(self):
        self.assertEqual([], guard.findings())


if __name__ == "__main__":
    unittest.main(verbosity=2)
