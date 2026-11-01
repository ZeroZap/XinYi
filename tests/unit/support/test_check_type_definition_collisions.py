#!/usr/bin/env python3
"""Self-test for the one-type-name-two-definitions rule.

The rule's whole value is discriminating: it has to report a name defined twice with different
bodies, stay quiet about a name defined once, stay quiet about a difference that is only a comment or
only formatting, and notice when a pin it carries has stopped being true. Every one of those is a case
here, because a rule that reports everything and a rule that reports nothing look identical from a
green run.

Fixtures are real files in a temporary tree: the rule walks the tree, so the tree is what it judges.
"""

from __future__ import annotations

import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import check_type_definition_collisions as guard  # noqa: E402


class CollisionTests(unittest.TestCase):
    def setUp(self) -> None:
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.root = Path(self.directory.name)

    def write(self, relative: str, text: str) -> None:
        path = self.root / relative
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text, encoding="utf-8")

    def judge(self, pins: dict[str, str] | None = None) -> list[str]:
        return guard.findings(self.root, {} if pins is None else pins)

    def test_one_name_with_two_different_bodies_is_reported(self):
        self.write("a/one.h", "typedef enum { A = 0 } colour_t;\n")
        self.write("b/two.h", "typedef enum { A = 1 } colour_t;\n")
        problems = self.judge()
        self.assertEqual(1, len(problems))
        self.assertIn("colour_t", problems[0])
        self.assertIn("a/one.h", problems[0])
        self.assertIn("b/two.h", problems[0])

    def test_an_enum_and_a_struct_under_one_name_still_collide(self):
        # The rule compares bodies, not kinds: `typedef enum {...} X` and `typedef struct {...} X` are
        # two different meanings for one name, which is the defect and not a near-miss.
        self.write("a.h", "typedef enum { A } thing_t;\n")
        self.write("b.h", "typedef struct { int a; } thing_t;\n")
        self.assertEqual(1, len(self.judge()))

    def test_one_definition_is_not_a_collision(self):
        self.write("a.h", "typedef enum { A } colour_t;\n")
        self.assertEqual([], self.judge())

    def test_two_headers_spelling_the_same_definition_are_still_a_collision(self):
        # Byte-identical copies disagree the moment one of them is edited, and nothing makes that
        # impossible. The rule asks whether two owners exist, not whether they currently agree.
        self.write("a.h", "typedef enum { A } colour_t;\n")
        self.write("b.h", "typedef   enum  {  A  }  colour_t ;\n")
        self.assertEqual(1, len(self.judge()))

    def test_a_comment_difference_is_not_a_body_difference_but_the_copy_still_counts(self):
        self.write("a.h", "typedef enum { A /* first */ } colour_t;\n")
        self.write("b.h", "typedef enum { A /* second */ } colour_t;\n")
        problems = self.judge()
        self.assertEqual(1, len(problems), "two definitions must still be reported")
        self.assertIn("colour_t", problems[0])

    def test_a_commented_out_definition_is_not_a_definition(self):
        # The discriminating case: without comment stripping this reads as two owners of `colour_t`
        # and invents a collision, which is the same defect as a commented-out branch satisfying a
        # contract - only pointed the other way.
        self.write("a.h", "/* typedef enum { A = 0 } colour_t; */\n")
        self.write("b.h", "typedef enum { A = 1 } colour_t;\n")
        self.assertEqual([], self.judge())

    def test_a_pinned_name_passes(self):
        self.write("a.h", "typedef enum { A = 0 } colour_t;\n")
        self.write("b.h", "typedef enum { A = 1 } colour_t;\n")
        self.assertEqual([], self.judge({"colour_t": "two subsystems, decision pending"}))

    def test_a_pin_without_a_reason_is_reported(self):
        self.write("a.h", "typedef enum { A = 0 } colour_t;\n")
        self.write("b.h", "typedef enum { A = 1 } colour_t;\n")
        problems = self.judge({"colour_t": "   "})
        self.assertEqual(1, len(problems))
        self.assertIn("without stating why", problems[0])

    def test_a_pin_whose_collision_disappeared_is_reported(self):
        self.write("a.h", "typedef enum { A = 0 } colour_t;\n")
        problems = self.judge({"colour_t": "was a collision once"})
        self.assertEqual(1, len(problems))
        self.assertIn("defined once", problems[0])

    def test_a_pin_for_a_name_nobody_defines_is_reported(self):
        self.write("a.h", "typedef enum { A = 0 } other_t;\n")
        problems = self.judge({"colour_t": "was a collision once"})
        self.assertEqual(1, len(problems))
        self.assertIn("no header defines it", problems[0])

    def test_a_pinned_collision_whose_file_vanished_is_reported(self):
        self.write("a.h", "typedef enum { A = 0 } colour_t;\n")
        self.write("b.h", "typedef enum { A = 1 } colour_t;\n")
        pins = {"colour_t": "documented"}
        self.assertEqual([], self.judge(pins))
        (self.root / "b.h").unlink()
        problems = self.judge(pins)
        self.assertTrue(problems)
        self.assertIn("defined once", problems[0])

    def test_a_vendored_subtree_is_not_this_repositorys_collision(self):
        # The two vendored samples in this tree ship an STM32F0 and an STM32F4 peripheral library and
        # disagree about GPIOSpeed_TypeDef by construction; reporting that would bury the real ones.
        self.write("components/crypto/curve25519-cortexm0-20150813/x.h",
                   "typedef enum { A = 0 } speed_t;\n")
        self.write("components/net/at/AT-Command-V2/x.h", "typedef enum { A = 1 } speed_t;\n")
        self.write("components/mine/inc/mine.h", "typedef enum { A = 0 } own_t;\n")
        self.assertEqual([], self.judge())

    def test_a_vendored_file_next_to_a_project_file_is_still_excluded(self):
        self.write("components/crypto/curve25519-cortexm0-20150813/x.h",
                   "typedef enum { A = 0 } speed_t;\n")
        self.write("components/crypto/inc/y.h", "typedef enum { A = 1 } speed_t;\n")
        self.assertEqual([], self.judge(), "a single remaining definition is not a collision")

    def test_an_empty_walk_is_reported_rather_than_passing(self):
        problems = self.judge()
        self.assertEqual(1, len(problems))
        self.assertIn("walk is broken", problems[0])

    def test_the_real_tree_finds_names_and_reports_nothing(self):
        # Non-empty and green: a walk that read nothing cannot hide behind this, and the pins must
        # still describe something real.
        types = guard.corpus()
        self.assertGreater(len(types), 100)
        self.assertEqual([], guard.findings(guard.ROOT))


if __name__ == "__main__":
    unittest.main()
