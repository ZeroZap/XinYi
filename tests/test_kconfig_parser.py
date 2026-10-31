#!/usr/bin/env python3
"""Regression tests for the lightweight Kconfig parser."""

import sys
import tempfile
import unittest
from pathlib import Path

# Run as a script, this file puts tests/ on the path, not the repository root, so the import below fails
# with ModuleNotFoundError. It did, for as long as nothing ran this file - see the registration in
# tests/unit/CMakeLists.txt that keeps it running.
ROOT = Path(__file__).resolve().parents[1]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from cmake.kconfig_parser import KconfigParser  # noqa: E402


class KconfigParserOutputTest(unittest.TestCase):
    def parse_text(self, text: str) -> KconfigParser:
        with tempfile.TemporaryDirectory() as tmp:
            kconfig_path = Path(tmp) / "Kconfig"
            kconfig_path.write_text(text, encoding="utf-8")
            parser = KconfigParser()
            parser.parse(str(kconfig_path))
            return parser

    def test_string_defaults_are_not_double_quoted_in_generated_outputs(self):
        parser = self.parse_text(
            """
config XY_VERSION
    string
    default "1.0.0"
"""
        )

        with tempfile.TemporaryDirectory() as tmp:
            tmp_path = Path(tmp)
            config_path = tmp_path / ".config"
            autoconf_path = tmp_path / "include" / "autoconf.h"
            cmake_path = tmp_path / "config.cmake"

            parser.generate_config(str(config_path))
            parser.generate_autoconf(str(autoconf_path))
            parser.generate_cmake(str(cmake_path))

            self.assertIn('CONFIG_XY_VERSION="1.0.0"', config_path.read_text(encoding="utf-8"))
            self.assertIn('#define CONFIG_XY_VERSION "1.0.0"', autoconf_path.read_text(encoding="utf-8"))
            self.assertIn('set(XY_XY_VERSION "1.0.0")', cmake_path.read_text(encoding="utf-8"))
            self.assertNotIn('""1.0.0""', cmake_path.read_text(encoding="utf-8"))
    def test_string_default_containing_if_substring_is_not_corrupted(self):
        parser = self.parse_text(
            """
config FEATURE_NAME
    string
    default "notify"
"""
        )

        self.assertEqual(
            parser.resolve_values(platform="PC")["FEATURE_NAME"], '"notify"'
        )

        with tempfile.TemporaryDirectory() as tmp:
            tmp_path = Path(tmp)
            config_path = tmp_path / ".config"
            autoconf_path = tmp_path / "include" / "autoconf.h"
            cmake_path = tmp_path / "config.cmake"

            parser.generate_config(str(config_path))
            parser.generate_autoconf(str(autoconf_path))
            parser.generate_cmake(str(cmake_path))

            self.assertIn('CONFIG_FEATURE_NAME="notify"', config_path.read_text(encoding="utf-8"))
            self.assertIn('#define CONFIG_FEATURE_NAME "notify"', autoconf_path.read_text(encoding="utf-8"))
            self.assertIn('set(XY_FEATURE_NAME "notify")', cmake_path.read_text(encoding="utf-8"))

    def test_conditional_defaults_follow_selected_platform(self):
        parser = self.parse_text(
            """
config PLATFORM_PC
    bool
    default y if PLATFORM_PC

config PLATFORM_STM32
    bool

config PLATFORM_STM32U5
    bool
    depends on PLATFORM_STM32
    default y if PLATFORM_STM32U5

config FS_ENABLED
    bool
    default y if PLATFORM_STM32U5

config GUI_SDL
    bool
    depends on PLATFORM_PC
    default y if PLATFORM_PC

config GUI_TFT
    bool
    depends on PLATFORM_STM32U5
    default y if PLATFORM_STM32U5
"""
        )

        pc = parser.resolve_values(platform="PC")
        self.assertEqual(pc["PLATFORM_PC"], "y")
        self.assertEqual(pc["PLATFORM_STM32U5"], "n")
        self.assertEqual(pc["FS_ENABLED"], "n")
        self.assertEqual(pc["GUI_SDL"], "y")
        self.assertEqual(pc["GUI_TFT"], "n")

        u5 = parser.resolve_values(platform="STM32U5")
        self.assertEqual(u5["PLATFORM_PC"], "n")
        self.assertEqual(u5["PLATFORM_STM32U5"], "y")
        self.assertEqual(u5["FS_ENABLED"], "y")
        self.assertEqual(u5["GUI_SDL"], "n")
        self.assertEqual(u5["GUI_TFT"], "y")

    def test_select_does_not_enable_symbols_with_unsatisfied_dependencies(self):
        parser = self.parse_text(
            """
config PLATFORM_PC
    bool
    default y if PLATFORM_PC

config SDL2
    bool

config GUI_SDL
    bool
    depends on PLATFORM_PC
    select SDL2
    default y if PLATFORM_PC

config GUI_ENABLED
    bool
    select GUI_SDL
    default y
"""
        )

        pc = parser.resolve_values(platform="PC")
        self.assertEqual(pc["GUI_SDL"], "y")
        self.assertEqual(pc["SDL2"], "y")

        u5 = parser.resolve_values(platform="STM32U5")
        self.assertEqual(u5["GUI_ENABLED"], "y")
        self.assertEqual(u5["GUI_SDL"], "n")
        self.assertEqual(u5["SDL2"], "n")

    def test_overrides_cannot_enable_symbols_with_unsatisfied_dependencies(self):
        parser = self.parse_text(
            """
config DRIVER_DISPLAY
    bool
    default n

config DRIVER_DISPLAY_LCD
    bool
    depends on DRIVER_DISPLAY
    default n

config DRIVER_DISPLAY_LCD_SPI
    bool
    depends on DRIVER_DISPLAY_LCD
    default n
"""
        )

        values = parser.resolve_values(
            platform="PC",
            overrides={"DRIVER_DISPLAY_LCD_SPI": "ON"},
        )

        self.assertEqual(values["DRIVER_DISPLAY"], "n")
        self.assertEqual(values["DRIVER_DISPLAY_LCD"], "n")
        self.assertEqual(values["DRIVER_DISPLAY_LCD_SPI"], "n")

    def test_root_kconfig_does_not_expose_unimplemented_standalone_rgb_driver(self):
        root = Path(__file__).resolve().parents[1]
        parser = KconfigParser()
        parser.parse(str(root / "Kconfig"))

        self.assertNotIn("DRIVER_DISPLAY_RGB", parser.config)
        self.assertIn("DRIVER_DISPLAY_LED_SERIAL_RGB", parser.config)

    def test_if_block_condition_gates_enclosed_symbols(self):
        """An `if` block is a dependency on everything it encloses.

        The parser matched `config NAME` anywhere in the file, so an `if` block was
        decoration: a symbol inside it could be switched on while its guard was off.
        """
        parser = self.parse_text(
            """
config PARENT
    bool
    default n

if PARENT

config CHILD
    bool "child"
    default n

endif
"""
        )

        self.assertEqual(parser.config["CHILD"]["depends_on"], ["PARENT"])
        self.assertEqual(
            parser.resolve_values(platform="PC", overrides={"CHILD": "ON"})["CHILD"], "n"
        )
        self.assertEqual(
            parser.resolve_values(
                platform="PC", overrides={"PARENT": "ON", "CHILD": "ON"}
            )["CHILD"],
            "y",
        )

    def test_nested_if_blocks_accumulate_outer_conditions(self):
        parser = self.parse_text(
            """
config OUTER
    bool
    default y

config INNER
    bool
    default n

if OUTER
if INNER

config LEAF
    bool "leaf"
    default n

endif
endif
"""
        )

        self.assertEqual(parser.config["LEAF"]["depends_on"], ["OUTER", "INNER"])
        self.assertEqual(
            parser.resolve_values(platform="PC", overrides={"LEAF": "ON"})["LEAF"], "n"
        )

    def test_unclosed_if_block_is_rejected(self):
        with self.assertRaisesRegex(ValueError, "never closed by 'endif'"):
            self.parse_text(
                """
config PARENT
    bool
    default y

if PARENT

config CHILD
    bool
    default n
"""
            )

    def test_stray_endif_is_rejected(self):
        with self.assertRaisesRegex(ValueError, "without a matching 'if'"):
            self.parse_text(
                """
config PARENT
    bool
    default y

endif
"""
            )

    def test_source_directive_is_rejected_instead_of_silently_ignored(self):
        """`source` used to configure nothing and say nothing."""
        with self.assertRaisesRegex(ValueError, "'source' is not supported"):
            self.parse_text(
                """
source "components/sensor/Kconfig"

config PARENT
    bool
    default y
"""
            )

    def test_root_kconfig_if_block_gates_display_children(self):
        root = Path(__file__).resolve().parents[1]
        parser = KconfigParser()
        parser.parse(str(root / "Kconfig"))

        self.assertIn("DRIVER_DISPLAY", parser.config["DRIVER_DISPLAY_LCD"]["depends_on"])

        rejected = parser.resolve_values(
            platform="PC",
            overrides={"DRIVER_DISPLAY": "OFF", "DRIVER_DISPLAY_LCD": "ON"},
        )
        self.assertEqual(rejected["DRIVER_DISPLAY"], "n")
        self.assertEqual(rejected["DRIVER_DISPLAY_LCD"], "n")

        accepted = parser.resolve_values(
            platform="PC",
            overrides={"DRIVER_DISPLAY": "ON", "DRIVER_DISPLAY_LCD": "ON"},
        )
        self.assertEqual(accepted["DRIVER_DISPLAY"], "y")
        self.assertEqual(accepted["DRIVER_DISPLAY_LCD"], "y")

    def test_duplicate_blocks_merge_defaults_dependencies_selects_and_help(self):
        parser = self.parse_text(
            """
config BASE
    bool
    default y

config EXTRA
    bool
    default y

config DUP
    bool "first"
    depends on BASE
    select SELECT_A
    default n
    help
      first help

config DUP
    bool "second"
    depends on EXTRA
    select SELECT_B
    default y if EXTRA
    help
      second help

config SELECT_A
    bool

config SELECT_B
    bool
"""
        )

        self.assertEqual(parser.config["DUP"]["type"], "bool")
        self.assertEqual(len(parser.config["DUP"]["defaults"]), 2)
        self.assertEqual(parser.config["DUP"]["depends_on"], ["BASE", "EXTRA"])
        self.assertEqual(parser.config["DUP"]["selects"], ["SELECT_A", "SELECT_B"])
        self.assertIn("first help", parser.config["DUP"]["help"])
        self.assertIn("second help", parser.config["DUP"]["help"])
        self.assertEqual(parser.resolve_values(platform="PC")["DUP"], "n")

    def test_duplicate_block_type_conflict_is_rejected(self):
        with self.assertRaisesRegex(ValueError, "Conflicting Kconfig types for DUP"):
            self.parse_text(
                """
config DUP
    bool
    default y

config DUP
    string
    default "later"
"""
            )

    def test_merged_conditional_defaults_generate_for_pc_and_stm32u5(self):
        parser = self.parse_text(
            """
config PLATFORM_PC
    bool
    default n

config PLATFORM_STM32
    bool
    default n

config PLATFORM_STM32U5
    bool
    depends on PLATFORM_STM32
    default n

config TARGET
    bool
    default y if PLATFORM_PC

config TARGET
    bool
    default y if PLATFORM_STM32U5
"""
        )

        self.assertEqual(parser.resolve_values(platform="PC")["TARGET"], "y")
        self.assertEqual(parser.resolve_values(platform="STM32U5")["TARGET"], "y")


if __name__ == "__main__":
    unittest.main()
