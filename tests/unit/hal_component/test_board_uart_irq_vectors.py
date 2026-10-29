#!/usr/bin/env python3
"""Synthetic cases for the board UART vector guard.

Run against a real repository, the guard can only ever be seen passing - there is exactly
one vector in the tree today and it is correct. These cases feed it boards that swallow the
interrupt, boards that arm without a vector, and boards that do it properly, so the failing
shapes are known to fail.
"""

import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import check_board_uart_irq_vectors as guard  # noqa: E402


SWALLOWING = """
void USART1_IRQHandler(void)
{
    /* reads the flag, clears it, never tells the HAL */
    reg = USART1->ISR;
}
"""

FORWARDING_WCH = """
void USART1_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void USART1_IRQHandler(void) {
    xy_hal_uart_irq_handler(USART1);
}
"""

FORWARDING_STM32 = """
void USART2_IRQHandler(void)
{
    HAL_UART_IRQHandler(&huart2);
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
    def test_a_swallowing_vector_is_reported(self):
        vectors = guard.vectors_in(SWALLOWING)
        self.assertEqual(set(vectors), {"USART1_IRQHandler"})
        findings = guard.vector_findings(vectors, "board")
        self.assertEqual(len(findings), 1)
        self.assertIn("does not forward", findings[0])

    def test_the_hal_entry_point_counts_as_forwarding(self):
        self.assertEqual(guard.vector_findings(guard.vectors_in(FORWARDING_WCH), "b"), [])

    def test_the_vendor_handler_counts_as_forwarding(self):
        self.assertEqual(guard.vector_findings(guard.vectors_in(FORWARDING_STM32), "b"), [])

    def test_an_attribute_declaration_is_not_mistaken_for_a_second_vector(self):
        vectors = guard.vectors_in(FORWARDING_WCH)
        self.assertEqual(len(vectors), 1)


class ArmingTests(unittest.TestCase):
    def test_arming_without_any_vector_is_reported(self):
        findings = guard.arming_findings({"main.c": ARMING}, set(), False, "board")
        self.assertEqual(len(findings), 1)
        self.assertIn("no UART vector exists", findings[0])

    def test_arming_with_a_vector_is_fine(self):
        findings = guard.arming_findings({"main.c": ARMING}, {"USART1_IRQHandler"}, False,
                                         "board")
        self.assertEqual(findings, [])

    def test_arming_is_fine_when_the_backend_supplies_vectors(self):
        findings = guard.arming_findings({"main.c": ARMING}, set(), True, "board")
        self.assertEqual(findings, [])

    def test_disarming_is_not_arming(self):
        findings = guard.arming_findings({"main.c": DISARMING}, set(), False, "board")
        self.assertEqual(findings, [])

    def test_a_board_that_never_touches_the_interrupt_is_not_blamed(self):
        findings = guard.arming_findings({"main.c": "int main(void) { return 0; }"},
                                         set(), False, "board")
        self.assertEqual(findings, [])


class RepositoryTests(unittest.TestCase):
    def test_the_repository_passes(self):
        self.assertEqual(guard.scan_repository(), [])


if __name__ == "__main__":
    unittest.main(verbosity=1)
