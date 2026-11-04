#!/usr/bin/env python3
"""Self-test for check_legacy_sensor_includes.py.

The legacy sensor framework headers are all gone; the checker's remaining job is to keep reporting
zero legacy headers, zero consumers and zero pins, so that a re-introduced sensor_device_t header
cannot silently reappear. The only case left to assert is the real one: the current tree is clean.
"""

from __future__ import annotations

import subprocess
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from check_legacy_sensor_includes import REPO_ROOT  # noqa: E402


class CurrentTree(unittest.TestCase):
    def test_current_tree_is_clean(self) -> None:
        proc = subprocess.run(
            [sys.executable, str(Path(__file__).resolve().parent /
                                 "check_legacy_sensor_includes.py")],
            capture_output=True, text=True, cwd=str(REPO_ROOT))
        self.assertEqual(0, proc.returncode, proc.stdout + proc.stderr)
        self.assertIn("legacy_sensor_includes ok", proc.stdout)


if __name__ == "__main__":
    unittest.main(verbosity=2)
