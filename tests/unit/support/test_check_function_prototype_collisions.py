#!/usr/bin/env python3
"""Self-test for the one-name-two-prototypes rule.

Two of these cases exist because the rule got them wrong while it was being built, and both would have
made it report confidently wrong findings:

  * a header wrapped in `extern "C" { ... }` lost every declaration, because the wrapper was treated
    as a type body and dropped whole;
  * `static inline` helpers inside that wrapper leaked their bodies into the next statement, so the
    function they *call* was read as the function being declared.

The rest guard the discrimination the rule needs: incompatible declarations are findings, identical
ones are legal C and must stay quiet, and a difference that is only pointer spacing is not a
difference.
"""

from __future__ import annotations

import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import check_function_prototype_collisions as guard  # noqa: E402


class PrototypeCollisionTests(unittest.TestCase):
    def setUp(self) -> None:
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.root = Path(self.directory.name)
        self.write("witness/inc/witness.h", "int witness_baseline(void);\n")

    def write(self, relative: str, text: str) -> None:
        path = self.root / relative
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text, encoding="utf-8")

    def judge(self, pins: dict[str, str] | None = None) -> list[str]:
        return guard.findings(self.root, {} if pins is None else pins)

    def test_two_incompatible_prototypes_are_reported(self):
        self.write("a.h", "int run(int count);\n")
        self.write("b.h", "int run(void *frame, int flags);\n")
        problems = self.judge()
        self.assertEqual(1, len(problems))
        self.assertIn("run", problems[0])
        self.assertIn("a.h", problems[0])
        self.assertIn("b.h", problems[0])

    def test_a_return_type_difference_is_reported(self):
        self.write("a.h", "int start(void);\n")
        self.write("b.h", "void start(void);\n")
        self.assertEqual(1, len(self.judge()))

    def test_an_identical_redeclaration_is_legal_and_not_reported(self):
        # C allows repeated identical declarations, so this must stay quiet - it is what separates this
        # rule from the typedef one, where every repeat is illegal.
        self.write("a.h", "int run(int count);\n")
        self.write("b.h", "int run(int count);\n")
        self.assertEqual([], self.judge())

    def test_pointer_spacing_is_not_a_prototype_difference(self):
        self.write("a.h", "void fail(uint8_t *file, uint32_t line);\n")
        self.write("b.h", "void fail(uint8_t* file, uint32_t line);\n")
        self.assertEqual([], self.judge())

    def test_extern_c_wrapping_does_not_hide_the_declarations(self):
        # The regression: a header whose whole body sits inside `extern "C"` must still be read.
        self.write("a.h", 'extern "C" {\nint run(int count);\n}\n')
        self.write("b.h", "int run(void *frame);\n")
        problems = self.judge()
        self.assertEqual(1, len(problems))
        self.assertIn("run", problems[0])

    def test_a_static_inline_body_does_not_declare_the_function_it_calls(self):
        # The leak: without walking the wrapper again, `wrapped_call(...)` inside the body reads as a
        # second declaration of the function being called, inventing a collision.
        self.write("a.h",
                   'extern "C" {\n'
                   'static inline int wrapper(void *frame) { return run(frame, 1); }\n'
                   '}\n')
        self.write("b.h", "int run(void *frame);\n")
        self.assertEqual([], self.judge())

    def test_a_function_pointer_parameter_does_not_become_the_declared_name(self):
        # Scanning forwards for the first parenthesis names the callback parameter `on_event` instead
        # of the function `register_handler`, which invents a declaration of a function that does not
        # exist and hides the one that does.
        self.write("a.h", "void register_handler(void (*on_event)(int code));\n")
        declared = guard.corpus(self.root)
        self.assertIn("register_handler", declared)
        self.assertNotIn("on_event", declared)
        self.assertNotIn("code", declared)

    def test_parameter_names_are_not_part_of_the_prototype(self):
        # `int run(int count)` and `int run(int flags)` declare the same function. Reporting a rename
        # as an incompatibility would be the same defect this rule looks for, pointing the other way.
        self.write("a.h", "int run(int count);\n")
        self.write("b.h", "int run(int flags);\n")
        self.assertEqual([], self.judge())

    def test_pointer_and_array_parameter_names_are_not_part_of_the_prototype(self):
        self.write("a.h", "int fill(uint8_t *buffer, const char *text);\n")
        self.write("b.h", "int fill(uint8_t *store, const char *label);\n")
        self.write("c.h", "int seal(uint8_t signature[64]);\n")
        self.write("d.h", "int seal(uint8_t mac[64]);\n")
        self.assertEqual([], self.judge())

    def test_a_pinned_name_passes(self):
        self.write("a.h", "int run(int count);\n")
        self.write("b.h", "int run(void *frame);\n")
        self.assertEqual([], self.judge({"run": "two subsystems, decision pending"}))

    def test_a_pin_without_a_reason_is_reported(self):
        self.write("a.h", "int run(int count);\n")
        self.write("b.h", "int run(void *frame);\n")
        problems = self.judge({"run": "  "})
        self.assertEqual(1, len(problems))
        self.assertIn("without stating why", problems[0])

    def test_a_pin_whose_incompatibility_disappeared_is_reported(self):
        self.write("a.h", "int run(int count);\n")
        self.write("b.h", "int run(int count);\n")
        problems = self.judge({"run": "was incompatible once"})
        self.assertEqual(1, len(problems))
        self.assertIn("declared consistently", problems[0])

    def test_a_pin_for_an_undeclared_name_is_reported(self):
        problems = self.judge({"run": "was incompatible once"})
        self.assertEqual(1, len(problems))
        self.assertIn("declared consistently", problems[0])

    def test_a_vendored_subtree_is_not_this_repositorys_collision(self):
        self.write("components/kernel/osal/portable/freertos/port.h", "void port_enter(void);\n")
        self.write("components/kernel/osal/xy_os.h", "int port_enter(int flags);\n")
        self.assertEqual([], self.judge())

    def test_an_empty_walk_is_reported_rather_than_passing(self):
        empty = tempfile.TemporaryDirectory()
        self.addCleanup(empty.cleanup)
        problems = guard.findings(Path(empty.name), {})
        self.assertEqual(1, len(problems))
        self.assertIn("walk is broken", problems[0])

    def test_the_real_tree_finds_names_and_reports_nothing(self):
        declared = guard.corpus()
        self.assertGreater(len(declared), 100)
        self.assertEqual([], guard.findings(guard.ROOT))


if __name__ == "__main__":
    unittest.main()
