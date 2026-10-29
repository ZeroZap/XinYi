#!/usr/bin/env python3
"""Cases for the timeout-unit guard.

The defect is an argument whose meaning was never written down, so the cases are about what the
documentation says and who hands the value to a function that reads a different unit.
"""

from __future__ import annotations

import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import check_osal_timeout_units as guard  # noqa: E402


class DocumentedUnitTests(unittest.TestCase):
    def test_a_documented_unit_passes(self):
        header = " * @param[in] timeout Ticks to wait; XY_OS_WAIT_FOREVER to block\n"
        self.assertEqual([], guard.undocumented_timeouts(header))

    def test_a_missing_unit_is_named(self):
        header = " * @param[in] timeout Timeout value or 0 (no wait)\n"
        problems = guard.undocumented_timeouts(header)
        self.assertEqual(1, len(problems))
        self.assertIn("without naming its unit", problems[0])

    def test_a_header_with_no_timeout_parameter_fails(self):
        """No lines found reads exactly like every line being documented."""
        problems = guard.undocumented_timeouts(" * @brief nothing to see\n")
        self.assertEqual(1, len(problems))
        self.assertIn("a parser that finds nothing", problems[0])


class EntryCountTests(unittest.TestCase):
    def test_matching_counts_pass(self):
        header = ("xy_os_status_t xy_os_mutex_acquire(xy_os_mutex_id_t id, uint32_t timeout);\n"
                  " * @param[in] timeout Ticks to wait\n")
        self.assertEqual([], guard.wait_entries_unmentioned(header))

    def test_an_undocumented_entry_is_named(self):
        header = ("xy_os_status_t xy_os_mutex_acquire(xy_os_mutex_id_t id, uint32_t timeout);\n"
                  "xy_os_status_t xy_os_semaphore_acquire(xy_os_semaphore_id_t id, uint32_t timeout);\n"
                  " * @param[in] timeout Ticks to wait\n")
        problems = guard.wait_entries_unmentioned(header)
        self.assertEqual(1, len(problems))
        self.assertIn("2 entries take a timeout but 1", problems[0])


class RawMillisecondTests(unittest.TestCase):
    def test_a_converted_pass_is_not_flagged(self):
        source = "osStatus_t s = osMutexAcquire(m, ticks_to_ms(timeout));\n"
        self.assertEqual([], guard.raw_ms_passes(source))

    def test_a_raw_pass_is_flagged(self):
        source = "osStatus_t s = osMutexAcquire((osMutexId_t)mutex_id, timeout);\n"
        self.assertEqual(1, len(guard.raw_ms_passes(source)))

    def test_a_call_without_the_public_timeout_is_not_flagged(self):
        source = "osStatus_t s = osMutexAcquire(m, osWaitForever);\n"
        self.assertEqual([], guard.raw_ms_passes(source))

    def test_a_pinned_backend_is_not_named(self):
        real = Path(__file__).resolve().parents[3] / "components/kernel/osal/backend"
        problems = guard.backend_findings(real, {"cmsis_rtx": "why", "rtx5": "why"})
        self.assertEqual([], problems)

    def test_a_pin_that_closes_is_named(self):
        """A backend whose raw passes are gone must not stay pinned."""
        real = Path(__file__).resolve().parents[3] / "components/kernel/osal/backend"
        problems = guard.backend_findings(real, {"freertos": "pinned but it converts nothing raw"})
        self.assertTrue(any("freertos" in p and "pin is stale" in p for p in problems), problems)


class RealTreeTests(unittest.TestCase):
    def test_the_header_documents_the_unit(self):
        self.assertEqual([], guard.header_findings())

    def test_the_real_backends_pass(self):
        self.assertEqual([], guard.backend_findings(guard.BACKENDS))

    def test_the_walk_reaches_every_backend(self):
        found = {p.parent.name for p in guard.BACKENDS.glob("*/xy_os_*.c")}
        self.assertGreaterEqual(len(found), 4)
        self.assertIn("cmsis_rtx", guard.PINNED_RAW_MS)


if __name__ == "__main__":
    unittest.main(verbosity=2)
