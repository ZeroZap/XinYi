#!/usr/bin/env python3
"""Cases for the blocking UART contract guard.

Two structurally different correct forms exist (the vendor owns the deadline, or the backend
derives a remaining budget), one defect shape comes back naturally (the caller's timeout handed
to a wait helper once per byte), and one trap is easy to introduce while fixing the defect (a
derived budget that reaches zero, which the wait helpers read as "wait forever").

The last two cases exist because the guard fired falsely on correct code: a wait call whose
argument list merely *mentions* timeout - because it passes a derived budget computed from it -
is the fix, not the bug. Requiring the bare timeout is what separates them.
"""

import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import check_hal_uart_blocking_contract as guard  # noqa: E402


VENDOR_FORM = """
xy_hal_error_t xy_hal_uart_recv(void *uart, uint8_t *data, size_t len, uint32_t timeout)
{
    if (uart == NULL || data == NULL || len == 0U || len > UINT16_MAX) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    return uart_status(HAL_UART_Receive(uart, data, (uint16_t)len, timeout));
}
"""

DEADLINE_FORM = """
xy_hal_error_t xy_hal_uart_recv(void* instance, uint8_t* data, size_t len, uint32_t timeout) {
    uint32_t start = uart_tick_ms();
    uint32_t remaining = timeout;
    if (!uart_valid(instance) || data == NULL || len == 0U || len > UINT16_MAX)
        return XY_HAL_ERROR_INVALID_PARAM;
    start = uart_tick_ms();
    for (size_t i = 0U; i < len; ++i) {
        if (uart_budget_exhausted(start, timeout) != 0U)
            return XY_HAL_ERROR_TIMEOUT;
        remaining = uart_budget_remaining(start, timeout);
        if (uart_wait(instance, USART_FLAG_RXNE, remaining) != XY_HAL_OK)
            return XY_HAL_ERROR_TIMEOUT;
        data[i] = (uint8_t)USART_ReceiveData(instance);
    }
    return XY_HAL_OK;
}
"""

PER_BYTE_FORM = """
xy_hal_error_t xy_hal_uart_recv(void* instance, uint8_t* data, size_t len, uint32_t timeout) {
    if (data == NULL || len == 0U || len > UINT16_MAX)
        return XY_HAL_ERROR_INVALID_PARAM;
    for (size_t i = 0U; i < len; ++i) {
        if (uart_wait(instance, USART_FLAG_RXNE, timeout) != XY_HAL_OK)
            return XY_HAL_ERROR_TIMEOUT;
        data[i] = (uint8_t)USART_ReceiveData(instance);
    }
    return XY_HAL_OK;
}
"""

# The trap: a derived budget can be zero, and zero means "wait forever" to these helpers.
BUDGET_WITHOUT_CHECK = """
xy_hal_error_t xy_hal_uart_recv(void* instance, uint8_t* data, size_t len, uint32_t timeout) {
    uint32_t remaining;
    if (data == NULL || len == 0U || len > UINT16_MAX)
        return XY_HAL_ERROR_INVALID_PARAM;
    for (size_t i = 0U; i < len; ++i) {
        remaining = uart_budget_remaining(start, timeout);
        if (uart_wait(instance, USART_FLAG_RXNE, remaining) != XY_HAL_OK)
            return XY_HAL_ERROR_TIMEOUT;
        data[i] = (uint8_t)USART_ReceiveData(instance);
    }
    return XY_HAL_OK;
}
"""

BUDGET_CHECK_AFTER_WAIT = """
xy_hal_error_t xy_hal_uart_recv(void* instance, uint8_t* data, size_t len, uint32_t timeout) {
    uint32_t remaining;
    if (data == NULL || len == 0U || len > UINT16_MAX)
        return XY_HAL_ERROR_INVALID_PARAM;
    for (size_t i = 0U; i < len; ++i) {
        remaining = uart_budget_remaining(start, timeout);
        if (uart_wait(instance, USART_FLAG_RXNE, remaining) != XY_HAL_OK)
            return XY_HAL_ERROR_TIMEOUT;
        if (uart_budget_exhausted(start, timeout) != 0U)
            return XY_HAL_ERROR_TIMEOUT;
    }
    return XY_HAL_OK;
}
"""

HEADER_OK = """
 * @param timeout is the budget for the whole transfer, not per byte.
 * @param timeout is the budget for the whole transfer.
 * len must be non-zero and no larger than UINT16_MAX.
"""


class BodyTests(unittest.TestCase):
    def check(self, body, where="backend"):
        return guard.body_findings(body, where)

    def test_the_vendor_deadline_form_passes(self):
        self.assertEqual(self.check(VENDOR_FORM), [])

    def test_the_derived_budget_form_passes(self):
        self.assertEqual(self.check(DEADLINE_FORM), [])

    def test_a_wait_call_mentioning_timeout_through_a_derived_budget_is_not_the_bug(self):
        # Regression: the first version of this rule matched any wait call whose arguments
        # contained the word timeout, which flagged the correct WCH implementation.
        self.assertFalse(any("restarts the clock"
                             in problem for problem in self.check(DEADLINE_FORM)))

    def test_the_bare_timeout_in_a_loop_is_reported(self):
        problems = self.check(PER_BYTE_FORM)
        self.assertTrue(any("restarts the clock" in problem for problem in problems))

    def test_a_budget_that_can_reach_zero_without_a_timeout_is_reported(self):
        problems = self.check(BUDGET_WITHOUT_CHECK)
        self.assertTrue(any("wait forever" in problem for problem in problems))

    def test_reporting_the_exhausted_budget_after_the_wait_is_reported(self):
        problems = self.check(BUDGET_CHECK_AFTER_WAIT)
        self.assertTrue(any("before the exhausted case" in problem for problem in problems))

    def test_a_missing_length_limit_is_reported(self):
        body = VENDOR_FORM.replace(" || len > UINT16_MAX", "")
        self.assertTrue(any("UINT16_MAX" in problem for problem in self.check(body)))

    def test_zero_length_acceptance_is_reported(self):
        body = VENDOR_FORM.replace("len == 0U || ", "")
        self.assertTrue(any("zero length" in problem for problem in self.check(body)))

    def test_a_missing_function_is_reported(self):
        self.assertEqual(len(self.check("")), 1)


class HeaderTests(unittest.TestCase):
    def test_the_documented_semantics_pass(self):
        self.assertEqual(guard.header_findings(HEADER_OK), [])

    def test_a_header_that_states_it_once_is_reported(self):
        # Both blocking calls have to say it; one comment covering one of them is how the
        # second call ended up with different semantics in the first place.
        header = HEADER_OK.replace(" * @param timeout is the budget for the whole transfer, not per byte.\n", "")
        problems = guard.header_findings(header)
        # Two consequences: only one call states the budget, and the "per byte is wrong"
        # sentence went with it.
        self.assertEqual(len(problems), 2)
        self.assertTrue(any("both blocking calls" in problem for problem in problems))

    def test_a_silent_header_is_reported(self):
        self.assertEqual(len(guard.header_findings(" * Timeout in milliseconds\n")), 4)


class RepositoryTests(unittest.TestCase):
    def test_every_backend_agrees(self):
        self.assertEqual(guard.findings(), [])


if __name__ == "__main__":
    unittest.main(verbosity=1)
