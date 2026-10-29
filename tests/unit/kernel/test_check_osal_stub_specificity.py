#!/usr/bin/env python3
"""Cases for the stub-specificity guard."""

from __future__ import annotations

import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import check_osal_stub_specificity as guard  # noqa: E402


class StubTests(unittest.TestCase):
    def test_a_specific_answer_passes(self):
        source = "xy_os_status_t xy_os_thread_suspend(xy_os_thread_id_t id)\n{\n"
        source += "    return XY_OS_ERROR_NOT_SUPPORTED;\n}\n"
        self.assertEqual([], guard.stub_findings(source, "bm"))

    def test_the_unspecified_return_is_named(self):
        source = "xy_os_status_t xy_os_thread_suspend(xy_os_thread_id_t id)\n{\n    return XY_OS_ERROR;\n}\n"
        problems = guard.stub_findings(source, "bm")
        self.assertEqual(1, len(problems))
        self.assertIn("unspecified RTOS error", problems[0])
        self.assertIn("xy_os_thread_suspend", problems[0])

    def test_a_cast_away_parameter_still_counts_as_bare(self):
        """`(void)param; return XY_OS_ERROR;` is the same non-answer with the warning silenced."""
        source = ("xy_os_status_t xy_os_thread_join(xy_os_thread_id_t id)\n{\n"
                  "    (void)id;\n    return XY_OS_ERROR;\n}\n")
        self.assertEqual(1, len(guard.stub_findings(source, "bm")))

    def test_a_body_with_a_decision_passes(self):
        """Anything that looks at its input is a decision, not a stub."""
        source = ("xy_os_status_t xy_os_thread_suspend(xy_os_thread_id_t id)\n{\n"
                  "    if (!id) return XY_OS_ERROR;\n    return XY_OS_OK;\n}\n")
        self.assertEqual([], guard.stub_findings(source, "bm"))

    def test_a_success_after_a_failed_lookup_passes(self):
        """A single return inside a longer body is not a stub."""
        source = ("xy_os_status_t xy_os_mutex_release(xy_os_mutex_id_t id)\n{\n"
                  "    bm_mutex_t *m = (bm_mutex_t *)id;\n"
                  "    if (!m) return XY_OS_ERROR;\n    m->locked = 0;\n    return XY_OS_OK;\n}\n")
        self.assertEqual([], guard.stub_findings(source, "bm"))


class RealTreeTests(unittest.TestCase):
    def test_the_walk_reaches_the_backends(self):
        found = list(guard.BACKENDS.glob("*/xy_os_*.c"))
        self.assertGreaterEqual(len(found), 4)
        self.assertEqual([], guard.findings())


if __name__ == "__main__":
    unittest.main(verbosity=2)
