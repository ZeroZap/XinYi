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
import check_hal_i2c_contract as guard  # noqa: E402


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


class RegisterAddressTests(unittest.TestCase):
    """One uint16_t parameter, three behaviours: truncated, derived, refused.

    L4 and U5 hardcoded I2C_MEMADD_SIZE_8BIT, so a register above 0xFF reached the wrong
    register with no error at all. F4 derived the encoding from the value. WCH refused the wide
    address, which is at least visible but splits one public parameter into three meanings.
    """

    L4_HARDCODED = """
xy_hal_error_t xy_hal_i2c_mem_write(void *i2c, uint16_t dev_addr, uint16_t reg_addr,
                                    const uint8_t *data, size_t len, uint32_t timeout)
{
    return map_status(HAL_I2C_Mem_Write(handle, (uint16_t)(dev_addr << 1), reg_addr,
                                        I2C_MEMADD_SIZE_8BIT, (uint8_t *)data, (uint16_t)len,
                                        timeout));
}
"""

    WCH_REFUSES = """
xy_hal_error_t xy_hal_i2c_mem_read(void* instance, uint16_t address, uint16_t reg,
                                   uint8_t* data, size_t len, uint32_t timeout) {
    if (i2c_address_valid(address) || reg > 0xFFU || len == 0U)
        return XY_HAL_ERROR_INVALID_PARAM;
    I2C_SendData(instance, (uint8_t)reg);
    return XY_HAL_OK;
}
"""

    WCH_WIDE = """
xy_hal_error_t xy_hal_i2c_mem_read(void* instance, uint16_t address, uint16_t reg,
                                   uint8_t* data, size_t len, uint32_t timeout) {
    if (reg > 0xFFU) {
        I2C_SendData(instance, (uint8_t)(reg >> 8));
    }
    I2C_SendData(instance, (uint8_t)(reg & 0xFFU));
    return XY_HAL_OK;
}
"""

    F4_DERIVED = """
xy_hal_error_t xy_hal_i2c_mem_write(void *i2c, uint16_t dev_addr, uint16_t reg_addr,
                                    const uint8_t *data, size_t len, uint32_t timeout)
{
    return map_status(HAL_I2C_Mem_Write(handle, (uint16_t)(dev_addr << 1), reg_addr,
                                        i2c_mem_address_size(reg_addr), (uint8_t *)data,
                                        (uint16_t)len, timeout));
}
"""

    def test_a_hardcoded_eight_bit_size_is_reported_as_truncation(self):
        problems = guard.register_address_findings(self.L4_HARDCODED, "l4")
        self.assertTrue(any("truncated to its low byte" in problem for problem in problems))

    def test_refusing_the_wide_address_is_reported_as_refusal(self):
        problems = guard.register_address_findings(self.WCH_REFUSES, "wch")
        self.assertTrue(any("refuses register addresses above 0xFF" in problem
                            for problem in problems))

    def test_branching_on_the_wide_address_is_not_a_refusal(self):
        """Regression case: `if (reg > 0xFFU) {` is the fix, not the defect.

        The first version of the rule matched the condition alone and reported the corrected
        WCH backend as still refusing the address.
        """
        self.assertEqual(guard.register_address_findings(self.WCH_WIDE, "wch"), [])

    def test_a_size_derived_from_the_value_passes(self):
        self.assertEqual(guard.register_address_findings(self.F4_DERIVED, "f4"), [])


class TimeoutTests(unittest.TestCase):
    """`timeout` means the budget for the whole call on every backend.

    WCH ran five to nine event waits per transfer, each with the caller's full timeout, plus one
    per data byte: a 10 ms request could block for a quarter of a second. STM32 spends the same
    number inside a single HAL call.
    """

    PER_EVENT = """
xy_hal_error_t xy_hal_i2c_master_transmit(void* instance, uint16_t address, const uint8_t* data,
                                          size_t len, uint32_t timeout) {
    result = i2c_wait_event(instance, I2C_EVENT_MASTER_MODE_SELECT, timeout);
    if (result != XY_HAL_OK)
        return result;
    return XY_HAL_OK;
}
"""

    ONE_DEADLINE = """
xy_hal_error_t xy_hal_i2c_master_transmit(void* instance, uint16_t address, const uint8_t* data,
                                          size_t len, uint32_t timeout) {
    uint32_t transfer_start = i2c_tick_ms();
    result = i2c_wait_event_until(instance, I2C_EVENT_MASTER_MODE_SELECT, transfer_start, timeout);
    if (result != XY_HAL_OK)
        return result;
    return XY_HAL_OK;
}

static xy_hal_error_t i2c_wait_event_until(void* instance, uint32_t event, uint32_t start,
                                           uint32_t timeout) {
    if (i2c_budget_exhausted(start, timeout))
        return XY_HAL_ERROR_TIMEOUT;
    return i2c_wait_event(instance, event, i2c_budget_remaining(start, timeout));
}
"""

    READY_WITH_TRIALS = """
xy_hal_error_t xy_hal_i2c_is_device_ready(void *i2c, uint16_t dev_addr, uint32_t trials,
                                          uint32_t timeout)
{
    return map_status(HAL_I2C_IsDeviceReady((I2C_HandleTypeDef *)i2c,
                                            (uint16_t)(dev_addr << 1), trials, timeout));
}
"""

    READY_WITH_BUDGET = READY_WITH_TRIALS.replace(
        "trials, timeout));", "trials, per_trial));")

    def test_a_per_event_wait_is_reported(self):
        problems = guard.timeout_findings(self.PER_EVENT, "wch")
        self.assertTrue(any("again on each event" in problem for problem in problems))

    def test_a_helper_fed_by_a_start_time_passes(self):
        """The timeout is still an argument; what matters is the budget travelling with it."""
        self.assertEqual(guard.timeout_findings(self.ONE_DEADLINE, "wch"), [])

    def test_a_budget_without_a_timeout_report_is_reported(self):
        broken = self.ONE_DEADLINE.replace("return XY_HAL_ERROR_TIMEOUT;", "return XY_HAL_OK;")
        problems = guard.timeout_findings(broken, "wch")
        self.assertTrue(any("never reports TIMEOUT" in problem for problem in problems))

    def test_handing_trials_and_the_timeout_to_hal_is_reported(self):
        problems = guard.timeout_findings(self.READY_WITH_TRIALS, "stm32l4")
        self.assertTrue(any("trials times" in problem for problem in problems))

    def test_splitting_the_budget_across_trials_passes(self):
        self.assertEqual(guard.timeout_findings(self.READY_WITH_BUDGET, "stm32l4"), [])


class HeaderTests(unittest.TestCase):
    """One fixture carrying every header requirement, so a case isolates the rule it names.

    Adding the timeout and trials requirements broke four of these cases at once: the fixtures
    were minimal strings, so each new rule fired on all of them and the exact-count assertions
    started counting the wrong thing.
    """

    COMPLETE = (
        " * @param dev_addr Device address, 7-bit and unshifted\n"
        " * @param reg_addr Register address, 8- or 16-bit\n"
        " * @param timeout Budget for the whole call in milliseconds\n"
        " * @param trials Number of trials; N trials share the timeout\n"
        "  XY_HAL_I2C_ADDR_10BIT, /**< declared, init returns NOT_SUPPORTED */\n"
    )

    def test_the_complete_fixture_passes(self):
        self.assertEqual(guard.header_findings(self.COMPLETE), [])

    def test_a_header_that_omits_the_7_bit_address_is_reported(self):
        header = self.COMPLETE.replace("7-bit", "unspecified")
        self.assertTrue(any("7-bit address" in problem
                            for problem in guard.header_findings(header)))

    def test_declaring_10_bit_without_saying_it_is_refused_is_reported(self):
        header = self.COMPLETE.replace("init returns NOT_SUPPORTED", "10-bit addressing")
        problems = guard.header_findings(header)
        self.assertTrue(any("10BIT is declared without saying" in problem for problem in problems))

    def test_omitting_the_timeout_contract_is_reported(self):
        header = self.COMPLETE.replace("Budget for the whole call", "Timeout")
        problems = guard.header_findings(header)
        self.assertTrue(any("whole call" in problem for problem in problems))

    def test_omitting_the_trials_contract_is_reported(self):
        header = self.COMPLETE.replace("N trials share the timeout", "attempts")
        problems = guard.header_findings(header)
        self.assertTrue(any("shared between them" in problem for problem in problems))

    def test_omitting_the_register_width_is_reported(self):
        header = self.COMPLETE.replace("8- or 16-bit", "register")
        problems = guard.header_findings(header)
        self.assertTrue(any("16-bit" in problem for problem in problems))


class RepositoryTests(unittest.TestCase):
    def test_every_backend_agrees(self):
        self.assertEqual(guard.findings(), [])


if __name__ == "__main__":
    unittest.main(verbosity=1)
