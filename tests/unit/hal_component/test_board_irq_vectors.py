#!/usr/bin/env python3
"""Synthetic cases for the board interrupt vector guard.

Against the real repository the guard can only be seen passing. These cases feed it the
shapes that must fail - a vector that reads the flag and returns, a timer vector that
bypasses the HAL without declaring a boundary, a boundary declared with no reason - so the
failing behaviour is tested rather than assumed.
"""

import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import check_board_irq_vectors as guard  # noqa: E402


UART_SWALLOWING = """
void USART1_IRQHandler(void)
{
    (void)USART1->STATR;
}
"""

UART_FORWARDING_WCH = """
void USART1_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void USART1_IRQHandler(void) {
    xy_hal_uart_irq_handler(USART1);
}
"""

UART_FORWARDING_STM32 = """
void USART2_IRQHandler(void)
{
    HAL_UART_IRQHandler(&huart2);
}
"""

EXTI_FORWARDING = """
void EXTI9_5_IRQHandler(void) { xy_hal_gpio_irq_handler(GPIOB, 8U); }
"""

EXTI_SWALLOWING = """
void EXTI9_5_IRQHandler(void)
{
    EXTI->PR1 = (1U << 8);
}
"""

TIM_BYPASSING = """
void TIM3_IRQHandler(void)
{
    TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
    count++;
}
"""

TIM_FORWARDING = """
void TIM6_DAC_IRQHandler(void)
{
    xy_hal_timer_irq_handler(&pandora_tim6);
}
"""

ARMING = """
static void start(void)
{
    if (xy_hal_uart_enable_rx_irq(USART1, 1) == XY_HAL_OK) {
        ready = 1;
    }
}
"""

DISARMING = """
static void stop(void)
{
    (void)xy_hal_uart_enable_rx_irq(USART1, 0);
}
"""


class VectorTests(unittest.TestCase):
    def test_a_swallowing_uart_vector_is_reported(self):
        findings = guard.vector_findings(guard.vectors_in(UART_SWALLOWING), "board")
        self.assertEqual(len(findings), 1)
        self.assertIn("does not hand the interrupt to the HAL", findings[0])

    def test_the_hal_entry_point_counts_as_forwarding(self):
        self.assertEqual(guard.vector_findings(guard.vectors_in(UART_FORWARDING_WCH), "b"), [])

    def test_the_vendor_handler_counts_as_forwarding(self):
        self.assertEqual(guard.vector_findings(guard.vectors_in(UART_FORWARDING_STM32), "b"),
                         [])

    def test_an_attribute_declaration_is_not_a_second_vector(self):
        self.assertEqual(len(guard.vectors_in(UART_FORWARDING_WCH)), 1)

    def test_a_gpio_vector_must_use_the_gpio_entry_point(self):
        self.assertEqual(guard.vector_findings(guard.vectors_in(EXTI_FORWARDING), "b"), [])
        findings = guard.vector_findings(guard.vectors_in(EXTI_SWALLOWING), "b")
        self.assertEqual(len(findings), 1)
        self.assertIn("xy_hal_gpio_irq_handler", findings[0])

    def test_a_timer_vector_bypassing_the_hal_is_reported(self):
        findings = guard.vector_findings(guard.vectors_in(TIM_BYPASSING), "b")
        self.assertEqual(len(findings), 1)

    def test_a_timer_vector_using_the_hal_passes(self):
        self.assertEqual(guard.vector_findings(guard.vectors_in(TIM_FORWARDING), "b"), [])

    def test_a_declared_boundary_vector_passes(self):
        findings = guard.vector_findings(guard.vectors_in(TIM_BYPASSING), "b",
                                         {"TIM3_IRQHandler": "board owns it for the demo"})
        self.assertEqual(findings, [])

    def test_a_boundary_without_a_reason_is_still_a_finding(self):
        findings = guard.vector_findings(guard.vectors_in(TIM_BYPASSING), "b",
                                         {"TIM3_IRQHandler": "   "})
        self.assertEqual(len(findings), 1)
        self.assertIn("without a reason", findings[0])

    def test_an_unknown_family_must_still_hand_the_interrupt_on(self):
        source = "void I2C1_EV_IRQHandler(void) { reg++; }"
        findings = guard.vector_findings(guard.vectors_in(source), "b")
        self.assertEqual(len(findings), 1)
        source_ok = "void I2C1_EV_IRQHandler(void) { xy_hal_i2c_irq_handler(hi2c1); }"
        self.assertEqual(guard.vector_findings(guard.vectors_in(source_ok), "b"), [])


class ArmingTests(unittest.TestCase):
    def test_arming_without_any_vector_is_reported(self):
        findings = guard.arming_findings({"main.c": ARMING}, set(), False, "board")
        self.assertEqual(len(findings), 1)
        self.assertIn("no UART vector exists", findings[0])

    def test_arming_with_a_vector_is_fine(self):
        self.assertEqual(
            guard.arming_findings({"main.c": ARMING}, {"USART1_IRQHandler"}, False, "board"), [])

    def test_arming_is_fine_when_the_backend_supplies_vectors(self):
        self.assertEqual(guard.arming_findings({"main.c": ARMING}, set(), True, "board"), [])

    def test_disarming_is_not_arming(self):
        self.assertEqual(guard.arming_findings({"main.c": DISARMING}, set(), False, "board"), [])

    def test_a_board_that_never_touches_the_interrupt_is_not_blamed(self):
        self.assertEqual(
            guard.arming_findings({"main.c": "int main(void) { return 0; }"}, set(), False, "b"),
            [])


class RepositoryTests(unittest.TestCase):
    def test_the_repository_passes(self):
        self.assertEqual(guard.scan_repository(), [])

    def test_the_boundary_list_is_not_silently_empty(self):
        # The declared boundaries are printed on every run; if the list empties while
        # TIM2 vectors still bypass the HAL, the guard should have complained instead.
        self.assertTrue(all(reason.strip() for reason in guard.BOUNDARY_VECTORS.values()))


if __name__ == "__main__":
    unittest.main(verbosity=1)
