#!/usr/bin/env python3
"""Cases for the DMA item-count contract guard."""

import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import check_hal_dma_contract as guard  # noqa: E402

CORRECT = """
xy_hal_error_t xy_hal_dma_start(void *dma, uint32_t src_addr, uint32_t dst_addr, size_t data_len)
{
    if (dma == NULL || data_len == 0U || data_len > 0xFFFFU) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    return HAL_DMA_Start((DMA_HandleTypeDef *)dma, src_addr, dst_addr, (uint32_t)data_len) == HAL_OK
               ? XY_HAL_OK
               : XY_HAL_ERROR_FAIL;
}

xy_hal_error_t xy_hal_dma_stop(void *dma)
{
    if (find_context(dma) == NULL) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_dma_poll_complete(void *dma, uint32_t timeout)
{
    if (((DMA_HandleTypeDef *)dma)->State == HAL_DMA_STATE_RESET) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    return XY_HAL_OK;
}
"""

UNCAPPED = CORRECT.replace("|| data_len > 0xFFFFU", "")
NO_ZERO = CORRECT.replace("data_len == 0U || ", "")
STOP_WITHOUT_NOT_INIT = CORRECT.replace(
    """    if (find_context(dma) == NULL) {
        return XY_HAL_ERROR_NOT_INIT;
    }
""", "")


def backend(source):
    return {"stm32l4": source}


class ItemCountTests(unittest.TestCase):
    def test_the_correct_shape_passes(self):
        self.assertEqual(guard.findings(backend(CORRECT)), [])

    def test_a_missing_counter_cap_is_reported(self):
        problems = guard.findings(backend(UNCAPPED))
        self.assertTrue(any("hardware counter" in p for p in problems))

    def test_a_zero_count_being_accepted_is_reported(self):
        problems = guard.findings(backend(NO_ZERO))
        self.assertTrue(any("zero item count" in p for p in problems))

    def test_a_stop_that_hides_not_init_is_reported(self):
        problems = guard.findings(backend(STOP_WITHOUT_NOT_INIT))
        self.assertTrue(any("NOT_INIT" in p for p in problems))

    def test_uint16_max_is_also_accepted_as_a_cap(self):
        source = CORRECT.replace("data_len > 0xFFFFU", "data_len > UINT16_MAX")
        self.assertEqual(guard.findings(backend(source)), [])


class HeaderTests(unittest.TestCase):
    def test_the_unit_and_limit_must_be_documented(self):
        problems = guard.findings(backend(CORRECT), header=Path("/dev/null"))
        self.assertTrue(any("count of items" in p for p in problems))
        self.assertTrue(any("item counter limit" in p for p in problems))


class RepositoryTests(unittest.TestCase):
    def test_every_backend_agrees(self):
        self.assertEqual(guard.findings(), [])


if __name__ == "__main__":
    unittest.main(verbosity=1)
