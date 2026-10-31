#!/usr/bin/env python3
"""A guard that has never failed is a guard whose red nobody has seen.

`check_crypto_crc_catalogue.py` passes on this repository, which is the state in which it is
indistinguishable from a script that walks nothing and prints `ok`. These cases drive the same
`findings()` against synthetic inputs, so every rule it claims to enforce has a red it produces -
including the one this slice was written for: a header entry point that no translation unit
defines.
"""

from __future__ import annotations

import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import check_crypto_crc_catalogue as guard  # noqa: E402


class CryptoCrcCatalogueTest(unittest.TestCase):
    def judge(self, declared: dict[str, list[str]], defined: set[str],
              unwritten: dict[str, str] | None = None,
              source_only: dict[str, str] | None = None) -> list[str]:
        return guard.findings(declared, defined, unwritten or {}, source_only or {})

    def test_a_declared_and_defined_variant_passes(self) -> None:
        self.assertEqual([], self.judge({"xy_crc16_modbus": ["h.h"]}, {"xy_crc16_modbus"}))

    def test_a_declared_variant_nothing_defines_is_reported(self) -> None:
        problems = self.judge({"xy_crc16_kermit": ["h.h"], "xy_crc16_modbus": ["h.h"]},
                              {"xy_crc16_modbus"})
        self.assertEqual(1, len(problems), problems)
        self.assertIn("xy_crc16_kermit", problems[0])
        self.assertIn("fails to link", problems[0])

    def test_a_pinned_unwritten_variant_passes(self) -> None:
        self.assertEqual([],
                         self.judge({"xy_crc16_kermit": ["h.h"], "xy_crc16_modbus": ["h.h"]},
                                    {"xy_crc16_modbus"},
                                    {"xy_crc16_kermit": "no definition in the tree"}))

    def test_a_pin_that_became_implemented_is_stale(self) -> None:
        # The shape that matters when the catalogue gets filled in: the entry is written and the pin
        # is left behind, which is how the next unwritten entry stops being visible.
        problems = self.judge({"xy_crc16_kermit": ["h.h"]}, {"xy_crc16_kermit"},
                              {"xy_crc16_kermit": "no definition in the tree"})
        self.assertEqual(1, len(problems), problems)
        self.assertIn("stale", problems[0])

    def test_a_pin_without_a_reason_is_reported(self) -> None:
        problems = self.judge({"xy_crc16_kermit": ["h.h"], "xy_crc16_modbus": ["h.h"]},
                              {"xy_crc16_modbus"}, {"xy_crc16_kermit": "  "})
        self.assertTrue(any("without a stated reason" in problem for problem in problems), problems)

    def test_a_pin_that_outlived_the_declaration_is_reported(self) -> None:
        problems = self.judge({"xy_crc16_modbus": ["h.h"]}, {"xy_crc16_modbus"},
                              {"xy_crc16_kermit": "no definition in the tree"})
        self.assertTrue(any("outlived" in problem for problem in problems), problems)

    def test_a_definition_no_header_declares_is_reported(self) -> None:
        # `xy_crc8_bluetooth_hid()` was exactly this: the implementation existed under a name the
        # header never published, so the declared name never linked.
        problems = self.judge({"xy_crc8_bluetooth": ["h.h"]}, {"xy_crc8_bluetooth_hid"},
                              {"xy_crc8_bluetooth": "no definition in the tree"})
        self.assertTrue(any("xy_crc8_bluetooth_hid" in problem and "declared in no header" in problem
                            for problem in problems), problems)

    def test_a_source_only_pin_passes_and_goes_stale_when_declared(self) -> None:
        self.assertEqual([], self.judge({"xy_crc64_redis": ["h.h"]},
                                        {"xy_crc64_redis", "xy_crc64_redis_cached"},
                                        source_only={"xy_crc64_redis_cached": "gated out"}))
        problems = self.judge({"xy_crc64_redis": ["h.h"], "xy_crc64_redis_cached": ["h.h"]},
                              {"xy_crc64_redis", "xy_crc64_redis_cached"},
                              source_only={"xy_crc64_redis_cached": "gated out"})
        self.assertTrue(any("stale" in problem for problem in problems), problems)

    def test_an_empty_walk_is_a_failure_not_a_clean_result(self) -> None:
        self.assertTrue(self.judge({}, {"xy_crc16_modbus"}))
        self.assertTrue(self.judge({"xy_crc16_modbus": ["h.h"]}, set()))

    def test_the_repository_catalogue_is_clean(self) -> None:
        self.assertEqual([], guard.findings())

    def test_the_repository_pins_are_not_vacuous(self) -> None:
        # A guard whose tables are empty reports the same `ok` as one that never found anything.
        self.assertGreaterEqual(len(guard.DECLARED_BUT_UNWRITTEN), 1)
        for name, reason in guard.DECLARED_BUT_UNWRITTEN.items():
            self.assertTrue(reason.strip(), name)


if __name__ == "__main__":
    unittest.main(verbosity=2)
