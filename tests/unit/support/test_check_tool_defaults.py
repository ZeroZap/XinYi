#!/usr/bin/env python3
"""Cases for the absolute-default lint."""

from __future__ import annotations

import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import check_tool_defaults as guard  # noqa: E402

LITERAL = '''\
import argparse

parser = argparse.ArgumentParser()
parser.add_argument("--openocd", default="/opt/wch/openocd")
'''

ALIASED = '''\
import argparse
from pathlib import Path

_VENDOR = Path("D:/MounRiver/studio")
DEFAULT_ROOT = _VENDOR

parser = argparse.ArgumentParser()
parser.add_argument("--mrs-root", default=DEFAULT_ROOT)
'''

PATH_LOOKUP = '''\
import argparse
import shutil
from pathlib import Path

DEFAULT = Path(shutil.which("openocd")) if shutil.which("openocd") else Path("D:/vendor/openocd.exe")

parser = argparse.ArgumentParser()
parser.add_argument("--openocd", default=DEFAULT)
'''

RELATIVE = '''\
import argparse

parser = argparse.ArgumentParser()
parser.add_argument("--config", default="config/wch.cfg")
'''


class DefaultTests(unittest.TestCase):
    def run_guard(self, source: str, pinned: dict | None = None) -> list[str]:
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "tool.py"
            path.write_text(source)
            return guard.findings(pinned=pinned if pinned is not None else {}, paths=[path])

    def test_a_literal_absolute_default_is_named(self):
        problems = self.run_guard(LITERAL)
        self.assertEqual(1, len(problems))
        self.assertIn("--openocd", problems[0])
        self.assertIn("/opt/wch/openocd", problems[0])

    def test_an_alias_of_a_constant_is_followed(self):
        """DEFAULT_ROOT = _VENDOR is a name; the absolute path behind it still has to be seen."""
        problems = self.run_guard(ALIASED)
        self.assertEqual(1, len(problems))
        self.assertIn("--mrs-root", problems[0])

    def test_a_path_lookup_default_passes(self):
        self.assertEqual([], self.run_guard(PATH_LOOKUP))

    def test_a_relative_default_passes(self):
        self.assertEqual([], self.run_guard(RELATIVE))

    def test_a_pinned_option_passes(self):
        self.assertEqual([], self.run_guard(LITERAL, {("tool.py", "--openocd"): "vendor installer location"}))

    def test_an_empty_walk_is_reported(self):
        with tempfile.TemporaryDirectory() as tmp:
            problems = guard.findings(paths=[])
            self.assertEqual(1, len(problems))
            self.assertIn("walk is broken", problems[0])

    def test_the_real_tree_pins_every_absolute_default(self):
        self.assertEqual([], guard.findings())


if __name__ == "__main__":
    unittest.main(verbosity=2)
