#!/usr/bin/env python3
"""Self-test for check_kconfig_fragment_agreement.py.

The rule exists because of one concrete mistake: `select COMPONENT_DEVICE` was added to
`components/fuel_gauge/Kconfig`, which looked like a fix and changed nothing, because the build
reads only the root `Kconfig`. The decisive cases below are therefore the fragment that promises a
select the root does not have, and the block reader that must not let one `config` block's
`depends on` lines leak into the next one - the first version of this parser did exactly that and
reported GUI_WIDGETS depending on FOTA and PM.
"""

from __future__ import annotations

import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from check_kconfig_fragment_agreement import (  # noqa: E402
    REPO_ROOT,
    check,
    parse_blocks,
)

ROOT = """\
config COMPONENT_A
    bool "A"
    default y
    help
      A component.

config COMPONENT_B
    bool "B"
    default n
    select COMPONENT_A
"""


def _tree(root_text: str, fragment_text: str) -> Path:
    base = Path(tempfile.mkdtemp())
    (base / "Kconfig").write_text(root_text)
    component = base / "components" / "demo"
    component.mkdir(parents=True)
    (component / "Kconfig").write_text(fragment_text)
    return base


class BlockReader(unittest.TestCase):
    def test_selects_do_not_leak_into_the_next_block(self) -> None:
        blocks = parse_blocks(ROOT)
        self.assertEqual(["COMPONENT_A"], blocks["COMPONENT_B"]["selects"])
        self.assertEqual([], blocks["COMPONENT_A"]["selects"])

    def test_help_prose_does_not_become_a_dependency(self) -> None:
        text = ROOT + """
config COMPONENT_C
    bool "C"
    depends on COMPONENT_A
"""
        blocks = parse_blocks(text)
        self.assertEqual(["COMPONENT_A"], blocks["COMPONENT_C"]["depends"])
        self.assertEqual([], blocks["COMPONENT_B"]["depends"])


class Divergence(unittest.TestCase):
    def test_agreeing_fragment_passes(self) -> None:
        tree = _tree(ROOT, "config COMPONENT_B\n    bool \"B\"\n    select COMPONENT_A\n")
        failures, _, counts = check(tree, pins={})
        self.assertEqual([], failures)
        self.assertEqual(1, counts["shared"])

    def test_fragment_select_the_root_lacks_fails(self) -> None:
        """The trap this slice hit: a select that only the unread file declares."""
        tree = _tree(ROOT, "config COMPONENT_A\n    bool \"A\"\n    select COMPONENT_B\n")
        failures, _, _ = check(tree, pins={})
        self.assertEqual(1, len(failures))
        self.assertIn("COMPONENT_A selects COMPONENT_B", failures[0])
        self.assertIn("the build never applies this select", failures[0])

    def test_unpinned_depends_divergence_fails(self) -> None:
        tree = _tree(ROOT, "config COMPONENT_A\n    bool \"A\"\n    depends on COMPONENT_B\n")
        failures, _, _ = check(tree, pins={})
        self.assertEqual(1, len(failures))
        self.assertIn("depends on ['COMPONENT_B']", failures[0])

    def test_pinned_depends_divergence_is_reported_not_failed(self) -> None:
        tree = _tree(ROOT, "config COMPONENT_A\n    bool \"A\"\n    depends on COMPONENT_B\n")
        pins = {("components/demo/Kconfig", "COMPONENT_A"): "root is intentionally stricter"}
        failures, notes, _ = check(tree, pins=pins)
        self.assertEqual([], failures)
        self.assertTrue(any(note.startswith("pinned:") for note in notes))

    def test_pin_expires_when_the_fragment_agrees_again(self) -> None:
        tree = _tree(ROOT, "config COMPONENT_A\n    bool \"A\"\n")
        pins = {("components/demo/Kconfig", "COMPONENT_A"): "root is intentionally stricter"}
        failures, _, _ = check(tree, pins=pins)
        self.assertEqual(1, len(failures))
        self.assertIn("pin has expired", failures[0])

    def test_fragment_only_symbol_is_inventory_not_a_failure(self) -> None:
        tree = _tree(ROOT, "config COMPONENT_Z\n    bool \"Z\"\n    select COMPONENT_B\n")
        failures, notes, counts = check(tree, pins={})
        self.assertEqual([], failures)
        self.assertEqual(1, counts["fragment_only"])
        self.assertTrue(any("COMPONENT_Z" in note for note in notes))


class RealTree(unittest.TestCase):
    def test_current_tree_is_consistent(self) -> None:
        failures, _, counts = check(REPO_ROOT)
        self.assertEqual([], failures)
        self.assertGreater(counts["fragments"], 0)

    def test_fuel_gauge_select_lives_in_the_authoritative_file(self) -> None:
        """The dependency is only real in the root Kconfig; the fragment is a mirror."""
        blocks = parse_blocks((REPO_ROOT / "Kconfig").read_text())
        self.assertIn("COMPONENT_DEVICE", blocks["COMPONENT_FUEL_GAUGE"]["selects"])
        fragment = parse_blocks((REPO_ROOT / "components" / "fuel_gauge" / "Kconfig").read_text())
        self.assertIn("COMPONENT_DEVICE", fragment["COMPONENT_FUEL_GAUGE"]["selects"])


if __name__ == "__main__":
    unittest.main(verbosity=2)
