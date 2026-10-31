#!/usr/bin/env python3
"""A guard that has never failed is a guard whose red nobody has seen.

`check_dead_kconfig_options.py` passes on this repository, which is the state in which it is
indistinguishable from a script that walks nothing and prints `ok`. These cases drive the same
`findings()` against a synthetic tree, so each rule it claims to enforce has a red it produces.

The one that matters most is the comment case. A dead switch is exactly the kind of thing that gets
discussed in a comment before somebody decides what to do with it, so a guard that counted comment
text as a reader would report the symbol as live precisely when it is not.
"""

from __future__ import annotations

import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import check_dead_kconfig_options as guard  # noqa: E402


class DeadKconfigOptionsTest(unittest.TestCase):
    def setUp(self) -> None:
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.root = Path(self.directory.name)

    def write(self, name: str, text: str) -> None:
        path = self.root / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text, encoding="utf-8")

    def judge(self, symbols: list[str], pinned: dict[str, str] | None = None,
              witness: bool = True) -> list[str]:
        # "no files were inspected" is judged across the whole walk, not per symbol, so a synthetic
        # tree with nothing in it is that red rather than a clean result. The witness keeps the walk
        # non-empty; the empty-walk rule is measured on its own below.
        if witness:
            self.write("witness.cmake", "set(WITNESS_SCAFFOLD 1)\n")
        return guard.findings(symbols, guard.corpus(self.root), pinned or {})

    def test_a_symbol_the_build_reads_passes(self) -> None:
        self.write("config.cmake", "if(CONFIG_FOTA_ENABLED)\nendif()\n")
        self.assertEqual([], self.judge(["FOTA_ENABLED"]))

    def test_a_symbol_read_from_c_source_passes(self) -> None:
        self.write("src/app.c", "#if CONFIG_FOTA_ENABLED\n#endif\n")
        self.assertEqual([], self.judge(["FOTA_ENABLED"]))

    def test_a_symbol_nothing_reads_is_reported(self) -> None:
        problems = self.judge(["FOTA_ENABLED"])
        self.assertEqual(1, len(problems), problems)
        self.assertIn("FOTA_ENABLED", problems[0])
        self.assertIn("configures nothing", problems[0])

    def test_a_symbol_mentioned_only_in_a_cmake_comment_is_still_dead(self) -> None:
        # The realistic shape: the switch was discussed here before anyone wired it up.
        self.write("config.cmake", "# TODO: honour CONFIG_FOTA_ENABLED one day\n")
        problems = self.judge(["FOTA_ENABLED"])
        self.assertEqual(1, len(problems), problems)

    def test_a_symbol_mentioned_only_in_a_c_comment_is_still_dead(self) -> None:
        self.write("src/app.c", "/* CONFIG_FOTA_ENABLED is not consulted yet */\nint x;\n")
        problems = self.judge(["FOTA_ENABLED"])
        self.assertEqual(1, len(problems), problems)

    def test_a_pinned_symbol_passes(self) -> None:
        self.assertEqual([], self.judge(["FOTA_ENABLED"], {"FOTA_ENABLED": "declared for a menu"}))

    def test_a_stale_pin_is_reported_when_the_symbol_became_read(self) -> None:
        self.write("config.cmake", "set(X ${CONFIG_FOTA_ENABLED})\n")
        problems = self.judge(["FOTA_ENABLED"], {"FOTA_ENABLED": "declared for a menu"})
        self.assertEqual(1, len(problems), problems)
        self.assertIn("stale", problems[0])

    def test_a_pin_that_outlived_the_symbol_is_reported(self) -> None:
        self.write("config.cmake", "if(CONFIG_KEPT)\nendif()\n")
        problems = self.judge(["KEPT"], {"GONE": "was declared"})
        self.assertTrue(any("GONE" in p and "no longer declares" in p for p in problems), problems)

    def test_a_pin_without_a_reason_is_reported(self) -> None:
        problems = self.judge(["FOTA_ENABLED"], {"FOTA_ENABLED": "   "})
        self.assertTrue(any("without a stated reason" in p for p in problems), problems)

    def test_an_empty_declaration_list_is_reported(self) -> None:
        self.write("config.cmake", "if(CONFIG_A)\nendif()\n")
        problems = self.judge([])
        self.assertEqual(1, len(problems), problems)
        self.assertIn("walk found nothing", problems[0])

    def test_an_empty_corpus_is_reported(self) -> None:
        # No files at all: the walk inspected nothing, which must not read as a clean sweep.
        problems = self.judge(["FOTA_ENABLED"], witness=False)
        self.assertEqual(1, len(problems), problems)
        self.assertIn("walk is broken", problems[0])

    def test_the_declaration_file_is_not_itself_a_consumer(self) -> None:
        # A Kconfig line naming CONFIG_<symbol> would otherwise make every symbol look read.
        self.write("Kconfig", "config FOTA_ENABLED\n    bool \"x\"\n")
        problems = self.judge(["FOTA_ENABLED"])
        self.assertEqual(1, len(problems), problems)

    def test_the_cmake_integration_script_does_count_as_a_consumer(self) -> None:
        # Its name starts with "Kconfig" and it is not a declaration file. Skipping by name prefix
        # made a version of this guard report LOG_LEVEL and BUILD_SHARED_LIBS as dead.
        self.write("cmake/Kconfig.cmake", "set(BUILD_SHARED_LIBS ${CONFIG_BUILD_SHARED_LIBS})\n")
        self.assertEqual([], self.judge(["BUILD_SHARED_LIBS"]))

    def test_the_real_tree_pins_every_symbol_it_declares(self) -> None:
        # The claim that matters: every symbol Kconfig declares is either read or pinned with a reason.
        self.assertEqual([], guard.findings())


if __name__ == "__main__":
    unittest.main()
