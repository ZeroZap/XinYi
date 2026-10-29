#!/usr/bin/env python3
"""Contract tests for the comment stripping used by the boundary guards.

The helper exists because a comment naming a forbidden call turned a guard red while the
code did the right thing. These tests pin both halves of the behaviour: comments go, code
stays, and a literal is left alone.
"""

from __future__ import annotations

import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "support"))

from source_scan import code_only  # noqa: E402


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


if __name__ == "__main__":
    unittest.main()
