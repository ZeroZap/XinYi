#!/usr/bin/env python3
"""Contract tests for the comment stripping used by the boundary guards.

The helper exists because a comment naming a forbidden call turned a guard red while the
code did the right thing. These tests pin both halves of the behaviour: comments go, code
stays, and a literal is left alone.
"""

from __future__ import annotations

import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "support"))

from source_scan import (code_only, code_only_hash, fenced_blocks, read_code,  # noqa: E402
                         read_hash_code, read_raw)


class CodeOnlyTest(unittest.TestCase):
    def test_a_line_comment_is_removed(self) -> None:
        self.assertNotIn("__HAL_RCC_", code_only("// why not __HAL_RCC_X()\nint a;"))

    def test_a_block_comment_is_removed(self) -> None:
        source = "/* explains __HAL_RCC_CLEAR_RESET_FLAGS() */\nint a;"
        self.assertNotIn("__HAL_RCC_", code_only(source))

    def test_code_survives(self) -> None:
        source = "/* note */\nvoid f(void) { HAL_GPIO_Init(&g); }\n"
        self.assertIn("HAL_GPIO_Init", code_only(source))

    def test_a_literal_is_not_treated_as_a_comment(self) -> None:
        # A symbol inside a string is still a symbol someone typed.
        source = 'const char *s = "HAL_GPIO_Init";\n'
        self.assertIn("HAL_GPIO_Init", code_only(source))

    def test_a_comment_inside_a_literal_is_kept(self) -> None:
        source = 'const char *s = "a // b";\n'
        self.assertIn("a // b", code_only(source))

    def test_a_multiline_block_comment_is_handled(self) -> None:
        source = "/*\n * __HAL_RCC_A\n * __HAL_RCC_B\n */\nint a;\n"
        self.assertNotIn("__HAL_RCC_", code_only(source))


class ReadHelpersTest(unittest.TestCase):
    def setUp(self) -> None:
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.path = Path(self.directory.name) / "sample.c"

    def test_read_code_strips_a_comment(self) -> None:
        self.path.write_text("/* uses HAL_UART_Receive_IT */\nint a;\n", encoding="utf-8")
        self.assertNotIn("HAL_UART_Receive_IT", read_code(self.path))

    def test_read_code_keeps_the_code(self) -> None:
        self.path.write_text("/* note */\nvoid f(void) { HAL_GPIO_Init(&g); }\n", encoding="utf-8")
        self.assertIn("HAL_GPIO_Init", read_code(self.path))

    def test_read_raw_does_not_strip(self) -> None:
        # A Kconfig/CMake/Markdown read must not look as if it went through the C stripper.
        self.path.write_text("config NETWORK  # not a C comment\n", encoding="utf-8")
        self.assertIn("# not a C comment", read_raw(self.path))

    def test_read_raw_accepts_a_string_path(self) -> None:
        self.path.write_text("int a;\n", encoding="utf-8")
        self.assertIn("int a;", read_raw(str(self.path)))


class HashCodeOnlyTest(unittest.TestCase):
    """Kconfig and CMake comments: a guard that matches a line must match a live line."""

    def test_a_kconfig_comment_is_removed(self) -> None:
        source = "config OSAL_BACKEND_FREERTOS\n# config OSAL_BACKEND_RTX5\n"
        stripped = code_only_hash(source)
        self.assertIn("config OSAL_BACKEND_FREERTOS", stripped)
        self.assertNotIn("OSAL_BACKEND_RTX5", stripped)

    def test_a_cmake_dispatch_branch_in_a_comment_is_removed(self) -> None:
        source = '# elseif(OSAL_BACKEND STREQUAL "freertos")\n'
        self.assertNotIn("STREQUAL", code_only_hash(source))

    def test_code_after_a_trailing_comment_survives(self) -> None:
        source = 'if(A) # CONFIG_B is only prose\n    set(B 1)\nendif()\n'
        stripped = code_only_hash(source)
        self.assertIn("set(B 1)", stripped)
        self.assertNotIn("CONFIG_B", stripped)

    def test_a_hash_inside_a_quoted_string_is_data(self) -> None:
        # CMake: set(X "#") is a value; Kconfig: string options are quoted too.
        source = 'set(OSAL_BACKEND "#1" CACHE STRING "c # d")\n'
        self.assertIn("#1", code_only_hash(source))
        self.assertIn("c # d", code_only_hash(source))

    def test_a_bracket_comment_keeps_its_line_count(self) -> None:
        source = "#[[\nif(OSAL_BACKEND STREQUAL \"gone\")\n]]\nif(A)\nendif()\n"
        stripped = code_only_hash(source)
        self.assertNotIn("gone", stripped)
        self.assertEqual(source.count("\n"), stripped.count("\n"))

    def test_an_unterminated_quote_does_not_eat_the_next_line(self) -> None:
        # A stray quote must not turn the rest of the file into a string.
        source = 'set(A "unterminated)\nif(B)\nendif()\n'
        self.assertIn("if(B)", code_only_hash(source))

    def test_a_trailing_comment_does_not_hide_the_line(self) -> None:
        source = 'set(A 1)# note\nset(B 2)\n'
        self.assertEqual(["set(A 1)", "set(B 2)"],
                         [line for line in code_only_hash(source).splitlines() if line])


class ReadHashCodeTest(unittest.TestCase):
    def setUp(self) -> None:
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.path = Path(self.directory.name) / "CMakeLists.txt"

    def test_read_hash_code_strips_a_commented_branch(self) -> None:
        self.path.write_text('# elseif(OSAL_BACKEND STREQUAL "freertos")\n', encoding="utf-8")
        self.assertNotIn("STREQUAL", read_hash_code(self.path))

    def test_read_hash_code_keeps_live_lines(self) -> None:
        self.path.write_text('if(OSAL_BACKEND STREQUAL "freertos")\nendif()\n', encoding="utf-8")
        self.assertIn("freertos", read_hash_code(self.path))

    def test_read_hash_code_accepts_a_string_path(self) -> None:
        self.path.write_text("# note\nset(A 1)\n", encoding="utf-8")
        self.assertIn("set(A 1)", read_hash_code(str(self.path)))


class FencedBlocksTest(unittest.TestCase):
    """The one fenced-block reader, shared by the documentation guards.

    A single regex over the whole document cannot tell a nested fence from a closing one, and two
    guards with their own readers is how they start disagreeing about what a code block is.
    """

    def test_a_backtick_block_reports_its_info_string_and_first_line(self) -> None:
        # "# Doc", "", "```c", "int a;" -> the content starts on line 4.
        self.assertEqual([("c", 4, "int a;")], fenced_blocks("# Doc\n\n```c\nint a;\n```\n"))

    def test_a_tilde_block_opens_and_closes(self) -> None:
        self.assertEqual([("bash", 2, "make")], fenced_blocks("~~~bash\nmake\n~~~\n"))

    def test_a_four_backtick_fence_is_not_closed_by_three(self) -> None:
        text = "````markdown\n```c\nint a;\n```\n````\n"
        blocks = fenced_blocks(text)
        self.assertEqual(1, len(blocks))
        self.assertIn("```c", blocks[0][2])

    def test_a_block_whose_terminator_never_arrives_runs_to_the_end(self) -> None:
        self.assertEqual([("c", 2, "int a;\nint b;")], fenced_blocks("```c\nint a;\nint b;\n"))

    def test_an_indented_by_four_spaces_fence_is_not_a_fence(self) -> None:
        self.assertEqual([], fenced_blocks("    ```c\n    int a;\n    ```\n"))

    def test_an_indented_by_three_spaces_fence_is_a_fence(self) -> None:
        self.assertEqual([("c", 2, "int a;")], fenced_blocks("   ```c\nint a;\n   ```\n"))

    def test_prose_outside_a_fence_is_not_a_block(self) -> None:
        self.assertEqual([], fenced_blocks("# Doc\n\nNo block here.\n"))


if __name__ == "__main__":
    unittest.main()
