#!/usr/bin/env python3
"""Cases for the SPI byte-versus-frame guard."""

import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import check_hal_spi_contract as guard  # noqa: E402


HAL_BACKED_CORRECT = """
static xy_hal_error_t spi_frame_count(const void *spi, size_t len, uint16_t *frames)
{
    const SPI_HandleTypeDef *handle = (const SPI_HandleTypeDef *)spi;

    if (len == 0U) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    if (handle->Init.DataSize == SPI_DATASIZE_16BIT) {
        if ((len & 1U) != 0U) {
            return XY_HAL_ERROR_INVALID_PARAM;
        }
        len >>= 1U;
    }
    if (len > UINT16_MAX) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    *frames = (uint16_t)len;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_spi_transmit(void *spi, const uint8_t *data, size_t len, uint32_t timeout)
{
    uint16_t frames = 0U;

    if (spi == NULL || data == NULL) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    if (spi_frame_count(spi, len, &frames) != XY_HAL_OK) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    return map_status(HAL_SPI_Transmit((SPI_HandleTypeDef *)spi, (uint8_t *)data, frames, timeout));
}
"""

HAL_BACKED_TRUNCATED = HAL_BACKED_CORRECT.replace("frames, timeout))", "(uint16_t)len, timeout))")

DELEGATING_BACKEND = """
static xy_hal_error_t spi_validate(xy_hal_spi_context_t *spi, const uint8_t *data, size_t len,
                                   uint16_t *frames)
{
    if (data == NULL || len == 0U) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    if (len > UINT16_MAX) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    *frames = (uint16_t)len;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_spi_transmit(void *spi, const uint8_t *data, size_t len, uint32_t timeout)
{
    uint16_t frames = 0U;
    if (spi_validate(spi, data, len, &frames) != XY_HAL_OK) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    return XY_HAL_OK;
}
"""

BYTE_BACKEND = """
xy_hal_error_t xy_hal_spi_transmit(void* instance, const uint8_t* data, size_t len, uint32_t timeout)
{
    size_t i;
    if (instance == NULL || data == NULL || len == 0U)
        return XY_HAL_ERROR_INVALID_PARAM;
    for (i = 0U; i < len; ++i) {
        SPI_I2S_SendData(instance, data[i]);
    }
    return XY_HAL_OK;
}
"""

BYTE_BACKEND_16BIT = BYTE_BACKEND.replace(
    "return XY_HAL_OK;\n}", "return XY_HAL_OK;\n}").replace(
    "if (instance == NULL",
    "init.SPI_DataSize = SPI_DataSize_16b;\n    if (instance == NULL")


class ByteCountTests(unittest.TestCase):
    def test_a_converted_frame_count_passes(self):
        self.assertEqual(guard.byte_count_findings(HAL_BACKED_CORRECT, "stm32l4"), [])

    def test_the_raw_byte_count_reaching_hal_is_reported(self):
        problems = guard.byte_count_findings(HAL_BACKED_TRUNCATED, "stm32l4")
        self.assertTrue(any("twice the buffer" in problem for problem in problems))


class DelegationTests(unittest.TestCase):
    def test_a_delegating_entry_point_is_followed_one_hop(self):
        definitions = guard.local_definitions(DELEGATING_BACKEND)
        body = guard.function_body(DELEGATING_BACKEND, "xy_hal_spi_transmit(")
        resolved = guard.with_delegation(body, definitions)
        self.assertIn("len == 0U", resolved)
        # hal_backed=False: this fragment has no HAL call, so the frame rules do not apply to it.
        self.assertEqual(guard.validation_findings(resolved, "f4", hal_backed=False), [])

    def test_without_delegation_the_same_body_looks_unchecked(self):
        """What the guard would say if it only read the entry point's own lines."""
        body = guard.function_body(DELEGATING_BACKEND, "xy_hal_spi_transmit(")
        self.assertTrue(guard.validation_findings(body, "f4", hal_backed=False))


class ZeroLengthTests(unittest.TestCase):
    def test_a_zero_length_is_refused(self):
        self.assertEqual(guard.validation_findings(BYTE_BACKEND, "wch", hal_backed=False), [])

    def test_a_missing_zero_length_check_is_reported(self):
        body = BYTE_BACKEND.replace(" || len == 0U", "")
        self.assertTrue(guard.validation_findings(body, "wch", hal_backed=False))


class SixteenBitTests(unittest.TestCase):
    def test_a_byte_backend_that_programs_16_bit_is_reported(self):
        problems = guard.sixteen_bit_findings(BYTE_BACKEND_16BIT, "wch")
        self.assertTrue(any("its own 16-bit frame" in problem for problem in problems))

    def test_a_backend_that_refuses_the_mode_passes(self):
        """WCH refuses 16-bit at init rather than sending bytes as frames."""
        refusing = (
            "    if (config->datasize == XY_HAL_SPI_DATASIZE_16BIT)\n"
            "        return XY_HAL_ERROR_NOT_SUPPORTED;\n"
            "    init.SPI_DataSize = SPI_DataSize_8b;\n" + BYTE_BACKEND)
        self.assertEqual(guard.sixteen_bit_findings(refusing, "wch"), [])

    def test_a_backend_that_ignores_the_question_is_reported(self):
        problems = guard.sixteen_bit_findings(BYTE_BACKEND, "wch")
        self.assertTrue(any("neither converts" in problem for problem in problems))

    def test_converting_backend_passes(self):
        self.assertEqual(guard.sixteen_bit_findings(HAL_BACKED_CORRECT, "stm32l4"), [])


class RepositoryTests(unittest.TestCase):
    def test_every_backend_agrees(self):
        self.assertEqual(guard.findings(), [])


if __name__ == "__main__":
    unittest.main(verbosity=1)
