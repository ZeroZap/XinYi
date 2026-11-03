#!/usr/bin/env python3
"""Synthetic cases for the board interrupt vector guard.

Against the real repository the guard can only be seen passing. These cases feed it the
shapes that must fail - a vector that reads the flag and returns, a timer vector that
bypasses the HAL without declaring a boundary, a boundary declared with no reason - and the
shapes that must not be mistaken for failures: a vector that only exists in a comment, one
that forwards through a local dispatcher, one that lives in its own HAL implementation.
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

UART_FORWARDING_ONE_HOP = """
static void dispatch(void *instance)
{
    HAL_UART_IRQHandler(instance);
}

void USART1_IRQHandler(void)
{
    dispatch((void *)USART1);
}
"""

UART_ONLY_IN_A_COMMENT = """
/* The board used to install one:
 * void USART1_IRQHandler(void) {
 *     HAL_UART_IRQHandler(&huart1);
 * }
 */
int main(void) { return 0; }
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

BOARD = "boards/some_board/main.c"
HAL_WRAPPER = "components/hal/stm32/stm32l4/xy_hal_uart.c"


class VectorTests(unittest.TestCase):
    def check(self, source, path=BOARD, boundary=None):
        return guard.vector_findings(source, "f", path, boundary or {})

    def test_a_swallowing_uart_vector_is_reported(self):
        findings = self.check(UART_SWALLOWING)
        self.assertEqual(len(findings), 1)
        self.assertIn("does not hand the interrupt to the HAL", findings[0])

    def test_the_hal_entry_point_counts_as_forwarding(self):
        self.assertEqual(self.check(UART_FORWARDING_WCH), [])

    def test_the_vendor_handler_counts_as_forwarding(self):
        self.assertEqual(self.check(UART_FORWARDING_STM32), [])

    def test_forwarding_through_a_local_dispatcher_counts(self):
        self.assertEqual(self.check(UART_FORWARDING_ONE_HOP), [])

    def test_a_vector_that_only_appears_in_a_comment_is_not_code(self):
        self.assertEqual(self.check(UART_ONLY_IN_A_COMMENT), [])

    def test_an_attribute_declaration_is_not_a_second_vector(self):
        self.assertEqual(len(guard.vectors_in(UART_FORWARDING_WCH)), 1)

    def test_a_gpio_vector_must_use_the_gpio_entry_point(self):
        self.assertEqual(self.check(EXTI_FORWARDING), [])
        findings = self.check(EXTI_SWALLOWING)
        self.assertEqual(len(findings), 1)
        self.assertIn("xy_hal_gpio_irq_handler", findings[0])

    def test_a_timer_vector_bypassing_the_hal_is_reported(self):
        self.assertEqual(len(self.check(TIM_BYPASSING)), 1)

    def test_a_timer_vector_using_the_hal_passes(self):
        self.assertEqual(self.check(TIM_FORWARDING), [])

    def test_a_vector_inside_its_own_hal_implementation_needs_no_explanation(self):
        self.assertEqual(self.check(UART_SWALLOWING, HAL_WRAPPER), [])

    def test_the_ownership_exemption_does_not_leak_outside_components_hal(self):
        self.assertEqual(len(self.check(UART_SWALLOWING, "boards/some_board/xy_hal_uart.c")), 1)

    def test_a_declared_boundary_passes(self):
        self.assertEqual(
            self.check(TIM_BYPASSING, boundary={(BOARD, "TIM3_IRQHandler"): "board owns it"}), [])

    def test_a_boundary_keyed_to_another_file_does_not_exempt_this_one(self):
        findings = self.check(TIM_BYPASSING,
                              boundary={("boards/other/main.c", "TIM3_IRQHandler"): "reason"})
        self.assertEqual(len(findings), 1)

    def test_a_boundary_without_a_reason_is_still_a_finding(self):
        findings = self.check(TIM_BYPASSING, boundary={(BOARD, "TIM3_IRQHandler"): "   "})
        self.assertEqual(len(findings), 1)
        self.assertIn("without a reason", findings[0])

    def test_an_unknown_family_must_still_hand_the_interrupt_on(self):
        self.assertEqual(len(self.check("void I2C1_EV_IRQHandler(void) { reg++; }")), 1)
        self.assertEqual(
            self.check("void I2C1_EV_IRQHandler(void) { xy_hal_i2c_irq_handler(hi2c1); }"), [])


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

    def test_every_declared_boundary_points_at_a_real_vector(self):
        for line in guard.boundary_report():
            self.assertNotIn("stale", line, msg=line)

    def test_unchecked_sources_are_named_rather_than_silently_skipped(self):
        # The vendor samples that define vectors but appear in no build have to be reported,
        # or "not checked" would be indistinguishable from "checked and clean".
        for note in guard.unchecked_vectors():
            self.assertIn("not checked", note)


if __name__ == "__main__":
    unittest.main(verbosity=1)
