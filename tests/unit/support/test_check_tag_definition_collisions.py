#!/usr/bin/env python3
"""Self-test for the tag-collision rule.

The rule exists because `struct xy_device` was defined in two headers and nothing noticed until the
umbrella header started including both. The cases below pin down the discrimination it needs: a tag
with two bodies is a finding, a tag with one body is not, and a difference written in a comment or
produced by reformatting is not a difference.
"""

from __future__ import annotations

import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import check_tag_definition_collisions as guard  # noqa: E402


class TagCollisionTests(unittest.TestCase):
    def setUp(self) -> None:
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.root = Path(self.directory.name)
        self.write("witness/inc/witness.h", "struct witness_baseline { int a; };\n")

    def write(self, relative: str, text: str) -> None:
        path = self.root / relative
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text, encoding="utf-8")

    def judge(self, pins: dict[str, str] | None = None) -> list[str]:
        return guard.findings(self.root, {} if pins is None else pins)

    def test_one_tag_with_two_bodies_is_reported(self):
        self.write("a.h", "struct thing { int a; };\n")
        self.write("b.h", "struct thing { const char *name; int a; };\n")
        problems = self.judge()
        self.assertEqual(1, len(problems))
        self.assertIn("struct thing", problems[0])
        self.assertIn("a.h", problems[0])
        self.assertIn("b.h", problems[0])

    def test_a_struct_and_a_union_with_one_name_do_not_collide(self):
        # `struct thing` and `union thing` are separate namespaces in C: declaring both is legal.
        self.write("a.h", "struct thing { int a; };\n")
        self.write("b.h", "union thing { int a; };\n")
        self.assertEqual([], self.judge())

    def test_two_definitions_in_one_file_are_not_a_cross_header_collision(self):
        # Platform-conditional variants of one tag inside one header are a different thing from two
        # owners: the file is the owner either way, so it counts once.
        self.write("a.h",
                   "#ifdef MODEL_A\nstruct thing { int a; };\n#endif\n"
                   "#ifdef MODEL_B\nstruct thing { int b; };\n#endif\n")
        self.assertEqual(["a.h"], guard.corpus(self.root)["struct thing"])
        self.assertEqual([], self.judge())

    def test_a_commented_out_tag_is_not_a_definition(self):
        # The discriminating case for comment stripping: without read_code() this reads as two owners
        # of `struct thing` and invents a collision.
        self.write("a.h", "/* struct thing { int a; }; */\n")
        self.write("b.h", "struct thing { int b; };\n")
        self.assertEqual([], self.judge())

    def test_an_enum_tag_is_covered_too(self):
        self.write("a.h", "enum mode { MODE_A = 0 };\n")
        self.write("b.h", "enum mode { MODE_A = 1 };\n")
        problems = self.judge()
        self.assertEqual(1, len(problems))
        self.assertIn("enum mode", problems[0])

    def test_one_definition_is_not_a_collision(self):
        self.write("a.h", "struct thing { int a; };\n")
        self.assertEqual([], self.judge())

    def test_a_typedef_without_a_tag_is_not_a_tag(self):
        # Most types in this tree look like this, which is why the class is small - and why a typedef
        # must not be reported here; the typedef guard owns those.
        self.write("a.h", "typedef struct { int a; } thing_t;\n")
        self.write("b.h", "typedef struct { int b; } thing_t;\n")
        self.assertEqual([], self.judge())

    def test_identical_bodies_in_two_headers_are_still_a_collision(self):
        # C forbids defining a tag twice in one translation unit even when the bodies match, so the
        # rule reports the second definition whatever it holds. Comparing bodies would only change the
        # wording, not the verdict - which is why every repeat is pinned rather than only the divergent
        # ones, unlike the function rule where a repeated identical prototype is legal.
        self.write("a.h", "struct thing { int a; };\n")
        self.write("b.h", "struct thing { int a; };\n")
        self.assertEqual(1, len(self.judge()))

    def test_a_single_definition_with_nested_braces_stays_one_tag(self):
        self.write("a.h", "struct thing { struct { int x; } inner; int a; };\n")
        tags = guard.corpus(self.root)
        self.assertIn("struct thing", tags)
        self.assertFalse([name for name in tags if "inner" in name],
                         "the anonymous inner struct must not become a tag")

    def test_reformatting_does_not_change_what_a_tag_is(self):
        self.write("a.h", "struct thing { int a; int b; };\n")
        self.write("b.h", "struct thing {\n    int a;\n    int b;\n};\n")
        problems = self.judge()
        self.assertEqual(1, len(problems), "still a collision, and reported as one")
        self.assertIn("struct thing", problems[0])

    def test_a_pinned_tag_passes(self):
        self.write("a.h", "struct thing { int a; };\n")
        self.write("b.h", "struct thing { int b; };\n")
        self.assertEqual([], self.judge({"struct thing": "decision pending"}))

    def test_a_pin_without_a_reason_is_reported(self):
        self.write("a.h", "struct thing { int a; };\n")
        self.write("b.h", "struct thing { int b; };\n")
        problems = self.judge({"struct thing": "   "})
        self.assertEqual(1, len(problems))
        self.assertIn("without stating why", problems[0])

    def test_a_pin_whose_collision_disappeared_is_reported(self):
        self.write("a.h", "struct thing { int a; };\n")
        problems = self.judge({"struct thing": "was a collision once"})
        self.assertEqual(1, len(problems))
        self.assertIn("defined once", problems[0])

    def test_a_vendored_subtree_is_not_this_repositorys_collision(self):
        self.write("components/kernel/osal/portable/freertos/port.h", "struct port { int a; };\n")
        self.write("components/kernel/osal/xy_port.h", "struct port { int b; };\n")
        self.assertEqual([], self.judge())

    def test_an_empty_walk_is_reported_rather_than_passing(self):
        empty = tempfile.TemporaryDirectory()
        self.addCleanup(empty.cleanup)
        problems = guard.findings(Path(empty.name), {})
        self.assertEqual(1, len(problems))
        self.assertIn("walk is broken", problems[0])

    def test_the_real_tree_finds_tags_and_reports_nothing(self):
        tags = guard.corpus()
        self.assertGreater(len(tags), 50)
        self.assertEqual([], guard.findings(guard.ROOT))


if __name__ == "__main__":
    unittest.main()
