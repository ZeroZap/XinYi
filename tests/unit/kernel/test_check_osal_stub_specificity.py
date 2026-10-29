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


class LocalRejectionTests(unittest.TestCase):
    """A backend's own check knows why it refused; the unspecified code throws that away."""

    def test_a_local_rejection_with_the_unspecified_code_is_named(self):
        source = ("xy_os_status_t xy_os_timer_stop(xy_os_timer_id_t id)\n{\n"
                  "    if (!id) {\n        return XY_OS_ERROR;\n    }\n    return XY_OS_OK;\n}\n")
        problems = guard.local_check_findings(source, "bm", {}, {})
        self.assertEqual(1, len(problems))
        self.assertIn("xy_os_timer_stop", problems[0])

    def test_the_same_check_with_a_specific_code_passes(self):
        source = ("xy_os_status_t xy_os_timer_stop(xy_os_timer_id_t id)\n{\n"
                  "    if (!id) {\n        return XY_OS_ERROR_PARAMETER;\n    }\n"
                  "    return XY_OS_OK;\n}\n")
        self.assertEqual([], guard.local_check_findings(source, "bm", {}, {}))

    def test_a_vendor_result_is_not_a_local_check(self):
        """A failure reported by the vendor is genuinely unspecified at this layer."""
        source = ("xy_os_status_t xy_os_mutex_release(xy_os_mutex_id_t id)\n{\n"
                  "    if (xSemaphoreGive(m) != pdPASS) {\n        return XY_OS_ERROR;\n    }\n"
                  "    return XY_OS_OK;\n}\n")
        self.assertEqual([], guard.local_check_findings(source, "ft", {}, {}))

    def test_a_pinned_site_passes(self):
        source = ("xy_os_status_t xy_os_mutex_release(xy_os_mutex_id_t id)\n{\n"
                  "    if (!m->locked) { return XY_OS_ERROR; }\n    return XY_OS_OK;\n}\n")
        sites = {("bm", "xy_os_mutex_release"): "pinned"}
        self.assertEqual([], guard.local_check_findings(source, "bm", {}, sites))

    def test_a_pinned_backend_is_skipped_whole(self):
        source = ("xy_os_status_t xy_os_timer_stop(xy_os_timer_id_t id)\n{\n"
                  "    if (!id) { return XY_OS_ERROR; }\n    return XY_OS_OK;\n}\n")
        self.assertEqual([], guard.local_check_findings(source, "rtthread", {"rtthread": "unbuildable"}, {}))


class FalseSuccessTests(unittest.TestCase):
    """A flag-word entry answering 0 is indistinguishable from a working backend with nothing set."""

    def test_a_bare_zero_flag_answer_is_named(self):
        source = "uint32_t xy_os_thread_flags_get(void)\n{\n    return 0;\n}\n"
        problems = guard.false_success_findings(source, "bm")
        self.assertEqual(1, len(problems))
        self.assertIn("XY_OS_THREAD_FLAGS_ERROR", problems[0])

    def test_the_error_answer_passes(self):
        source = "uint32_t xy_os_thread_flags_get(void)\n{\n    return XY_OS_THREAD_FLAGS_ERROR;\n}\n"
        self.assertEqual([], guard.false_success_findings(source, "bm"))

    def test_a_cast_away_parameter_with_zero_still_counts(self):
        source = ("uint32_t xy_os_thread_flags_clear(uint32_t f)\n{\n    (void)f;\n    return 0;\n}\n")
        self.assertEqual(1, len(guard.false_success_findings(source, "bm")))

    def test_an_entry_that_can_succeed_is_not_a_stub(self):
        source = ("uint32_t xy_os_thread_flags_get(void)\n{\n    if (!s_ready) return XY_OS_THREAD_FLAGS_ERROR;\n"
                  "    return s_flags;\n}\n")
        self.assertEqual([], guard.false_success_findings(source, "bm"))

    def test_other_entries_returning_zero_are_left_alone(self):
        """An event-group read answering 0 is genuinely "nothing set" - that call has state behind it."""
        source = "uint32_t xy_os_event_flags_get(xy_os_event_flags_id_t id)\n{\n    return 0;\n}\n"
        self.assertEqual([], guard.false_success_findings(source, "bm"))


class ConstantQueryTests(unittest.TestCase):
    """A query answering a constant is indistinguishable from a working one."""

    def test_a_bare_null_query_is_named(self):
        source = "xy_os_thread_id_t xy_os_mutex_get_owner(xy_os_mutex_id_t id)\n{\n    (void)id;\n    return NULL;\n}\n"
        problems = guard.constant_query_findings(source, "bm", {})
        self.assertEqual(1, len(problems))
        self.assertIn("xy_os_mutex_get_owner", problems[0])

    def test_a_bare_zero_query_is_named(self):
        source = "uint32_t xy_os_thread_get_count(void)\n{\n    return 0U;\n}\n"
        self.assertEqual(1, len(guard.constant_query_findings(source, "bm", {})))

    def test_a_query_that_reads_state_passes(self):
        source = ("xy_os_thread_id_t xy_os_mutex_get_owner(xy_os_mutex_id_t id)\n{\n"
                  "    bm_mutex_t *m = (bm_mutex_t *)id;\n    return m->locked ? m->owner : NULL;\n}\n")
        self.assertEqual([], guard.constant_query_findings(source, "bm", {}))

    def test_a_pinned_reason_is_accepted(self):
        source = "uint32_t xy_os_thread_get_count(void)\n{\n    return 0U;\n}\n"
        sites = {("ft", "xy_os_thread_get_count"): "deliberate: kernel-global inventory is not ours"}
        self.assertEqual([], guard.constant_query_findings(source, "ft", sites))

    def test_a_non_osal_function_is_left_alone(self):
        source = "int bm_helper(void)\n{\n    return 0;\n}\n"
        self.assertEqual([], guard.constant_query_findings(source, "bm", {}))


class UnreadParameterTests(unittest.TestCase):
    """A value-returning function that ignores its argument answers about something else."""

    def test_a_query_ignoring_its_id_is_named(self):
        source = ("xy_os_thread_state_t xy_os_thread_get_state(xy_os_thread_id_t id)\n{\n"
                  "    return XY_OS_THREAD_RUNNING;\n}\n")
        problems = guard.unread_parameter_findings(source, "bm", {})
        self.assertEqual(1, len(problems))
        self.assertIn("without reading id", problems[0])

    def test_a_query_that_reads_its_id_passes(self):
        source = ("xy_os_thread_state_t xy_os_thread_get_state(xy_os_thread_id_t id)\n{\n"
                  "    return (id == xy_os_thread_get_id()) ? XY_OS_THREAD_RUNNING : XY_OS_THREAD_ERROR;\n}\n")
        self.assertEqual([], guard.unread_parameter_findings(source, "bm", {}))

    def test_a_status_function_may_refuse_without_reading(self):
        source = ("xy_os_status_t xy_os_thread_suspend(xy_os_thread_id_t id)\n{\n"
                  "    return XY_OS_ERROR_NOT_SUPPORTED;\n}\n")
        self.assertEqual([], guard.unread_parameter_findings(source, "bm", {}))

    def test_a_pinned_reason_is_accepted(self):
        source = "uint32_t xy_os_thread_get_stack_size(xy_os_thread_id_t id)\n{\n    return 0;\n}\n"
        sites = {("bm", "xy_os_thread_get_stack_size"): "no per-thread stacks"}
        self.assertEqual([], guard.unread_parameter_findings(source, "bm", sites))


class ApiVersionTests(unittest.TestCase):
    """The api field says which contract this backend speaks, not a number typed into it."""

    def test_a_literal_is_named(self):
        source = "xy_os_status_t xy_os_kernel_get_info(void)\n{\n    version->api = (1 << 16);\n}\n"
        problems = guard.api_version_findings(source, "fr", {})
        self.assertEqual(1, len(problems))
        self.assertIn("not this adapter's version", problems[0])

    def test_the_adapters_macros_pass(self):
        source = ("xy_os_status_t xy_os_kernel_get_info(void)\n{\n"
                  "    version->api = (XY_OSAL_VERSION_MAJOR << 16) | XY_OSAL_VERSION_MINOR;\n}\n")
        self.assertEqual([], guard.api_version_findings(source, "fr", {}))

    def test_a_vendor_query_passes(self):
        source = "xy_os_status_t xy_os_kernel_get_info(void)\n{\n    version->api = osKernelGetVersion().api;\n}\n"
        self.assertEqual([], guard.api_version_findings(source, "cm", {}))

    def test_a_pinned_backend_is_accepted(self):
        source = "xy_os_status_t xy_os_kernel_get_info(void)\n{\n    version->api = 0x20000;\n}\n"
        sites = {("rt", "version->api"): "unreachable from every build"}
        self.assertEqual([], guard.api_version_findings(source, "rt", sites))


class RealTreeTests(unittest.TestCase):
    def test_the_walk_reaches_the_backends(self):
        found = list(guard.BACKENDS.glob("*/xy_os_*.c"))
        self.assertGreaterEqual(len(found), 4)
        self.assertEqual([], guard.findings())


if __name__ == "__main__":
    unittest.main(verbosity=2)
