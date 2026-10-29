#!/usr/bin/env python3
"""Cases for the dead-CLI-option lint."""

from __future__ import annotations

import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import check_dead_cli_options as guard  # noqa: E402

DEAD = 'import argparse\np = argparse.ArgumentParser()\np.add_argument("--mrs-root", type=Path, default=Path("/x"))\nargs = p.parse_args()\n'
LIVE = 'import argparse\np = argparse.ArgumentParser()\np.add_argument("--live")\nargs = p.parse_args()\nprint(args.live)\n'
DYNAMIC_ONLY = 'import argparse\np = argparse.ArgumentParser()\np.add_argument("--anything")\nprint(vars(p.parse_args()))\n'


class OptionTests(unittest.TestCase):
    def run_guard(self, text: str, pinned: dict | None = None) -> list[str]:
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "tool.py"
            path.write_text(text)
            return guard.findings(paths=[path], pinned=pinned if pinned is not None else {})

    def test_a_declared_option_nothing_reads_is_named(self):
        problems = self.run_guard(DEAD)
        self.assertEqual(1, len(problems))
        self.assertIn("--mrs-root", problems[0])
        self.assertIn("never reads it", problems[0])

    def test_a_read_option_passes(self):
        self.assertEqual([], self.run_guard(LIVE))

    def test_a_dest_that_differs_from_the_flag_is_followed(self):
        text = LIVE.replace('"--live"', '"--live-flag", dest="target"').replace("args.live", "args.target")
        self.assertEqual([], self.run_guard(text))

    def test_a_dynamic_parser_is_skipped_loudly(self):
        """vars()/getattr() make the names unreadable; skipping silently would look like a clean run."""
        problems = self.run_guard(DYNAMIC_ONLY)
        self.assertEqual(1, len(problems))
        self.assertIn("walk is broken", problems[0])

    def test_a_pinned_option_passes(self):
        self.assertEqual([], self.run_guard(DEAD, {("tool.py", "--mrs-root"): "reserved for the next slice"}))

    def test_the_real_tools_have_no_dead_option(self):
        self.assertEqual([], guard.findings())


if __name__ == "__main__":
    unittest.main(verbosity=2)
