#!/usr/bin/env python3
"""Cases for the xy_hal_uart_recv contract guard.

The two forms the guard has to accept are structurally different: STM32 hands the timeout to a
vendor call that owns the deadline, WCH computes a remaining budget across a polling loop. Both
are correct and neither looks like the other, so the cases pin both - and pin the shape that was
actually wrong, which is also the shape most likely to come back: a wait helper inside a loop
being handed the caller's full timeout.
"""

import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import check_hal_uart_recv_contract as guard  # noqa: E402


VENDOR_FORM = """
xy_hal_error_t xy_hal_uart_recv(void *uart, uint8_t *data, size_t len, uint32_t timeout)
{
    if (uart == NULL || data == NULL || len == 0U) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    return uart_status(HAL_UART_Receive(uart, data, (uint16_t)len, timeout));
}
"""

DEADLINE_FORM = """
xy_hal_error_t xy_hal_uart_recv(void* instance, uint8_t* data, size_t len, uint32_t timeout) {
    uint32_t start = uart_tick_ms();
    uint32_t remaining = timeout;
    if (data == NULL || len == 0U)
        return XY_HAL_ERROR_INVALID_PARAM;
    for (size_t i = 0U; i < len; ++i) {
        remaining = timeout - (uart_tick_ms() - start);
        if (uart_wait(instance, USART_FLAG_RXNE, remaining) != XY_HAL_OK)
            return XY_HAL_ERROR_TIMEOUT;
        data[i] = (uint8_t)USART_ReceiveData(instance);
    }
    return XY_HAL_OK;
}
"""

PER_BYTE_FORM = """
xy_hal_error_t xy_hal_uart_recv(void* instance, uint8_t* data, size_t len, uint32_t timeout) {
    if (data == NULL)
        return XY_HAL_ERROR_INVALID_PARAM;
    for (size_t i = 0U; i < len; ++i) {
        if (uart_wait(instance, USART_FLAG_RXNE, timeout) != XY_HAL_OK)
            return XY_HAL_ERROR_TIMEOUT;
        data[i] = (uint8_t)USART_ReceiveData(instance);
    }
    return XY_HAL_OK;
}
"""

HEADER_OK = """
 * @param timeout is the budget for the whole transfer, not per byte.
 * @param len must be non-zero: a zero-length receive is a caller mistake.
"""


class BodyTests(unittest.TestCase):
    def check(self, body, where="backend"):
        return guard.body_findings(body, where)

    def test_the_vendor_deadline_form_passes(self):
        self.assertEqual(self.check(VENDOR_FORM), [])

    def test_the_derived_budget_form_passes(self):
        self.assertEqual(self.check(DEADLINE_FORM), [])

    def test_a_per_byte_wait_restarting_the_clock_is_reported(self):
        problems = self.check(PER_BYTE_FORM)
        self.assertTrue(any("full budget" in problem for problem in problems))

    def test_a_zero_length_success_is_reported(self):
        problems = self.check(PER_BYTE_FORM + "\n")
        self.assertTrue(any("zero length" in problem for problem in problems))

    def test_rejecting_zero_length_but_returning_the_wrong_error_is_reported(self):
        body = VENDOR_FORM.replace("XY_HAL_ERROR_INVALID_PARAM", "XY_HAL_ERROR_FAIL")
        problems = self.check(body)
        self.assertTrue(any("INVALID_PARAM" in problem for problem in problems))

    def test_a_body_without_any_deadline_is_reported(self):
        body = """
        xy_hal_error_t xy_hal_uart_recv(void* u, uint8_t* d, size_t len, uint32_t timeout) {
            if (len == 0U) return XY_HAL_ERROR_INVALID_PARAM;
            while (1) { poll(); }
            return XY_HAL_OK;
        }
        """
        problems = self.check(body)
        self.assertTrue(any("no single deadline" in problem for problem in problems))

    def test_a_missing_function_is_reported(self):
        self.assertEqual(len(self.check("")), 1)


class HeaderTests(unittest.TestCase):
    def test_the_documented_semantics_pass(self):
        self.assertEqual(guard.header_findings(HEADER_OK), [])

    def test_a_silent_header_is_reported(self):
        problems = guard.header_findings(" * @param timeout Timeout in milliseconds\n")
        self.assertEqual(len(problems), 3)

    def test_stating_the_budget_without_forbidding_per_byte_is_reported(self):
        header = "the budget for the whole transfer\nlen must be non-zero\n"
        problems = guard.header_findings(header)
        self.assertEqual(len(problems), 1)
        self.assertIn("per-byte", problems[0])


class RepositoryTests(unittest.TestCase):
    def test_every_backend_agrees(self):
        self.assertEqual(guard.findings(), [])


if __name__ == "__main__":
    unittest.main(verbosity=1)
