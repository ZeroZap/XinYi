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


if __name__ == "__main__":
    unittest.main()
