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

int xy_hal_dma_get_counter(void *dma)
{
    if (dma == NULL) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    if (find_context(dma) == NULL) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    return (int)__HAL_DMA_GET_COUNTER((DMA_HandleTypeDef *)dma);
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
    return HAL_DMA_PollForTransfer((DMA_HandleTypeDef *)dma, HAL_DMA_FULL_TRANSFER,
                                   timeout == 0U ? HAL_MAX_DELAY : timeout) == HAL_OK
               ? XY_HAL_OK
               : XY_HAL_ERROR_TIMEOUT;
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


class ZeroTimeoutTests(unittest.TestCase):
    POLL_VENDOR = """
xy_hal_error_t xy_hal_dma_poll_complete(void *dma, uint32_t timeout)
{
    if (find_context(dma) == NULL) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    HAL_StatusTypeDef st = HAL_DMA_PollForTransfer((DMA_HandleTypeDef *)dma,
                                                   HAL_DMA_FULL_TRANSFER, %s);
    return st == HAL_OK ? XY_HAL_OK : XY_HAL_ERROR_TIMEOUT;
}
"""
    POLL_LOOP = """
xy_hal_error_t xy_hal_dma_poll_complete(void* dma, uint32_t timeout) {
    if (!contexts[index].initialized)
        return XY_HAL_ERROR_NOT_INIT;
    start = dma_tick_ms();
    while (DMA_GetFlagStatus(complete_flag) == RESET) {
        if (timeout != 0U && (dma_tick_ms() - start) >= timeout)
            return XY_HAL_ERROR_TIMEOUT;
    }
    return XY_HAL_OK;
}
"""

    def test_a_translated_timeout_passes(self):
        source = self.POLL_VENDOR % "timeout == 0U ? HAL_MAX_DELAY : timeout" + CORRECT
        self.assertEqual(guard.findings(backend(source)), [])

    def test_a_bare_timeout_is_reported(self):
        source = self.POLL_VENDOR % "timeout"
        problems = guard.findings(backend(source))
        self.assertTrue(any("unchanged" in p for p in problems))

    def test_an_explicit_zero_test_is_accepted(self):
        problems = guard.findings(backend(self.POLL_LOOP))
        self.assertFalse(any("zero timeout" in p for p in problems))


class ConfigValidationTests(unittest.TestCase):
    VALIDATING = """
static int dma_config_valid(const xy_hal_dma_config_t *config)
{
    if (config->direction > XY_HAL_DMA_DIR_MEM_TO_MEM) return 0;
    if (config->mode > XY_HAL_DMA_MODE_CIRCULAR) return 0;
    if (config->priority > XY_HAL_DMA_PRIORITY_VERY_HIGH) return 0;
    if (config->periph_width > XY_HAL_DMA_WIDTH_WORD) return 0;
    if (config->mem_width > XY_HAL_DMA_WIDTH_WORD) return 0;
    if (config->periph_incr > XY_HAL_DMA_INCR_ENABLE) return 0;
    if (config->mem_incr > XY_HAL_DMA_INCR_ENABLE) return 0;
    return 1;
}

xy_hal_error_t xy_hal_dma_init(void *dma, const xy_hal_dma_config_t *config)
{
    if (dma == NULL || config == NULL || !dma_config_valid(config)) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    return XY_HAL_OK;
}
"""

    def test_validation_through_a_helper_passes(self):
        problems = guard.findings(backend(self.VALIDATING))
        self.assertFalse(any("out-of-range" in p for p in problems))

    def test_an_unvalidated_field_is_reported(self):
        source = self.VALIDATING.replace(
            "    if (config->mem_width > XY_HAL_DMA_WIDTH_WORD) return 0;\n", "")
        problems = guard.findings(backend(source))
        self.assertTrue(any("mem_width" in p and "out-of-range" in p for p in problems))

    def test_a_config_never_examined_is_reported(self):
        source = """
xy_hal_error_t xy_hal_dma_init(void *dma, const xy_hal_dma_config_t *config)
{
    if (dma == NULL || config == NULL) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    return XY_HAL_OK;
}
"""
        problems = guard.findings(backend(source))
        self.assertTrue(any("direction" in p for p in problems))


class ModeTests(unittest.TestCase):
    HONOURS = """
xy_hal_error_t xy_hal_dma_init(void *dma, const xy_hal_dma_config_t *config)
{
    if (dma == NULL || config == NULL) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    hdma->Init.Mode = to_mode(config->mode);
    return XY_HAL_OK;
}
"""

    SILENT_NORMAL = """
xy_hal_error_t xy_hal_dma_init(void *dma, const xy_hal_dma_config_t *config)
{
    if (dma == NULL || config == NULL) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    hdma->Init.Mode = DMA_NORMAL;
    return XY_HAL_OK;
}
"""

    REFUSED = """
xy_hal_error_t xy_hal_dma_init(void *dma, const xy_hal_dma_config_t *config)
{
    if (config->mode == XY_HAL_DMA_MODE_CIRCULAR) {
        return XY_HAL_ERROR_NOT_SUPPORTED;
    }
    hdma->Init.Mode = DMA_NORMAL;
    return XY_HAL_OK;
}
"""

    def test_a_mapped_mode_passes(self):
        problems = guard.findings(backend(self.HONOURS))
        self.assertFalse(any("circular" in p for p in problems))

    def test_a_silent_normal_mode_is_reported(self):
        problems = guard.findings(backend(self.SILENT_NORMAL))
        self.assertTrue(any("constant normal mode" in p for p in problems))

    def test_refusing_circular_is_accepted(self):
        problems = guard.findings(backend(self.REFUSED))
        self.assertFalse(any("constant normal mode" in p for p in problems))


class CallbackRegistrationTests(unittest.TestCase):
    CHECKED = """
xy_hal_error_t xy_hal_dma_init(void *dma, const xy_hal_dma_config_t *config)
{
    if (HAL_DMA_RegisterCallback(hdma, HAL_DMA_XFER_CPLT_CB_ID, dma_cb_complete) != HAL_OK ||
        HAL_DMA_RegisterCallback(hdma, HAL_DMA_XFER_ERROR_CB_ID, dma_cb_error) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }
    return XY_HAL_OK;
}
"""

    UNCHECKED = """
xy_hal_error_t xy_hal_dma_init(void *dma, const xy_hal_dma_config_t *config)
{
    HAL_DMA_RegisterCallback(hdma, HAL_DMA_XFER_CPLT_CB_ID, dma_cb_complete);
    return XY_HAL_OK;
}
"""

    def test_a_checked_registration_passes(self):
        problems = guard.findings(backend(self.CHECKED))
        self.assertFalse(any("RegisterCallback" in p for p in problems))

    def test_an_ignored_registration_is_reported(self):
        problems = guard.findings(backend(self.UNCHECKED))
        self.assertTrue(any("interrupt shims" in p for p in problems))


class CounterTests(unittest.TestCase):
    ANSWERS_NOT_INIT = """
int xy_hal_dma_get_counter(void *dma)
{
    if (dma == NULL) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    if (find_context(dma) == NULL) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    return (int)__HAL_DMA_GET_COUNTER((DMA_HandleTypeDef *)dma);
}
"""

    READS_ANY = """
xy_hal_error_t xy_hal_dma_get_counter(void *dma)
{
    if (dma == NULL) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    return (int)__HAL_DMA_GET_COUNTER((DMA_HandleTypeDef *)dma);
}
"""

    def test_a_guarded_counter_passes(self):
        problems = guard.findings(backend(self.ANSWERS_NOT_INIT))
        self.assertFalse(any("counter register" in p for p in problems))

    def test_an_unguarded_counter_is_reported(self):
        source = self.READS_ANY.replace("xy_hal_error_t", "int")
        problems = guard.findings(backend(source))
        self.assertTrue(any("never set up" in p for p in problems))


class RoutingOwnershipTests(unittest.TestCase):
    STATED = """
xy_hal_error_t xy_hal_dma_init(void *dma, const xy_hal_dma_config_t *config)
{
    /* Init.Request is owned by the caller: a peripheral transfer that leaves it at zero is muxed
     * onto another peripheral's request. */
    hdma->Init.Direction = to_direction(config->direction);
    if (hdma->Init.Request == 0U) {
        hdma->Init.Request = DMA_REQUEST_SW;
    }
    return XY_HAL_OK;
}
"""

    SILENT = """
xy_hal_error_t xy_hal_dma_init(void *dma, const xy_hal_dma_config_t *config)
{
    hdma->Init.Direction = to_direction(config->direction);
    return XY_HAL_OK;
}
"""

    def test_a_stated_boundary_passes(self):
        problems = guard.findings(backend(self.STATED))
        self.assertFalse(any("muxed onto" in p for p in problems))

    def test_a_silent_routing_field_is_reported(self):
        problems = guard.findings(backend(self.STATED), header=Path("/dev/null"))
        self.assertFalse(any("stm32l4: programs or leaves" in p for p in problems))
        problems = guard.findings(backend(self.SILENT + "/* Init.Request left alone */\n"))
        self.assertTrue(any("routing is the caller's" in p for p in problems))

    def test_a_file_without_routing_fields_is_not_asked(self):
        problems = guard.findings(backend(CORRECT))
        self.assertFalse(any("routing is the caller's" in p for p in problems))


class RepositoryTests(unittest.TestCase):
    def test_every_backend_agrees(self):
        self.assertEqual(guard.findings(), [])


if __name__ == "__main__":
    unittest.main(verbosity=1)
