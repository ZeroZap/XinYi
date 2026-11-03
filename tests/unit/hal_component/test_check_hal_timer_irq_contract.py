#!/usr/bin/env python3
"""Cases for the timer interrupt contract guard."""

import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import check_hal_timer_irq_contract as guard  # noqa: E402

CORRECT = """
static int timer_irqn(const TIM_HandleTypeDef *handle, IRQn_Type *irqn)
{
    if (handle->Instance == (void *)TIM6) {
        *irqn = TIM6_IRQn;
        return 1;
    }
    return 0;
}

static int arm_nvic(const TIM_HandleTypeDef *handle)
{
    IRQn_Type irqn;
    if (!timer_irqn(handle, &irqn)) {
        return 0;
    }
    HAL_NVIC_SetPriority(irqn, 5U, 0U);
    HAL_NVIC_EnableIRQ(irqn);
    return 1;
}

xy_hal_error_t xy_hal_timer_enable_irq(void *timer, xy_hal_timer_event_t event)
{
    if (event != XY_HAL_TIMER_EVENT_UPDATE) {
        return XY_HAL_ERROR_NOT_SUPPORTED;
    }
    if (!arm_nvic((TIM_HandleTypeDef *)timer)) {
        return XY_HAL_ERROR_NOT_SUPPORTED;
    }
    __HAL_TIM_ENABLE_IT((TIM_HandleTypeDef *)timer, TIM_IT_UPDATE);
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_timer_disable_irq(void *timer, xy_hal_timer_event_t event)
{
    if (event != XY_HAL_TIMER_EVENT_UPDATE) {
        return XY_HAL_ERROR_NOT_SUPPORTED;
    }
    __HAL_TIM_DISABLE_IT((TIM_HandleTypeDef *)timer, TIM_IT_UPDATE);
    return XY_HAL_OK;
}
"""

STARTS = CORRECT.replace(
    "    if (!arm_nvic((TIM_HandleTypeDef *)timer)) {",
    "    if (HAL_TIM_Base_Start_IT((TIM_HandleTypeDef *)timer) != HAL_OK) {\n        return XY_HAL_ERROR_FAIL;\n    }\n    if (!arm_nvic((TIM_HandleTypeDef *)timer)) {")

STOPS = CORRECT.replace("    __HAL_TIM_DISABLE_IT((TIM_HandleTypeDef *)timer, TIM_IT_UPDATE);",
                        "    return HAL_TIM_Base_Stop_IT((TIM_HandleTypeDef *)timer) == HAL_OK ? 0 : -1;")

NO_NVIC = CORRECT.replace("    HAL_NVIC_EnableIRQ(irqn);", "    (void)irqn;")

IGNORES_EVENT = CORRECT.replace(
    """    if (event != XY_HAL_TIMER_EVENT_UPDATE) {
        return XY_HAL_ERROR_NOT_SUPPORTED;
    }
    if (!arm_nvic""",
    """    XY_UNUSED(event);
    if (!arm_nvic""")


def backend(source):
    return {"stm32l4": source}


class EnableTests(unittest.TestCase):
    def test_the_correct_shape_passes(self):
        self.assertEqual(guard.findings(backend(CORRECT)), [])

    def test_starting_the_counter_is_reported(self):
        problems = guard.findings(backend(STARTS))
        self.assertTrue(any("starts the counter" in p for p in problems))

    def test_disabling_by_stopping_the_counter_is_reported(self):
        problems = guard.findings(backend(STOPS))
        self.assertTrue(any("stops the counter" in p for p in problems))

    def test_a_missing_nvic_arm_is_reported(self):
        problems = guard.findings(backend(NO_NVIC))
        self.assertTrue(any("never reaches the NVIC" in p for p in problems))

    def test_a_discarded_event_is_reported(self):
        problems = guard.findings(backend(IGNORES_EVENT))
        self.assertTrue(any("discards `event`" in p for p in problems))


class DelegationTests(unittest.TestCase):
    def test_the_nvic_arm_is_found_behind_a_helper(self):
        """Every backend arms through arm_nvic(); a guard reading one level would cry wolf."""
        body = guard.function_body(CORRECT, "xy_hal_timer_enable_irq(")
        self.assertNotIn("HAL_NVIC_EnableIRQ", body)
        definitions = guard.local_definitions(CORRECT)
        self.assertIn("HAL_NVIC_EnableIRQ", guard.with_delegation(body, definitions))

    def test_a_missing_entry_point_is_reported(self):
        problems = guard.findings({"stm32l4": CORRECT.split("xy_hal_timer_disable_irq")[0]})
        self.assertTrue(any("is missing" in p for p in problems))


class RepositoryTests(unittest.TestCase):
    def test_every_backend_agrees(self):
        self.assertEqual(guard.findings(), [])


if __name__ == "__main__":
    unittest.main(verbosity=1)
