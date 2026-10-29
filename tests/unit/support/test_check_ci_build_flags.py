#!/usr/bin/env python3
"""Cases for the workflow build-flag lint."""

from __future__ import annotations

import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import check_ci_build_flags as guard  # noqa: E402

BAD = '      run: cmake -S . -B build/x -DOSAL_BACKEND=freertos\n'
GOOD = '      run: cmake -S . -B build/x -DKCONFIG_OVERRIDES="OSAL_BACKEND_FREERTOS=ON"\n'
COMMENTED = '      # -DOSAL_BACKEND= does not work here, the backend comes from Kconfig\n'


class FlagTests(unittest.TestCase):
    def run_guard(self, text: str, pinned: dict | None = None) -> list[str]:
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "workflow.yml"
            path.write_text(text)
            return guard.findings(paths=[path], pinned=pinned if pinned is not None else {})

    def test_the_rejected_flag_is_named(self):
        problems = self.run_guard(BAD)
        self.assertEqual(1, len(problems))
        self.assertIn("-DOSAL_BACKEND=", problems[0])
        self.assertIn("Kconfig", problems[0])

    def test_the_kconfig_route_passes(self):
        self.assertEqual([], self.run_guard(GOOD))

    def test_a_comment_about_the_trap_is_not_a_usage(self):
        self.assertEqual([], self.run_guard(COMMENTED))

    def test_a_pinned_flag_passes(self):
        self.assertEqual([], self.run_guard(BAD, {("workflow.yml", "-DOSAL_BACKEND="): "we accept a fatal here"}))

    def test_an_empty_walk_is_reported(self):
        problems = guard.findings(paths=[])
        self.assertEqual(1, len(problems))
        self.assertIn("walk is broken", problems[0])

    def test_the_real_workflows_do_not_pass_it(self):
        self.assertEqual([], guard.findings())


if __name__ == "__main__":
    unittest.main(verbosity=2)
