#!/usr/bin/env python3
"""A coverage number is only a claim if something reports it changing.

`check_hal_adc_dac_surface.py` prints `stm32f4 28/40, stm32u5 12/40, wch 40/40` on this repository,
which is also what a script that reads nothing would print if its tables were hard-coded. These cases
drive the same `findings()` against throwaway trees, so each rule has a red that was actually produced:

* a function a backend used to define and no longer does;
* a pin that still calls a function missing after somebody implemented it;
* a declaration no backend defines at all;
* a column-0 call (`return xy_hal_adc_read(...)`) that must not be read as a definition;
* a header whose extraction yields nothing;
* a pin without a reason, and a pin for a backend that does not exist.

The trees are synthetic and tiny on purpose: the question is what the checker does with a definition
that disappeared, not whether today's U5 wrapper happens to be twelve functions.
"""

from __future__ import annotations

import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import check_hal_adc_dac_surface as guard  # noqa: E402

ADC_HEADER = """\
#ifndef XY_HAL_ADC_H
#define XY_HAL_ADC_H
xy_hal_error_t xy_hal_adc_init(void *adc, const xy_hal_adc_config_t *config);
int32_t xy_hal_adc_read(void *adc, uint8_t channel, uint32_t timeout);
uint32_t xy_hal_adc_value_to_mv(void *adc, uint32_t value, uint32_t vref);
#endif
"""
DAC_HEADER = """\
#ifndef XY_HAL_DAC_H
#define XY_HAL_DAC_H
xy_hal_error_t xy_hal_dac_init(void *dac, const xy_hal_dac_config_t *config);
#endif
"""
ADC_BACKEND = """\
#include "xy_hal_adc.h"

xy_hal_error_t xy_hal_adc_init(void *adc, const xy_hal_adc_config_t *config)
{
    return XY_HAL_OK;
}

int32_t xy_hal_adc_read(void *adc, uint8_t channel, uint32_t timeout)
{
    return 0;
}

uint32_t xy_hal_adc_value_to_mv(void *adc, uint32_t value, uint32_t vref)
{
    return value;
}
"""
DAC_BACKEND = """\
#include "xy_hal_dac.h"

xy_hal_error_t xy_hal_dac_init(void *dac, const xy_hal_dac_config_t *config)
{
    return XY_HAL_OK;
}
"""


class HalAdcDacSurfaceTest(unittest.TestCase):
    def setUp(self) -> None:
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.root = Path(self.directory.name)
        self.write("components/hal/inc/xy_hal_adc.h", ADC_HEADER)
        self.write("components/hal/inc/xy_hal_dac.h", DAC_HEADER)
        for backend in guard.BACKENDS:
            self.write(f"{guard.BACKENDS[backend][0]}/xy_hal_adc.c", ADC_BACKEND)
            self.write(f"{guard.BACKENDS[backend][0]}/xy_hal_dac.c", DAC_BACKEND)
        #: Every backend implements every declaration, so the only findings come from the pins.
        self.pins: dict[tuple[str, str], tuple[frozenset[str], str]] = {
            ("stm32f4", "adc"): (frozenset(), "the whole header is implemented here"),
            ("wch", "dac"): (frozenset(), "the whole header is implemented here"),
        }

    def write(self, relative: str, text: str) -> Path:
        path = self.root / relative
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text, encoding="utf-8")
        return path

    def judged(self) -> list[str]:
        return guard.findings(root=self.root, pins=self.pins)

    def test_a_complete_tree_with_matching_pins_is_clean(self) -> None:
        self.assertEqual(self.judged(), [])

    def test_a_definition_that_disappears_is_reported(self) -> None:
        self.write(f"{guard.BACKENDS['stm32f4'][0]}/xy_hal_adc.c",
                   ADC_BACKEND.split("int32_t xy_hal_adc_read")[0])
        problems = self.judged()
        self.assertTrue(any("no longer defines" in p and "xy_hal_adc_read" in p for p in problems),
                        problems)

    def test_a_pin_that_somebody_implemented_is_reported_as_stale(self) -> None:
        self.pins[("stm32f4", "adc")] = (frozenset({"xy_hal_adc_read"}),
                                         "this backend has no read path")
        problems = self.judged()
        self.assertTrue(any("still lists" in p and "xy_hal_adc_read" in p for p in problems),
                        problems)

    def test_a_declaration_no_backend_defines_is_reported(self) -> None:
        self.write("components/hal/inc/xy_hal_adc.h",
                   ADC_HEADER.replace("#endif",
                                      "int32_t xy_hal_adc_nowhere(void *adc);\n#endif"))
        problems = self.judged()
        self.assertTrue(any("no backend defines" in p and "xy_hal_adc_nowhere" in p
                            for p in problems), problems)

    def test_a_column_zero_call_is_not_a_definition(self) -> None:
        """`return xy_hal_adc_init(...)` at column 0 begins with a keyword, not a type."""
        self.write(f"{guard.BACKENDS['wch'][0]}/xy_hal_adc_extra.c",
                   "int32_t helper(void)\n{\nreturn xy_hal_adc_only_called(0, 0);\n}\n"
                   "\nint32_t xy_hal_adc_really_defined(void)\n{\nreturn 0;\n}\n")
        defined = guard.defined_functions(self.root, (guard.BACKENDS["wch"][0],), "adc")
        self.assertNotIn("xy_hal_adc_only_called", defined)
        self.assertIn("xy_hal_adc_really_defined", defined)
        # And a definition that disappears is still caught where a pin exists.
        self.write(f"{guard.BACKENDS['stm32f4'][0]}/xy_hal_adc.c",
                   ADC_BACKEND.replace("int32_t xy_hal_adc_read", "int32_t xy_hal_adc_removed"))
        self.assertIn("xy_hal_adc_read",
                      " ".join(guard.findings(root=self.root, pins=self.pins)))

    def test_a_header_that_declares_nothing_is_reported(self) -> None:
        self.write("components/hal/inc/xy_hal_dac.h", "#ifndef XY_HAL_DAC_H\n#endif\n")
        problems = self.judged()
        self.assertTrue(any("not one declaration was extracted" in p for p in problems), problems)

    def test_a_missing_header_is_reported(self) -> None:
        (self.root / "components/hal/inc/xy_hal_dac.h").unlink()
        problems = self.judged()
        self.assertTrue(any("xy_hal_dac.h: the module header is gone" in p for p in problems),
                        problems)

    def test_a_pin_without_a_reason_is_reported(self) -> None:
        self.pins[("stm32f4", "adc")] = (frozenset(), "  ")
        problems = self.judged()
        self.assertTrue(any("pinned without a reason" in p for p in problems), problems)

    def test_a_pin_for_a_backend_that_does_not_exist_is_reported(self) -> None:
        self.pins[("stm32l4", "adc")] = (frozenset(), "this branch builds no ADC")
        problems = self.judged()
        self.assertTrue(any("does not exist" in p and "stm32l4/adc" in p for p in problems),
                        problems)

    def test_a_backend_directory_that_moved_is_reported(self) -> None:
        directory = self.root / guard.BACKENDS["wch"][0]
        for path in sorted(directory.iterdir()):
            path.unlink()
        directory.rmdir()
        problems = self.judged()
        self.assertTrue(any("moved" in p and "wch" in p for p in problems), problems)

    def test_the_real_tree_passes(self) -> None:
        """The pins describe this repository today; the number the guide quotes is this one."""
        self.assertEqual(guard.findings(), [])
        self.assertIn("stm32f4 28/40", guard.report())
        self.assertIn("stm32u5 12/40", guard.report())
        self.assertIn("wch 40/40", guard.report())


if __name__ == "__main__":
    unittest.main()
