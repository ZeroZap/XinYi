#!/usr/bin/env python3
"""Self-test for check_component_hal_source_ownership.py.

The guard is only worth having if it rejects the shape it exists to reject, so
the decisive case reads the *pre-fix* `components/device/CMakeLists.txt` back out
of git and requires it to be flagged.
"""

from __future__ import annotations

import subprocess
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from check_component_hal_source_ownership import REPO_ROOT, hal_source_references  # noqa: E402

# The exact block that produced two copies of the PC backend (device, before the fix).
LEGACY_DEVICE = """
if(HAL_PLATFORM STREQUAL "PC")
    set(HAL_SOURCES
        ../hal/PC/xy_hal_pc.c
        ../hal/PC/xy_hal_gpio_pc.c
        ../hal/PC/xy_hal_i2c_pc.c
        ../hal/PC/xy_hal_spi_pc.c
        ../hal/PC/xy_hal_uart_pc.c
    )
endif()
if(HAL_SOURCES)
    target_sources(xy_device PRIVATE ${HAL_SOURCES})
endif()
"""

FIXED_DEVICE = """
if(HAL_PLATFORM STREQUAL "PC")
    set(HAL_INCLUDE_DIRS ../hal/inc ../hal)
    set(HAL_DEFINES HAL_PLATFORM_PC XY_HAL_PC_SIM)
endif()
target_link_libraries(xy_device PUBLIC xy_hal)
"""


class HalSourceReferences(unittest.TestCase):
    def test_legacy_device_block_is_flagged(self) -> None:
        refs = hal_source_references(LEGACY_DEVICE)
        self.assertEqual(5, len(refs))
        self.assertIn("../hal/PC/xy_hal_pc.c", refs)
        self.assertIn("../hal/PC/xy_hal_uart_pc.c", refs)

    def test_linking_the_hal_library_is_not_a_finding(self) -> None:
        self.assertEqual([], hal_source_references(FIXED_DEVICE))

    def test_hal_headers_and_include_dirs_are_not_findings(self) -> None:
        text = """
        target_include_directories(xy_device PUBLIC ../hal/inc ../hal)
        #include \"xy_hal_i2c.h\"
        """
        self.assertEqual([], hal_source_references(text))

    def test_embedded_backend_paths_are_flagged(self) -> None:
        text = "set(S ${CMAKE_CURRENT_SOURCE_DIR}/../hal/STM32/stm32l4/xy_hal_gpio.c)"
        self.assertEqual(1, len(hal_source_references(text)))

    def test_target_sources_form_is_flagged(self) -> None:
        text = "target_sources(xy_device PRIVATE ../hal/PC/xy_hal_spi_pc.c)"
        self.assertEqual(["../hal/PC/xy_hal_spi_pc.c"], hal_source_references(text))


class TreeFindings(unittest.TestCase):
    def test_current_tree_has_no_findings(self) -> None:
        proc = subprocess.run(
            [sys.executable, str(Path(__file__).resolve().parent /
                                 "check_component_hal_source_ownership.py")],
            capture_output=True, text=True, cwd=str(REPO_ROOT))
        self.assertEqual(0, proc.returncode, proc.stdout + proc.stderr)
        self.assertIn("component_hal_source_ownership ok", proc.stdout)

    def test_pre_fix_device_cmakelists_is_flagged(self) -> None:
        """The commit that introduced the fix must have a parent that fails the rule."""
        log = subprocess.run(
            ["git", "log", "--format=%H", "--", "components/device/CMakeLists.txt"],
            capture_output=True, text=True, cwd=str(REPO_ROOT))
        revisions = [line.strip() for line in log.stdout.splitlines() if line.strip()]
        self.assertTrue(revisions, "no history for components/device/CMakeLists.txt")
        # Find a revision whose device CMakeLists still compiled the PC backend.
        seen = 0
        for revision in revisions:
            show = subprocess.run(
                ["git", "show", f"{revision}:components/device/CMakeLists.txt"],
                capture_output=True, text=True, cwd=str(REPO_ROOT))
            if show.returncode != 0:
                continue
            refs = hal_source_references(show.stdout)
            if refs:
                self.assertGreaterEqual(len(refs), 5)
                seen += 1
                break
        self.assertEqual(1, seen, "no historical revision compiles the PC HAL backend; "
                                  "the rule never had a real subject")


if __name__ == "__main__":
    unittest.main(verbosity=2)
