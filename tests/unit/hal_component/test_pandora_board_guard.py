#!/usr/bin/env python3
"""Cases for the Pandora board guard's require() contract.

require() used to read the raw file, so a comment satisfied it: a board that only said
`// TODO: call xy_hal_sensor_register` in the right file passed a check that the call exists.
Reading the code instead is the point of the change, and it has to keep two properties:

  * a comment mentioning a symbol is not the symbol (otherwise the contract is prose);
  * a symbol typed into a string literal still counts (it is something someone typed, and
    dropping it would be a false negative in the direction that matters).

The guard itself runs against a live board tree, where both cases happen to be absent - which
is exactly when a guard can quietly stop checking anything.
"""

import importlib.util
import tempfile
import unittest
from pathlib import Path

GUARD = Path(__file__).resolve().parent / "check_pandora_stm32l475_board.py"


def load_guard():
    spec = importlib.util.spec_from_file_location("pandora_board_guard", GUARD)
    assert spec is not None and spec.loader is not None
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


class RequireTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.guard = load_guard()

    def write(self, text: str) -> Path:
        directory = tempfile.mkdtemp()
        path = Path(directory) / "board.c"
        path.write_text(text, encoding="utf-8")
        return path

    def test_a_symbol_in_code_satisfies_the_contract(self):
        path = self.write("void f(void) { xy_hal_sensor_register(&s); }\n")
        self.guard.require(path, "xy_hal_sensor_register")

    def test_a_symbol_only_in_a_comment_does_not(self):
        path = self.write("/* TODO: call xy_hal_sensor_register here */\nvoid f(void) {}\n")
        with self.assertRaises(AssertionError) as raised:
            self.guard.require(path, "xy_hal_sensor_register")
        self.assertIn("a comment mentioning it does not satisfy this", str(raised.exception))

    def test_a_symbol_only_in_a_line_comment_does_not(self):
        path = self.write("void f(void) {}\n// xy_hal_sensor_register(&s);\n")
        with self.assertRaises(AssertionError):
            self.guard.require(path, "xy_hal_sensor_register")

    def test_a_symbol_inside_a_string_literal_still_counts(self):
        # Not a comment: someone typed this. Dropping it would hide a real occurrence.
        path = self.write('const char *name = "xy_hal_sensor_register";\n')
        self.guard.require(path, "xy_hal_sensor_register")

    def test_a_missing_file_is_reported_as_missing(self):
        with self.assertRaises(AssertionError) as raised:
            self.guard.require(Path("/nonexistent/board.c"), "anything")
        self.assertIn("missing", str(raised.exception))

    def test_every_needle_is_required(self):
        path = self.write("void f(void) { xy_hal_a(); }\n")
        with self.assertRaises(AssertionError):
            self.guard.require(path, "xy_hal_a", "xy_hal_b")


if __name__ == "__main__":
    unittest.main(verbosity=1)
