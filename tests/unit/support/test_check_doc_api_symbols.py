#!/usr/bin/env python3
"""A guard that has never failed is a guard whose red nobody has seen.

`check_doc_api_symbols.py` passes on this repository, which is the state in which it is
indistinguishable from a script that inspects nothing and prints `ok`. These cases drive the same
`findings()` against synthetic documents and a synthetic header index, so every rule it claims to
enforce has a red it produces.

The index is injected rather than read so the cases stay honest about what is being judged: the
question is what the checker does with a token, not whether this tree happens to declare it today.

One contract this file had to discover: "no fenced C block was found" is judged **across all
candidates**, not per document. A single prose-only or shell-only document is therefore not a clean
result - it is the same red as a broken walk. So the cases that are about *what a document's fence
contains* hand in a companion document that does contain a clean C block; the walk's non-emptiness is
then not what is being measured, and it is measured on its own below.
"""

from __future__ import annotations

import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import check_doc_api_symbols as guard  # noqa: E402

#: What the real tree declares, shrunk to two names plus a header. Enough to tell "declared" from
#: "guessed at" without pinning the test to a header that may be renamed.
SYMBOLS = {"xy_hal_gpio_init", "xy_hal_gpio_write", "xy_hal_gpio_config_t"}
NAMES = {"xy_hal_gpio.h"}

#: A document that is unambiguously clean, used to keep the global walk non-empty.
WITNESS = '#include "xy_hal_gpio.h"\nxy_hal_gpio_init(0);\n'


def fenced(body: str, language: str = "c") -> str:
    return f"# Doc\n\n```{language}\n{body}\n```\n"


class DocApiSymbolsTest(unittest.TestCase):
    def setUp(self) -> None:
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.root = Path(self.directory.name)

    def judged(self, text: str, language: str = "c", witness: bool = True) -> list[str]:
        paths: list[Path] = []
        if witness:
            companion = self.root / "WITNESS.md"
            companion.write_text(fenced(WITNESS), encoding="utf-8")
            paths.append(companion)
        path = self.root / "SAMPLE.md"
        path.write_text(fenced(text, language), encoding="utf-8")
        paths.append(path)
        return guard.findings(paths, SYMBOLS, NAMES)

    def test_a_sample_that_only_uses_declared_names_passes(self) -> None:
        self.assertEqual([], self.judged(
            '#include "xy_hal_gpio.h"\n'
            "xy_hal_gpio_config_t config;\n"
            "xy_hal_gpio_init(&config);\n"
            "xy_hal_gpio_write(0, 1);\n"))

    def test_a_name_no_header_declares_is_reported(self) -> None:
        # The original defect: the sample taught `xy_hal_pin_init()`, which was never a function.
        problems = self.judged("xy_hal_pin_init(&config);\n")
        self.assertTrue(any("xy_hal_pin_init" in p and "no header declares" in p for p in problems),
                        problems)

    def test_an_unknown_name_is_offered_its_nearest_declaration(self) -> None:
        problems = self.judged("xy_hal_gpio_initt(&config);\n")
        self.assertTrue(any("did you mean xy_hal_gpio_init?" in p for p in problems), problems)

    def test_a_far_name_offers_no_hint_instead_of_a_bad_one(self) -> None:
        problems = self.judged("xy_os_thread_create(0);\n")
        self.assertTrue(any("xy_os_thread_create" in p for p in problems), problems)
        self.assertFalse(any("did you mean" in p for p in problems), problems)

    def test_an_include_that_is_not_a_header_here_is_reported(self) -> None:
        problems = self.judged('#include "xy_crypto.h"\n')
        self.assertTrue(any('xy_crypto.h' in p and "not a header in this repository" in p
                            for p in problems), problems)

    def test_an_include_of_a_header_that_exists_passes(self) -> None:
        self.assertEqual([], self.judged('#include "xy_hal_gpio.h"\n'))

    def test_a_name_the_block_defines_itself_passes(self) -> None:
        # A sample is allowed a helper of its own; judging that would punish a correct example.
        self.assertEqual([], self.judged(
            "#define xy_led_pin 13\n"
            "xy_hal_gpio_write(xy_led_pin, 1);\n"))

    def test_a_shell_block_is_not_an_api_claim(self) -> None:
        # `make <target>` is a different rule, owned by a different guard. Judging shell blocks here
        # would report every build command as an undeclared symbol.
        self.assertEqual([], self.judged("xy_not_a_symbol anywhere here", language="bash"))

    def test_prose_outside_a_fence_is_not_judged(self) -> None:
        prose = self.root / "PROSE.md"
        prose.write_text("# Doc\n\nCall `xy_totally_made_up()` to begin.\n", encoding="utf-8")
        companion = self.root / "WITNESS.md"
        companion.write_text(fenced(WITNESS), encoding="utf-8")
        self.assertEqual([], guard.findings([prose, companion], SYMBOLS, NAMES))

    def test_a_document_opening_and_closing_unbalanced_is_still_walked(self) -> None:
        # An unclosed fence must not swallow the rest of the document silently.
        path = self.root / "UNCLOSED.md"
        path.write_text("```c\nxy_made_up_symbol();\n", encoding="utf-8")
        problems = guard.findings([path], SYMBOLS, NAMES)
        self.assertTrue(any("xy_made_up_symbol" in p for p in problems), problems)

    def test_a_walk_over_no_block_at_all_reports_itself_broken(self) -> None:
        # The failure this whole file exists to make impossible: inspecting nothing and printing ok.
        path = self.root / "EMPTY.md"
        path.write_text("# Doc\n\nNo fenced C here.\n", encoding="utf-8")
        problems = guard.findings([path], SYMBOLS, NAMES)
        self.assertTrue(any("the walk is broken" in p for p in problems), problems)

    def test_only_one_document_without_a_fence_still_reports_the_walk(self) -> None:
        # Stated so the global reading of the rule is pinned, not just true by accident.
        empty = self.root / "EMPTY.md"
        empty.write_text("# Doc\n\nNo fenced C here.\n", encoding="utf-8")
        problems = guard.findings([empty], SYMBOLS, NAMES)
        self.assertTrue(any("the walk is broken" in p for p in problems), problems)


if __name__ == "__main__":
    unittest.main()
