#!/usr/bin/env python3
"""Self-test for check_device_bus_dependency.py.

The rule exists because the same defect shipped twice: a component whose archive
references `xy_i2c_device_*` was configured without the Device framework, so the
archive could never link. The decisive cases below are the two that a build
cannot see - a mapped component whose symbol lost the `select`, and a component
that newly started calling the bus helpers without being mapped at all - plus the
two that keep the table honest: a renamed symbol and a mapping that outlived its
references.
"""

from __future__ import annotations

import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from check_device_bus_dependency import (  # noqa: E402
    REPO_ROOT,
    bus_helper_references,
    check,
    parse_selects,
)
from source_scan import read_hash_code  # noqa: E402

ROOT = """\
config COMPONENT_DEVICE
    bool "Device Framework"
    default n
    help
      Unified device management framework.

config COMPONENT_DEMO
    bool "Demo"
    default n
    select COMPONENT_DEVICE
    help
      Demo component.

config COMPONENT_OTHER
    bool "Other"
    default n
"""

USES_BUS = """\
#include "xy_device.h"

int demo_init(xy_i2c_device_t *bus)
{
    return xy_i2c_device_init(bus, 0, 0x38, 100);
}
"""

NO_BUS = """\
int demo_init(void)
{
    return 0;
}
"""


def _tree(root_text: str, components: dict[str, str]) -> Path:
    base = Path(tempfile.mkdtemp())
    (base / "Kconfig").write_text(root_text)
    for name, source in components.items():
        directory = base / "components" / name
        directory.mkdir(parents=True)
        (directory / "demo.c").write_text(source)
    return base


MAPPING = {"components/demo": ("COMPONENT_DEMO",)}


class BlockReader(unittest.TestCase):
    def test_selects_are_block_local(self) -> None:
        blocks = parse_selects(ROOT)
        self.assertEqual(["COMPONENT_DEVICE"], blocks["COMPONENT_DEMO"])
        self.assertEqual([], blocks["COMPONENT_OTHER"])

    def test_comment_mention_is_not_a_reference(self) -> None:
        base = _tree(ROOT, {"demo": "/* xy_i2c_device_read was used before */\n"})
        self.assertEqual([], bus_helper_references(base / "components" / "demo"))


class Rule(unittest.TestCase):
    def test_mapped_component_with_select_passes(self) -> None:
        failures, _ = check(_tree(ROOT, {"demo": USES_BUS}), MAPPING)
        self.assertEqual([], failures)

    def test_component_without_bus_use_is_not_tracked(self) -> None:
        failures, notes = check(_tree(ROOT, {"demo": NO_BUS}), MAPPING)
        # A mapping with no references left is stale, so it is reported - but not
        # as a missing select.
        self.assertEqual(1, len(failures))
        self.assertIn("stale", failures[0])
        self.assertIn("0 component trees", notes[0])

    def test_symbol_without_select_fails(self) -> None:
        root = ROOT.replace("    select COMPONENT_DEVICE\n", "")
        failures, _ = check(_tree(root, {"demo": USES_BUS}), MAPPING)
        self.assertTrue(any("COMPONENT_DEMO" in item and "does not select" in item
                            for item in failures), failures)

    def test_unmapped_component_using_the_bus_fails(self) -> None:
        failures, _ = check(_tree(ROOT, {"demo": USES_BUS, "extra": USES_BUS}), MAPPING)
        self.assertTrue(any("components/extra" in item and "no root Kconfig symbol" in item
                            for item in failures), failures)

    def test_renamed_symbol_fails(self) -> None:
        failures, _ = check(_tree(ROOT, {"demo": USES_BUS}),
                           {"components/demo": ("COMPONENT_GONE",)})
        self.assertTrue(any("no longer exists" in item for item in failures), failures)

    def test_mapping_for_a_missing_directory_fails(self) -> None:
        failures, _ = check(_tree(ROOT, {"demo": NO_BUS}),
                           {"components/ghost": ("COMPONENT_DEMO",)})
        self.assertTrue(any("not a directory" in item for item in failures), failures)

    def test_owner_directory_is_skipped(self) -> None:
        base = _tree(ROOT, {"device": USES_BUS})
        failures, notes = check(base, {})
        self.assertEqual([], failures)
        self.assertIn("0 component trees", notes[0])

    def test_third_party_is_skipped(self) -> None:
        failures, _ = check(_tree(ROOT, {"third_party": USES_BUS}), {})
        self.assertEqual([], failures)

    def test_each_listed_symbol_is_checked(self) -> None:
        two = ROOT + """
config COMPONENT_SECOND
    bool "Second"
    default n
"""
        failures, _ = check(_tree(two, {"demo": USES_BUS}),
                            {"components/demo": ("COMPONENT_DEMO", "COMPONENT_SECOND")})
        self.assertEqual(1, len(failures), failures)
        self.assertIn("COMPONENT_SECOND", failures[0])


class CurrentTree(unittest.TestCase):
    def test_fuel_gauge_still_selects_device(self) -> None:
        blocks = parse_selects(read_hash_code(REPO_ROOT / "Kconfig"))
        self.assertIn("COMPONENT_DEVICE", blocks.get("COMPONENT_FUEL_GAUGE", []))

    def test_sensor_switches_still_select_device(self) -> None:
        blocks = parse_selects(read_hash_code(REPO_ROOT / "Kconfig"))
        for symbol in ("COMPONENT_SENSOR", "XY_SENSOR_ENABLE"):
            self.assertIn("COMPONENT_DEVICE", blocks.get(symbol, []), symbol)

    def test_driver_families_still_select_device(self) -> None:
        blocks = parse_selects(read_hash_code(REPO_ROOT / "Kconfig"))
        for symbol in ("DRIVER_SENSOR", "DRIVER_DISPLAY", "DRIVER_STORAGE",
                       "DRIVER_POWER", "DRIVER_WIRELESS", "DRIVER_SYSTEM"):
            self.assertIn("COMPONENT_DEVICE", blocks.get(symbol, []), symbol)

    def test_real_tree_is_consistent(self) -> None:
        failures, _ = check(REPO_ROOT)
        self.assertEqual([], failures)


if __name__ == "__main__":
    unittest.main(verbosity=2)
