#!/usr/bin/env python3
"""Cases for the I2C device-address guard.

The defect it was written for is one backend forgetting the left shift, which is invisible as
long as the other two shift: the contract, the header and a majority of the backends all agree
with each other while one addresses the wrong device. So the cases cover both smells - the bare
address reaching the vendor, a shifted one arriving safely, and the two helper spellings that
the WCH backend uses instead of an inline expression.

Two of the cases are regression cases. The first version of the guard could not see
`i2c_sdk_address(` or `i2c_address_valid(` at all, because a `\\b` before a snake_case suffix
matches nothing after an underscore - it reported the correct WCH backend thirteen times.
"""

import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import check_hal_i2c_address_convention as guard  # noqa: E402


STM32_SHIFTED = """
xy_hal_error_t xy_hal_i2c_master_transmit(void *i2c, uint16_t dev_addr,
                                          const uint8_t *data, size_t len, uint32_t timeout)
{
    if (i2c == NULL || data == NULL || len == 0U || dev_addr > 0x7FU) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    return map_status(HAL_I2C_Master_Transmit((I2C_HandleTypeDef *)i2c,
                                              (uint16_t)(dev_addr << 1), (uint8_t *)data,
                                              (uint16_t)len, timeout));
}
"""

STM32_UNSHIFTED = """
xy_hal_error_t xy_hal_i2c_master_transmit(void *i2c, uint16_t dev_addr,
                                          const uint8_t *data, size_t len, uint32_t timeout)
{
    if (i2c == NULL || data == NULL || len == 0U || dev_addr > 0x7FU) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    HAL_StatusTypeDef status = HAL_I2C_Master_Transmit(
        (I2C_HandleTypeDef *)i2c, dev_addr, (uint8_t *)data, (uint16_t)len, timeout);
    if (status != HAL_OK) {
        return XY_HAL_ERROR_IO;
    }
    return XY_HAL_OK;
}
"""

WCH_HELPER = """
xy_hal_error_t xy_hal_i2c_mem_read(void* instance, uint16_t address, uint16_t reg,
                                   uint8_t* data, size_t len, uint32_t timeout) {
    if (i2c_index(instance) < 0 || !i2c_address_valid(address) || reg > 0xFFU || len == 0U)
        return XY_HAL_ERROR_INVALID_PARAM;
    I2C_Send7bitAddress(instance, i2c_sdk_address(address), I2C_Direction_Transmitter);
    return XY_HAL_OK;
}
"""

WCH_HELPER_UNSHIFTED = WCH_HELPER.replace("i2c_sdk_address(address)", "address")


class BodyTests(unittest.TestCase):
    def check(self, body, where="backend"):
        return guard.body_findings(body, where)

    def test_a_shifted_address_passes(self):
        self.assertEqual(self.check(STM32_SHIFTED), [])

    def test_the_bare_address_reaching_the_vendor_is_reported(self):
        problems = self.check(STM32_UNSHIFTED)
        self.assertTrue(any("half the value" in problem for problem in problems))

    def test_the_wch_helper_spelling_is_recognised(self):
        self.assertEqual(self.check(WCH_HELPER), [])

    def test_the_wch_helper_spelling_without_a_shift_is_reported(self):
        self.assertTrue(any("half the value" in problem
                            for problem in self.check(WCH_HELPER_UNSHIFTED)))

    def test_a_missing_range_check_is_reported(self):
        body = STM32_SHIFTED.replace(" || dev_addr > 0x7FU", "")
        self.assertTrue(any("out-of-range" in problem for problem in self.check(body)))

    def test_a_missing_function_is_reported(self):
        self.assertEqual(len(self.check("")), 1)


class HelperTests(unittest.TestCase):
    def test_an_address_helper_that_restricts_the_range_passes(self):
        source = "static int i2c_address_valid(uint16_t address) {\n    return address <= 0x7FU;\n}\n"
        self.assertEqual(guard.helper_findings(source, "wch"), [])

    def test_an_address_helper_that_accepts_everything_is_reported(self):
        source = "static int i2c_address_valid(uint16_t address) {\n    return 1;\n}\n"
        self.assertTrue(guard.helper_findings(source, "wch"))


class TenBitTests(unittest.TestCase):
    """10-bit addressing is declared in the header and implemented nowhere.

    The failure is not that 10-bit is missing - it is that init approved the mode and set the
    peripheral up for it while every transfer rejected 10-bit addresses. Fail closed at init.
    """

    INIT_REFUSES = """
xy_hal_error_t xy_hal_i2c_init(void *i2c, const xy_hal_i2c_config_t *config)
{
    if (i2c == NULL || config == NULL) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    if (config->addr_mode == XY_HAL_I2C_ADDR_10BIT) {
        return XY_HAL_ERROR_NOT_SUPPORTED;
    }
    return XY_HAL_OK;
}
"""

    INIT_ACCEPTS = """
xy_hal_error_t xy_hal_i2c_init(void *i2c, const xy_hal_i2c_config_t *config)
{
    if (i2c == NULL || config == NULL) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    handle->Init.AddressingMode = config->addr_mode == XY_HAL_I2C_ADDR_10BIT
                                      ? I2C_ADDRESSINGMODE_10BIT
                                      : I2C_ADDRESSINGMODE_7BIT;
    return XY_HAL_OK;
}
"""

    def test_init_that_refuses_10_bit_passes(self):
        self.assertEqual(guard.ten_bit_findings(self.INIT_REFUSES, "backend"), [])

    def test_init_that_accepts_10_bit_is_reported_twice(self):
        problems = guard.ten_bit_findings(self.INIT_ACCEPTS, "backend")
        self.assertEqual(len(problems), 2)
        self.assertTrue(any("NOT_SUPPORTED" in problem for problem in problems))
        self.assertTrue(any("programs the peripheral" in problem for problem in problems))


class HeaderTests(unittest.TestCase):
    def test_a_header_that_names_the_7_bit_address_passes(self):
        self.assertEqual(guard.header_findings(" * @param dev_addr 7-bit address\n"), [])

    def test_a_silent_header_is_reported(self):
        self.assertEqual(len(guard.header_findings(" * @param dev_addr Device address\n")), 1)

    def test_declaring_10_bit_without_saying_it_is_refused_is_reported(self):
        header = " * @param dev_addr 7-bit address\n  XY_HAL_I2C_ADDR_10BIT,\n"
        problems = guard.header_findings(header)
        self.assertEqual(len(problems), 1)
        self.assertIn("10BIT is declared without saying", problems[0])

    def test_declaring_10_bit_with_the_refusal_documented_passes(self):
        header = (" * @param dev_addr 7-bit address\n  XY_HAL_I2C_ADDR_10BIT,\n"
                  " *   NOT_SUPPORTED from init\n")
        self.assertEqual(guard.header_findings(header), [])


class RepositoryTests(unittest.TestCase):
    def test_every_backend_agrees(self):
        self.assertEqual(guard.findings(), [])


if __name__ == "__main__":
    unittest.main(verbosity=1)
