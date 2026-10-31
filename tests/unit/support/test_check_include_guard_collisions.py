#!/usr/bin/env python3
"""The collision checker passes on this repository, which is the state in which it is
indistinguishable from a script that reads nothing and prints `ok`.

These cases drive the same `findings()` against synthetic headers, so every rule has a red
it produces: a guard shared inside one directory, a guard shared across directories, a guard
that defines a macro other than the one it tests, a guard with no `#define` at all, and each
of the five ways a pin stops being true. One case compiles two headers with one guard so the
claim ("the second header is not read") is measured rather than asserted.

Every case that is not about the empty walk includes a clean witness header, because the
empty-walk rule is global over the whole candidate list: a case handed only the offending
header would trip that rule instead of the one under test.
"""

from __future__ import annotations

import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import check_include_guard_collisions as guard  # noqa: E402

WITNESS = "clean.h"
WITNESS_TEXT = "#ifndef CLEAN_H\n#define CLEAN_H\nint clean(void);\n#endif\n"


def guarded(name: str, body: str = "int thing(void);") -> str:
    return f"#ifndef {name}\n#define {name}\n{body}\n#endif\n"


class IncludeGuardCollisionTest(unittest.TestCase):
    def judge(self, files: dict[str, str],
              pins: dict[str, tuple[tuple[str, ...], str]] | None = None) -> list[str]:
        return guard.findings(files, {} if pins is None else pins)

    def test_a_clean_tree_passes(self) -> None:
        self.assertEqual([], self.judge({WITNESS: WITNESS_TEXT}))

    def test_an_empty_walk_is_reported(self) -> None:
        problems = self.judge({})
        self.assertEqual(1, len(problems), problems)
        self.assertIn("empty walk", problems[0])

    def test_a_guard_shared_inside_one_directory_is_reported(self) -> None:
        problems = self.judge({
            WITNESS: WITNESS_TEXT,
            "inc/usb2can.h": guarded("_SHARED_H_"),
            "inc/usb2log.h": guarded("_SHARED_H_", "int usb2log_init(void);"),
        })
        self.assertEqual(1, len(problems), problems)
        self.assertIn("_SHARED_H_", problems[0])
        self.assertIn("one directory", problems[0])

    def test_a_guard_shared_across_directories_is_reported(self) -> None:
        problems = self.judge({
            WITNESS: WITNESS_TEXT,
            "components/fota/xy_fota.h": guarded("XY_FOTA_H"),
            "components/fota/inc/xy_fota.h": guarded("XY_FOTA_H", "int xy_fota_bank_swap(void);"),
        })
        self.assertEqual(1, len(problems), len(problems))
        self.assertIn("silently not read", problems[0])

    def test_identical_copies_are_not_reported(self) -> None:
        # Three vendored CMSIS headers are the same file in two sample trees. Identical
        # content means whichever copy wins declares the same things.
        same = guarded("__CORE_CM0_H_GENERIC", "typedef int cm0_t;")
        self.assertEqual([], self.judge({
            WITNESS: WITNESS_TEXT,
            "a/core_cm0.h": same,
            "b/core_cm0.h": same,
        }))

    def test_a_guard_that_defines_another_macro_is_reported(self) -> None:
        problems = self.judge({WITNESS: WITNESS_TEXT, "inc/usb2uart.h": "#ifndef _USB2UART_H_\n"
                                                          "#define _USBUART_H_\nint f(void);\n#endif\n"})
        self.assertEqual(1, len(problems), problems)
        self.assertIn("_USB2UART_H_", problems[0])
        self.assertIn("_USBUART_H_", problems[0])
        self.assertIn("never defined", problems[0])

    def test_a_guard_with_no_define_is_reported(self) -> None:
        problems = self.judge({WITNESS: WITNESS_TEXT, "inc/odd.h": "#ifndef ODD_H\n"
                                                       "#include <stdint.h>\nint f(void);\n#endif\n"})
        self.assertEqual(1, len(problems), problems)
        self.assertIn("not followed by a `#define`", problems[0])

    def test_a_feature_macro_block_is_not_a_guard(self) -> None:
        # at_log.h opens with `#ifndef LOG_TAG` around the macro block and has no guard at
        # all. The first-directive heuristic called that a broken guard; the structural test
        # (the closing #endif must end the file) does not.
        self.assertEqual([], self.judge({
            WITNESS: WITNESS_TEXT,
            "at_log.h": "#ifndef LOG_TAG\n#define LOG_TAG \"at\"\n#endif\nvoid at_log(const char *);\n",
        }))

    def test_pragma_once_is_not_a_guard(self) -> None:
        self.assertEqual([], self.judge({WITNESS: WITNESS_TEXT,
                                         "shim.h": "#pragma once\n#include \"vendor.h\"\n"}))

    def test_an_unterminated_conditional_is_not_a_guard(self) -> None:
        self.assertEqual([], self.judge({WITNESS: WITNESS_TEXT,
                                         "truncated.h": "#ifndef TRUNCATED_H\n#define TRUNCATED_H\n"}))

    def test_a_pin_with_a_reason_silences_a_divergent_collision(self) -> None:
        files = {
            WITNESS: WITNESS_TEXT,
            "components/clib/xy_clib/xy_stdio.h": guarded("XY_STDIO_H"),
            "components/trace/xy_log/inc/xy_stdio.h": guarded("XY_STDIO_H", "int xy_log_printf(void);"),
        }
        pins = {"XY_STDIO_H": (("components/clib/xy_clib/xy_stdio.h",
                                "components/trace/xy_log/inc/xy_stdio.h"),
                               "two stdio surfaces, converging them changes the stdio API")}
        self.assertEqual([], self.judge(files, pins))

    def test_a_pin_without_a_reason_is_reported(self) -> None:
        files = {
            WITNESS: WITNESS_TEXT,
            "a/one.h": guarded("SHARED_H"),
            "b/two.h": guarded("SHARED_H", "int two(void);"),
        }
        pins = {"SHARED_H": (("a/one.h", "b/two.h"), "   ")}
        problems = self.judge(files, pins)
        self.assertEqual(1, len(problems), problems)
        self.assertIn("without a stated reason", problems[0])

    def test_a_pin_that_outlived_the_collision_is_reported(self) -> None:
        files = {WITNESS: WITNESS_TEXT, "a/one.h": guarded("ONE_H")}
        pins = {"ONE_H": (("a/one.h", "b/two.h"), "was a collision once")}
        problems = self.judge(files, pins)
        self.assertTrue(any("outlived" in problem for problem in problems), problems)
        self.assertTrue(any("does not exist" in problem for problem in problems), problems)

    def test_a_pin_that_names_the_wrong_files_is_reported(self) -> None:
        files = {
            WITNESS: WITNESS_TEXT,
            "a/one.h": guarded("SHARED_H"),
            "b/two.h": guarded("SHARED_H", "int two(void);"),
        }
        pins = {"SHARED_H": (("a/one.h", "c/three.h"), "pinned for files that moved")}
        problems = self.judge(files, pins)
        self.assertTrue(any("now" in problem for problem in problems), problems)

    def test_a_pin_of_identical_files_is_reported(self) -> None:
        same = guarded("SHARED_H")
        files = {WITNESS: WITNESS_TEXT, "a/one.h": same, "b/two.h": same}
        pins = {"SHARED_H": (("a/one.h", "b/two.h"), "kept for historical reasons")}
        problems = self.judge(files, pins)
        self.assertTrue(any("now identical" in problem for problem in problems), problems)

    def test_a_same_directory_collision_cannot_be_pinned(self) -> None:
        # Include order cannot excuse it: there is one place `#include "x.h"` can resolve to,
        # so renaming is the only fix.
        files = {
            WITNESS: WITNESS_TEXT,
            "inc/a.h": guarded("SHARED_H"),
            "inc/b.h": guarded("SHARED_H", "int b(void);"),
        }
        pins = {"SHARED_H": (("inc/a.h", "inc/b.h"), "we will get to it")}
        problems = self.judge(files, pins)
        self.assertTrue(any("cannot be pinned" in problem for problem in problems), problems)

    def test_the_repository_itself_is_clean(self) -> None:
        problems = guard.findings()
        self.assertEqual([], problems)

    def test_a_colliding_guard_hides_the_second_header(self) -> None:
        """The claim, measured: two headers, one guard, and the second one is not read."""
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "first.h").write_text(guarded("_SHARED_H_", "int first(void);"), encoding="utf-8")
            (root / "second.h").write_text(guarded("_SHARED_H_", "int second(void);"), encoding="utf-8")
            (root / "user.c").write_text('#include "first.h"\n#include "second.h"\n', encoding="utf-8")

            def preprocessed() -> str:
                return subprocess.run(
                    ["gcc", "-E", "-P", str(root / "user.c")],
                    check=True, capture_output=True, text=True).stdout

            try:
                text = preprocessed()
            except FileNotFoundError:
                self.skipTest("gcc is not available; the claim is proven by the PC build instead")
            self.assertIn("int first(void);", text)
            self.assertNotIn("int second(void);", text,
                             "the second header survived a shared guard - the finding is wrong")

            (root / "second.h").write_text(guarded("_SECOND_H_", "int second(void);"), encoding="utf-8")
            self.assertIn("int second(void);", preprocessed(),
                          "with separate guards both headers must be read")


if __name__ == "__main__":
    unittest.main()
