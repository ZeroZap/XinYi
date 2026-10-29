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


class CiCoverageTests(unittest.TestCase):
    """A test target only the author types is a gate for whoever remembers."""

    MAKE = "test\n\t@true\n\ntest-orphan:\n\t@true\n"
    CI = "run: make test\n"

    def run_ci(self, ci_text: str, exempt: dict | None = None) -> list[str]:
        with tempfile.TemporaryDirectory() as tmp:
            make = Path(tmp) / "Makefile"
            make.write_text(self.MAKE)
            workflow = Path(tmp) / "wf.yml"
            workflow.write_text(ci_text)
            return guard.ci_coverage_findings(make, [workflow], exempt if exempt is not None else {})

    def test_a_target_no_workflow_runs_is_named(self):
        problems = self.run_ci(self.CI)
        self.assertEqual(1, len(problems))
        self.assertIn("test-orphan", problems[0])

    def test_a_target_a_workflow_names_passes(self):
        self.assertEqual([], self.run_ci("run: make test test-orphan\n"))

    def test_an_exempt_target_passes(self):
        self.assertEqual([], self.run_ci(self.CI, {"test-orphan": "needs local hardware"}))

    def test_no_targets_at_all_is_reported(self):
        with tempfile.TemporaryDirectory() as tmp:
            make = Path(tmp) / "Makefile"
            make.write_text("all:\n\t@true\n")
            problems = guard.ci_coverage_findings(make, [], {})
            self.assertEqual(1, len(problems))
            self.assertIn("walk is broken", problems[0])

    def test_the_real_tree_exempts_every_gap(self):
        self.assertEqual([], [
            problem for problem in guard.ci_coverage_findings()
            if "no workflow names" in problem])


class RegistrationTests(unittest.TestCase):
    """A test file no target names is a test that never runs."""

    DISCOVERY = ("TOOLS_TEST_DIR := tests/unit/support\n"
                 "\t@python3 -m unittest discover -s $(TOOLS_TEST_DIR) -p 'test_*.py'\n")

    def test_an_unregistered_file_is_named(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "test_nobody_runs.py"
            path.write_text("assert True\n")
            problems = guard.registration_findings(Path(tmp), [path])
            self.assertEqual(1, len(problems))
            self.assertIn("test_nobody_runs.py", problems[0])
            self.assertIn("not being tested", problems[0])

    def test_a_file_named_by_a_gate_target_passes(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "guard_module_hygiene.py"  # its own name is in tests/unit/CMakeLists.txt
            path.write_text("assert True\n")
            self.assertEqual([], guard.registration_findings(Path(tmp), [path]))

    def test_a_file_under_a_discovered_directory_passes(self):
        # Named by nothing, but the gate runs discovery over the directory it lives in. The directory is
        # spelled as a Makefile variable here because that is how the real Makefile spells its one.
        path = guard.ROOT / "tests" / "unit" / "support" / "test_source_scan.py"
        self.assertNotIn(path.name, self.DISCOVERY)
        self.assertEqual([], guard.registration_findings(files=[path], text=self.DISCOVERY))

    def test_a_discovery_line_for_another_directory_covers_nothing(self):
        path = guard.ROOT / "tests" / "unit" / "support" / "test_source_scan.py"
        text = "TOOLS_TEST_DIR := tests/unit/somewhere_else\n" \
               "\t@python3 -m unittest discover -s $(TOOLS_TEST_DIR) -p 'test_*.py'\n"
        problems = guard.registration_findings(files=[path], text=text)
        self.assertEqual(1, len(problems))
        self.assertIn("test_source_scan.py", problems[0])

    def test_the_makefile_discovery_directory_is_resolved(self):
        # The repository's only discovery line names its directory through a variable; read as a literal
        # token it covers no directory at all, and this rule would be enforcing nothing.
        self.assertEqual(["tools/tests"], guard.discovered_dirs(guard.registration_text()))

    def test_a_component_build_file_cannot_register_a_test(self):
        # Only the gate trees count. A component or vendor CMakeLists that happens to contain a name is
        # not a gate, and counting it would let a file look registered while nothing runs it.
        self.assertNotIn("XY_PROTO_AT_CLIENT", guard.registration_text())

    def test_an_empty_walk_is_reported(self):
        with tempfile.TemporaryDirectory() as tmp:
            problems = guard.registration_findings(Path(tmp), [])
            self.assertEqual(1, len(problems))
            self.assertIn("walk is broken", problems[0])


if __name__ == "__main__":
    unittest.main(verbosity=2)
