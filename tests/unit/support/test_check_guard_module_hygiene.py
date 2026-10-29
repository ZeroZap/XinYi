#!/usr/bin/env python3
"""Cases for the guard-module hygiene lint."""

from __future__ import annotations

import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import check_guard_module_hygiene as guard  # noqa: E402

CLEAN = """\
X = 1
Y = 2


def f():
    return X
"""

DUPLICATE = """\
NAME = "first"
OTHER = 1
NAME = "second"
"""

DUPLICATE_FUNCTION = """\
def f():
    return 1


def f():
    return 2
"""

TUPLE = """\
A, B = 1, 2
A = 3
"""

BROKEN = """\
def f(:
"""


class HygieneTests(unittest.TestCase):
    def write(self, tmp: str, name: str, source: str) -> list[Path]:
        path = Path(tmp) / name
        path.write_text(source)
        return [path]

    def run_guard(self, source: str, name: str = "check_x.py") -> list[str]:
        with tempfile.TemporaryDirectory() as tmp:
            return guard.findings(Path(tmp), self.write(tmp, name, source))

    def test_a_clean_module_passes(self):
        self.assertEqual([], self.run_guard(CLEAN))

    def test_a_duplicated_constant_is_named_with_both_lines(self):
        problems = self.run_guard(DUPLICATE)
        self.assertEqual(1, len(problems))
        self.assertIn("NAME", problems[0])
        self.assertIn("[1, 3]", problems[0])

    def test_a_redefined_function_is_named(self):
        problems = self.run_guard(DUPLICATE_FUNCTION)
        self.assertEqual(1, len(problems))
        self.assertIn("def f", problems[0].replace("f is defined", "def f"))

    def test_a_tuple_assignment_counts_as_binding(self):
        problems = self.run_guard(TUPLE)
        self.assertEqual(1, len(problems))
        self.assertIn("A", problems[0])

    def test_a_file_that_does_not_parse_is_reported(self):
        problems = self.run_guard(BROKEN)
        self.assertEqual(1, len(problems))
        self.assertIn("does not parse", problems[0])

    def test_an_empty_walk_is_reported(self):
        with tempfile.TemporaryDirectory() as tmp:
            problems = guard.findings(Path(tmp))
            self.assertEqual(1, len(problems))
            self.assertIn("walk is broken", problems[0])

    def test_the_real_tree_passes(self):
        self.assertEqual([], guard.findings())


class EntryPointTests(unittest.TestCase):
    """A test file that runs nothing exits 0 and reads as a pass."""

    def run_entry(self, source: str, name: str = "test_x.py") -> list[str]:
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / name
            path.write_text(source)
            return guard.entry_point_findings(Path(tmp), [path])

    def test_a_file_with_no_entry_point_is_named(self):
        problems = self.run_entry("def helper():\n    return 1\n")
        self.assertEqual(1, len(problems))
        self.assertIn("runs nothing", problems[0])

    def test_unittest_main_counts(self):
        self.assertEqual([], self.run_entry("import unittest\n\nif __name__ == '__main__':\n"
                                            "    unittest.main(verbosity=2)\n"))

    def test_a_top_level_assert_counts(self):
        self.assertEqual([], self.run_entry("import json\nassert json.loads('1') == 1\n"))

    def test_a_main_guard_counts(self):
        self.assertEqual([], self.run_entry("def main():\n    return 0\n\n"
                                            "if __name__ == '__main__':\n    raise SystemExit(main())\n"))

    def test_an_empty_walk_is_reported(self):
        with tempfile.TemporaryDirectory() as tmp:
            problems = guard.entry_point_findings(Path(tmp), [])
            self.assertEqual(1, len(problems))
            self.assertIn("walk is broken", problems[0])


if __name__ == "__main__":
    unittest.main(verbosity=2)
