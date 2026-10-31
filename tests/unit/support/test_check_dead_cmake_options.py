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


class ConditionTests(unittest.TestCase):
    """Names a condition tests must be ones CMake can actually hold."""

    def run_conditions(self, cmake: str, kconfig: str = "") -> list[str]:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            script = root / "CMakeLists.txt"
            script.write_text(cmake)
            symbols = root / "Kconfig"
            symbols.write_text(kconfig)
            return guard.condition_findings(cmake=[script], kconfigs=[symbols])

    def test_a_c_macro_in_a_condition_is_named(self):
        problems = self.run_conditions(
            "if(NOT XY_CONFIG_SMBUS_ENABLED)\n    return()\nendif()\n")
        self.assertEqual(1, len(problems))
        self.assertIn("XY_CONFIG_SMBUS_ENABLED", problems[0])
        self.assertIn("preprocessor macro", problems[0])

    def test_a_generated_name_without_a_symbol_is_named(self):
        problems = self.run_conditions(
            "if(XY_COMPONENT_SMBUS)\n    add_library(xy_smbus s.c)\nendif()\n")
        self.assertEqual(1, len(problems))
        self.assertIn("XY_COMPONENT_SMBUS", problems[0])
        self.assertIn("Kconfig declares no symbol COMPONENT_SMBUS", problems[0])

    def test_a_generated_name_backed_by_a_symbol_passes(self):
        self.assertEqual([], self.run_conditions(
            "if(XY_PROTO_AT_CLIENT)\n    list(APPEND S a.c)\nendif()\n",
            "config PROTO_AT_CLIENT\n    bool \"AT client\"\n"))

    def test_a_string_mention_is_not_a_condition(self):
        """A REGEX filter contains the name but decides nothing."""
        self.assertEqual([], self.run_conditions(
            'list(FILTER SOURCES EXCLUDE REGEX "XY_CONFIG_.*")\n'))

    def test_the_real_tree_has_no_impossible_condition(self):
        self.assertEqual([], guard.condition_findings())


if __name__ == "__main__":
    unittest.main(verbosity=2)
