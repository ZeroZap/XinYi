#!/usr/bin/env python3
"""Regression tests for the lightweight Kconfig parser."""

import tempfile
import unittest
from pathlib import Path

from cmake.kconfig_parser import KconfigParser


class KconfigParserOutputTest(unittest.TestCase):
    def parse_text(self, text: str) -> KconfigParser:
        with tempfile.TemporaryDirectory() as tmp:
            kconfig_path = Path(tmp) / "Kconfig"
            kconfig_path.write_text(text, encoding="utf-8")
            parser = KconfigParser()
            parser.parse(str(kconfig_path))
            parser.resolve_dependencies()
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


if __name__ == "__main__":
    unittest.main()
