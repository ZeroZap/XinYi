#!/usr/bin/env python3
"""Cases for the stub-returns guard."""

import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import check_hal_stub_returns as guard  # noqa: E402

SILENT_SUCCESS = """
xy_hal_error_t xy_hal_spi_set_cs(void *spi, uint8_t level)
{
    XY_UNUSED(spi);
    XY_UNUSED(level);
    return XY_HAL_OK;
}
"""

REFUSES = """
xy_hal_error_t xy_hal_spi_set_cs(void *spi, uint8_t level)
{
    XY_UNUSED(spi);
    XY_UNUSED(level);
    return XY_HAL_ERROR_NOT_SUPPORTED;
}
"""

REAL_WORK = """
xy_hal_error_t xy_hal_exti_map_gpio(uint8_t port, uint8_t pin, uint32_t line)
{
    if (!valid_line(line)) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    GPIO_EXTILineConfig(port, pin);
    return XY_HAL_OK;
}
"""

STUB_BODY = "{ XY_UNUSED(a); return XY_HAL_OK; }"


def body_of(source):
    start = source.index("xy_hal_")
    return guard.function_body(source, start)


class ShapeTests(unittest.TestCase):
    def test_a_silent_success_is_a_stub(self):
        self.assertTrue(guard.is_success_stub(body_of(SILENT_SUCCESS)))

    def test_refusing_with_not_supported_is_not_a_stub(self):
        self.assertFalse(guard.is_success_stub(body_of(REFUSES)))

    def test_real_work_is_not_a_stub(self):
        self.assertFalse(guard.is_success_stub(body_of(REAL_WORK)))


class RegistryTests(unittest.TestCase):
    PATH = "components/hal/x/xy_hal_thing.c"
    FUNCTION = "xy_hal_thing"

    def stub(self, path=None):
        return [(path or self.PATH, self.FUNCTION, STUB_BODY)]

    def test_an_undeclared_stub_is_reported(self):
        problems = guard.findings(self.stub(), {})
        self.assertTrue(any("nothing says why" in problem for problem in problems))

    def test_a_declared_stub_with_a_reason_passes(self):
        registry = {(self.PATH, self.FUNCTION): "the part has no calibration procedure"}
        self.assertEqual(guard.findings(self.stub(), registry), [])

    def test_an_empty_reason_is_reported(self):
        registry = {(self.PATH, self.FUNCTION): "   "}
        self.assertTrue(any("justification is empty" in problem
                            for problem in guard.findings(self.stub(), registry)))

    def test_a_stale_declaration_is_reported(self):
        """The function was fixed, so the reason no longer describes anything."""
        registry = {(self.PATH, self.FUNCTION): "no hardware here"}
        self.assertTrue(any("no longer is one" in problem
                            for problem in guard.findings([], registry)))

    def test_the_same_function_name_elsewhere_is_not_covered(self):
        registry = {(self.PATH, self.FUNCTION): "no hardware here"}
        other = [("components/hal/y/xy_hal_thing.c", self.FUNCTION, STUB_BODY)]
        self.assertTrue(any("components/hal/y" in problem
                            for problem in guard.findings(other, registry)))


class RepositoryTests(unittest.TestCase):
    def test_the_tree_has_no_undeclared_success_stubs(self):
        self.assertEqual(guard.findings(), [])


if __name__ == "__main__":
    unittest.main(verbosity=1)
