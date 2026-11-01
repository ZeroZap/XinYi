#!/usr/bin/env python3
"""Self-test for check_component_source_ownership.py.

The rule only earns its place if it rejects the shape it exists to reject, so the
decisive case pulls the *pre-fix* `components/sensor/CMakeLists.txt` back out of
git and requires all ten cross-owner sources to be reported.
"""

from __future__ import annotations

import subprocess
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from check_component_source_ownership import (  # noqa: E402
    REPO_ROOT,
    cross_component_sources,
)

SENSOR_DIR = REPO_ROOT / "components" / "sensor"

# The exact block that gave components/sensor a second copy of ten drivers.
LEGACY_SENSOR = """
# Add driver sources from sensors/ directory
file(GLOB SENSOR_DRIVERS "${CMAKE_CURRENT_LIST_DIR}/sensors/sensor_*.c")
list(APPEND SENSOR_DRIVERS
    "${CMAKE_CURRENT_LIST_DIR}/../drivers/sensor/temperature/sht30/xy_sht30.c"
    "${CMAKE_CURRENT_LIST_DIR}/../drivers/sensor/adc/ads1115/xy_ads1115.c"
    "${CMAKE_CURRENT_LIST_DIR}/../drivers/sensor/motion/mpu6050/xy_mpu6050.c"
    "${CMAKE_CURRENT_LIST_DIR}/../drivers/sensor/pressure/bmp280/xy_bmp280.c"
    "${CMAKE_CURRENT_LIST_DIR}/../drivers/sensor/light/bh1750/xy_bh1750.c"
    "${CMAKE_CURRENT_LIST_DIR}/../drivers/sensor/temperature/aht20/xy_aht20.c"
    "${CMAKE_CURRENT_LIST_DIR}/../drivers/sensor/temperature/aht10/xy_aht10.c"
    "${CMAKE_CURRENT_LIST_DIR}/../drivers/sensor/light/ap3216c/xy_ap3216c.c"
    "${CMAKE_CURRENT_LIST_DIR}/../drivers/sensor/motion/icm20608/xy_icm20608.c"
    "${CMAKE_CURRENT_LIST_DIR}/../drivers/sensor/motion/qma6100p/xy_qma6100p.c"
)
list(APPEND SENSOR_SOURCES ${SENSOR_DRIVERS})
"""

FIXED_SENSOR = """
file(GLOB SENSOR_DRIVERS "${CMAKE_CURRENT_LIST_DIR}/sensors/sensor_*.c")
list(APPEND SENSOR_SOURCES ${SENSOR_DRIVERS})
target_link_libraries(sensor_component PUBLIC hardware_abstraction xy_device
    xy_sensor_drivers_core)
"""


class CrossComponentSources(unittest.TestCase):
    def test_legacy_sensor_block_is_flagged(self) -> None:
        hits = cross_component_sources(LEGACY_SENSOR, SENSOR_DIR)
        self.assertEqual(10, len(hits))
        refs = [ref for ref, _owner in hits]
        self.assertIn(
            "${CMAKE_CURRENT_LIST_DIR}/../drivers/sensor/temperature/sht30/xy_sht30.c",
            refs)
        self.assertEqual({"drivers"}, {owner for _ref, owner in hits})

    def test_linking_the_owner_library_is_not_a_finding(self) -> None:
        self.assertEqual([], cross_component_sources(FIXED_SENSOR, SENSOR_DIR))

    def test_own_sources_are_not_findings(self) -> None:
        text = 'add_library(sensor_component STATIC sensor_bus.c sensor_core.c)'
        self.assertEqual([], cross_component_sources(text, SENSOR_DIR))

    def test_absolute_source_dir_form_is_flagged(self) -> None:
        text = ("set(S ${CMAKE_SOURCE_DIR}/components/drivers/sensor/light/"
                "bh1750/xy_bh1750.c)")
        hits = cross_component_sources(text, SENSOR_DIR)
        self.assertEqual(1, len(hits))
        self.assertEqual("drivers", hits[0][1])

    def test_sibling_headers_and_include_dirs_are_not_findings(self) -> None:
        text = ('target_include_directories(sensor_component PRIVATE\n'
                '    ${CMAKE_CURRENT_LIST_DIR}/../drivers/sensor/light/bh1750)\n'
                '#include "xy_bh1750.h"\n')
        self.assertEqual([], cross_component_sources(text, SENSOR_DIR))

    def test_comments_are_ignored(self) -> None:
        text = ('# used to compile ../drivers/sensor/light/bh1750/xy_bh1750.c\n'
                'target_link_libraries(sensor_component PUBLIC xy_sensor_drivers_core)\n')
        self.assertEqual([], cross_component_sources(text, SENSOR_DIR))

    def test_vendor_components_are_skipped(self) -> None:
        text = 'set(S ${CMAKE_SOURCE_DIR}/components/third_party/foo/bar.c)'
        self.assertEqual([], cross_component_sources(text, SENSOR_DIR))

    def test_unresolvable_variables_are_not_guessed(self) -> None:
        text = 'set(S ${SOMETHING_ELSE}/drivers/sensor/light/bh1750/xy_bh1750.c)'
        self.assertEqual([], cross_component_sources(text, SENSOR_DIR))


class TreeFindings(unittest.TestCase):
    def test_current_tree_has_no_findings(self) -> None:
        proc = subprocess.run(
            [sys.executable, str(Path(__file__).resolve().parent /
                                 "check_component_source_ownership.py")],
            capture_output=True, text=True, cwd=str(REPO_ROOT))
        self.assertEqual(0, proc.returncode, proc.stdout + proc.stderr)
        self.assertIn("component_source_ownership ok", proc.stdout)

    def test_pre_fix_sensor_cmakelists_is_flagged(self) -> None:
        """Some historical revision of the file must trip the rule."""
        if subprocess.run(["git", "rev-parse", "--git-dir"], cwd=str(REPO_ROOT),
                          capture_output=True, text=True).returncode != 0:
            # `canonical_host_clean_checkout` runs this suite from a `git archive`
            # export, which has no history to read. The evidence still runs in the
            # repository, where the revisions exist.
            self.skipTest("no git metadata in this tree (clean source export)")
        log = subprocess.run(
            ["git", "log", "--format=%H", "--", "components/sensor/CMakeLists.txt"],
            capture_output=True, text=True, cwd=str(REPO_ROOT))
        revisions = [line.strip() for line in log.stdout.splitlines() if line.strip()]
        self.assertTrue(revisions, "no history for components/sensor/CMakeLists.txt")
        flagged = 0
        for revision in revisions:
            show = subprocess.run(
                ["git", "show", f"{revision}:components/sensor/CMakeLists.txt"],
                capture_output=True, text=True, cwd=str(REPO_ROOT))
            if show.returncode != 0:
                continue
            hits = cross_component_sources(show.stdout, SENSOR_DIR)
            if hits:
                self.assertGreaterEqual(len(hits), 10)
                flagged += 1
                break
        self.assertEqual(1, flagged, "no historical revision compiles another "
                                     "component's sources; the rule never had a "
                                     "real subject")


if __name__ == "__main__":
    unittest.main(verbosity=2)
