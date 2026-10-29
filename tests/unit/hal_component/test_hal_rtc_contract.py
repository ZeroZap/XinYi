#!/usr/bin/env python3
"""Cases for the RTC contract guard.

Each case hands the guard a small synthetic backend, so the rule under test is the only thing that can
make it pass or fail. The guard reads the real tree when run standalone; here it reads only what the
case wrote, which is why one case produces exactly one finding.
"""

from __future__ import annotations

import sys
import textwrap
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import check_hal_rtc_contract as guard  # noqa: E402

# A header that carries the two statements the guard requires.
GOOD_HEADER = """
/** alarm_id selects which alarm is used. BCD is refused when a backend cannot produce it. */
"""

# A backend that does everything the guard asks, in the smallest form that satisfies it.
CORRECT = """
xy_hal_error_t xy_hal_rtc_init(void *rtc) { return XY_HAL_OK; }
xy_hal_error_t xy_hal_rtc_deinit(void *rtc) { return XY_HAL_OK; }
xy_hal_error_t xy_hal_rtc_set_time(void *rtc, const xy_hal_rtc_time_t *t, xy_hal_rtc_format_t format)
{
    if (format != XY_HAL_RTC_FORMAT_BIN) { return XY_HAL_ERROR_NOT_SUPPORTED; }
    return XY_HAL_OK;
}
xy_hal_error_t xy_hal_rtc_get_time(void *rtc, xy_hal_rtc_time_t *t, xy_hal_rtc_format_t format)
{
    if (format != XY_HAL_RTC_FORMAT_BIN) { return XY_HAL_ERROR_NOT_SUPPORTED; }
    return XY_HAL_OK;
}
xy_hal_error_t xy_hal_rtc_set_date(void *rtc, const xy_hal_rtc_date_t *d, xy_hal_rtc_format_t format)
{
    if (format != XY_HAL_RTC_FORMAT_BIN) { return XY_HAL_ERROR_NOT_SUPPORTED; }
    return XY_HAL_OK;
}
xy_hal_error_t xy_hal_rtc_get_date(void *rtc, xy_hal_rtc_date_t *d, xy_hal_rtc_format_t format)
{
    if (format != XY_HAL_RTC_FORMAT_BIN) { return XY_HAL_ERROR_NOT_SUPPORTED; }
    return XY_HAL_OK;
}
xy_hal_error_t xy_hal_rtc_set_alarm(void *rtc, const xy_hal_rtc_alarm_t *a, char alarm_id)
{
    if (alarm_id != 'A' && alarm_id != 'B') { return XY_HAL_ERROR_INVALID_PARAM; }
    return XY_HAL_OK;
}
xy_hal_error_t xy_hal_rtc_get_alarm(void *rtc, xy_hal_rtc_alarm_t *a, char alarm_id)
{
    if (alarm_id != 'A' && alarm_id != 'B') { return XY_HAL_ERROR_INVALID_PARAM; }
    return XY_HAL_OK;
}
xy_hal_error_t xy_hal_rtc_enable_alarm(void *rtc, char alarm_id)
{
    if (alarm_id != 'A') { return XY_HAL_ERROR_NOT_SUPPORTED; }
    return XY_HAL_OK;
}
xy_hal_error_t xy_hal_rtc_disable_alarm(void *rtc, char alarm_id)
{
    if (alarm_id != 'A') { return XY_HAL_ERROR_NOT_SUPPORTED; }
    return XY_HAL_OK;
}
xy_hal_error_t xy_hal_rtc_backup_read(uint32_t index, uint32_t *value)
{
    if (index >= XY_HAL_RTC_BACKUP_REGISTER_COUNT) { return XY_HAL_ERROR_INVALID_PARAM; }
    return XY_HAL_OK;
}
xy_hal_error_t xy_hal_rtc_backup_write(uint32_t index, uint32_t value)
{
    if (index >= XY_HAL_RTC_BACKUP_REGISTER_COUNT) { return XY_HAL_ERROR_INVALID_PARAM; }
    return XY_HAL_OK;
}
"""

def real_declared() -> set[str]:
    """The public entries, taken from the real header so the cases do not drift from it."""
    return guard.declared_entries()


class AlarmIdTests(unittest.TestCase):
    def test_a_backend_that_selects_the_id_passes(self):
        problems = guard.argument_findings(CORRECT, "ok", guard.ALARM_ENTRIES, "alarm_id",
                                           guard.SELECTS_ID, guard.DISCARDS_ALARM_ID,
                                           "the alarm id")
        self.assertEqual([], problems)

    def test_discarding_the_id_while_reporting_success_is_named(self):
        broken = """
        xy_hal_error_t xy_hal_rtc_get_alarm(void *rtc, xy_hal_rtc_alarm_t *a, char alarm_id)
        {
            (void)alarm_id;
            return XY_HAL_OK;
        }
        """
        problems = guard.argument_findings(broken, "bad", ("xy_hal_rtc_get_alarm",), "alarm_id",
                                           guard.SELECTS_ID, guard.DISCARDS_ALARM_ID,
                                           "the alarm id")
        self.assertEqual(1, len(problems))
        self.assertIn("discards alarm_id", problems[0])

    def test_discarding_the_id_with_a_refusal_is_allowed(self):
        honest = """
        xy_hal_error_t xy_hal_rtc_get_alarm(void *rtc, xy_hal_rtc_alarm_t *a, char alarm_id)
        {
            (void)rtc; (void)a; (void)alarm_id;
            return XY_HAL_ERROR_NOT_SUPPORTED;
        }
        """
        problems = guard.argument_findings(honest, "honest", ("xy_hal_rtc_get_alarm",), "alarm_id",
                                           guard.SELECTS_ID, guard.DISCARDS_ALARM_ID,
                                           "the alarm id")
        self.assertEqual([], problems)

    def test_ignoring_the_id_without_even_a_cast_is_named(self):
        quiet = """
        xy_hal_error_t xy_hal_rtc_enable_alarm(void *rtc, char alarm_id)
        {
            return XY_HAL_OK;
        }
        """
        problems = guard.argument_findings(quiet, "quiet", ("xy_hal_rtc_enable_alarm",),
                                           "alarm_id", guard.SELECTS_ID, guard.DISCARDS_ALARM_ID,
                                           "the alarm id")
        self.assertEqual(1, len(problems))
        self.assertIn("does not let the alarm id change what happens", problems[0])


class FormatTests(unittest.TestCase):
    def test_a_backend_that_honours_the_format_passes(self):
        problems = guard.argument_findings(CORRECT, "ok", guard.FORMAT_ENTRIES, "format",
                                           guard.HONOURS_FORMAT, guard.DISCARDS_FORMAT,
                                           "the requested format")
        self.assertEqual([], problems)

    def test_discarding_the_format_is_named(self):
        broken = """
        xy_hal_error_t xy_hal_rtc_get_time(void *rtc, xy_hal_rtc_time_t *t, xy_hal_rtc_format_t format)
        {
            (void)format;
            return XY_HAL_OK;
        }
        """
        problems = guard.argument_findings(broken, "bad", ("xy_hal_rtc_get_time",), "format",
                                           guard.HONOURS_FORMAT, guard.DISCARDS_FORMAT,
                                           "the requested format")
        self.assertEqual(1, len(problems))
        self.assertIn("discards format", problems[0])


    def test_every_format_entry_is_actually_found(self):
        """A rule that finds no bodies reports nothing, which reads exactly like full compliance.

        This is not hypothetical: FORMAT_ENTRIES was defined twice, the second copy with a trailing
        parenthesis, so `entry + "("` built `((` and every format rule silently inspected nothing.
        """
        for entry in guard.FORMAT_ENTRIES:
            self.assertTrue(guard.braced(CORRECT, entry + "("), f"{entry} found no body")

    def test_a_helper_that_branches_on_the_format_passes(self):
        # dedent matters: local_definitions anchors ^static, and an indented fixture finds none.
        backing = textwrap.dedent("""
        static xy_hal_error_t fmt_ok(xy_hal_rtc_format_t format)
        {
            return (format == XY_HAL_RTC_FORMAT_BIN) ? XY_HAL_OK : XY_HAL_ERROR_NOT_SUPPORTED;
        }

        xy_hal_error_t xy_hal_rtc_get_time(void *rtc, xy_hal_rtc_time_t *t, xy_hal_rtc_format_t format)
        {
            xy_hal_error_t status = fmt_ok(format);
            if (status != XY_HAL_OK) {
                return status;
            }
            *t = s_time;
            return XY_HAL_OK;
        }
        """)
        problems = guard.argument_findings(backing, "ok", ("xy_hal_rtc_get_time",), "format",
                                           guard.HONOURS_FORMAT, guard.DISCARDS_FORMAT,
                                           "the requested format")
        self.assertEqual([], problems)

    def test_a_helper_that_discards_the_format_is_named(self):
        # dedent matters: local_definitions anchors ^static, and an indented fixture finds none.
        backing = textwrap.dedent("""
        static xy_hal_error_t fmt_ok(xy_hal_rtc_format_t format)
        {
            (void)format;
            return XY_HAL_OK;
        }

        xy_hal_error_t xy_hal_rtc_get_time(void *rtc, xy_hal_rtc_time_t *t, xy_hal_rtc_format_t format)
        {
            xy_hal_error_t status = fmt_ok(format);
            if (status != XY_HAL_OK) {
                return XY_HAL_ERROR_NOT_INIT;
            }
            *t = s_time;
            return XY_HAL_OK;
        }
        """)
        problems = guard.argument_findings(backing, "bad", ("xy_hal_rtc_get_time",), "format",
                                           guard.HONOURS_FORMAT, guard.DISCARDS_FORMAT,
                                           "the requested format")
        self.assertEqual(1, len(problems))
        self.assertIn("discards it", problems[0])

    def test_a_helper_that_admits_it_ignores_the_format_is_named(self):
        # dedent matters: local_definitions anchors ^static, and an indented fixture finds none.
        backing = textwrap.dedent("""
        static xy_hal_error_t fmt_ok(xy_hal_rtc_format_t format)
        {
            XY_UNUSED(format);
            return XY_HAL_OK;
        }

        xy_hal_error_t xy_hal_rtc_get_time(void *rtc, xy_hal_rtc_time_t *t, xy_hal_rtc_format_t format)
        {
            (void)fmt_ok(format);
            *t = s_time;
            return XY_HAL_OK;
        }
        """)
        problems = guard.argument_findings(backing, "quiet", ("xy_hal_rtc_get_time",), "format",
                                           guard.HONOURS_FORMAT, guard.DISCARDS_FORMAT,
                                           "the requested format")
        self.assertTrue(problems, "a helper that neither branches on nor honours the format")


class AlarmMaskTests(unittest.TestCase):
    """Our mask names the fields that must match; the vendor bits name the fields to skip."""

    def test_a_backend_that_inverts_passes(self):
        backed = textwrap.dedent("""
        xy_hal_error_t xy_hal_rtc_set_alarm(void *rtc, const xy_hal_rtc_alarm_t *alarm, char id)
        {
            uint8_t skip = xy_rtc_alarm_skip_from_match(alarm->alarm_mask, XY_HAL_RTC_ALARM_MASK_ALL);

            sAlarm.AlarmMask = 0U;
            if ((skip & XY_HAL_RTC_ALARM_MASK_HOURS) != 0U) {
                sAlarm.AlarmMask |= RTC_ALARMMASK_HOURS;
            }
            return XY_HAL_OK;
        }
        """)
        self.assertEqual([], guard.alarm_mask_findings(backed, "ok"))

    def test_passing_the_mask_straight_through_is_named(self):
        backed = textwrap.dedent("""
        xy_hal_error_t xy_hal_rtc_set_alarm(void *rtc, const xy_hal_rtc_alarm_t *alarm, char id)
        {
            sAlarm.AlarmMask = alarm->alarm_mask;
            return XY_HAL_OK;
        }
        """)
        problems = guard.alarm_mask_findings(backed, "bad")
        self.assertEqual(1, len(problems))
        self.assertIn("matches on the fields the caller asked to ignore", problems[0])

    def test_reading_the_mask_back_without_inverting_is_named(self):
        backed = textwrap.dedent("""
        xy_hal_error_t xy_hal_rtc_get_alarm(void *rtc, xy_hal_rtc_alarm_t *alarm, char id)
        {
            alarm->alarm_mask = sAlarm.AlarmMask;
            return XY_HAL_OK;
        }
        """)
        problems = guard.alarm_mask_findings(backed, "bad")
        self.assertEqual(1, len(problems))
        self.assertIn("xy_hal_rtc_get_alarm", problems[0])

    def test_a_backend_without_a_vendor_mask_is_not_judged(self):
        backed = textwrap.dedent("""
        xy_hal_error_t xy_hal_rtc_set_alarm(void *rtc, const xy_hal_rtc_alarm_t *alarm, char id)
        {
            s_stored = *alarm;
            return XY_HAL_OK;
        }
        """)
        self.assertEqual([], guard.alarm_mask_findings(backed, "stored"))


class YearSpellingTests(unittest.TestCase):
    """A two-digit vendor year register has one correct spelling, and it lives in one place."""

    def test_a_backend_using_the_shared_spelling_passes(self):
        backed = textwrap.dedent("""
        xy_hal_error_t xy_hal_rtc_get_date(void *rtc, xy_hal_rtc_date_t *date, xy_hal_rtc_format_t f)
        {
            RTC_DateTypeDef sDate = { 0 };
            int year = xy_rtc_year_decode(sDate.Year, 1);

            if (year < 0) {
                return XY_HAL_ERROR_FAIL;
            }
            date->year = (uint16_t)year;
            return XY_HAL_OK;
        }
        """)
        self.assertEqual([], guard.year_findings(backed, "ok"))

    def test_a_backend_inventing_its_own_century_is_named(self):
        backed = textwrap.dedent("""
        xy_hal_error_t xy_hal_rtc_get_date(void *rtc, xy_hal_rtc_date_t *date, xy_hal_rtc_format_t f)
        {
            RTC_DateTypeDef sDate = { 0 };
            date->year = sDate.Year + 2000;
            return XY_HAL_OK;
        }
        """)
        problems = guard.year_findings(backed, "bad")
        self.assertEqual(1, len(problems))
        self.assertIn("shared spelling", problems[0])

    def test_a_backend_without_a_calendar_register_is_not_judged(self):
        """PC refuses BCD and WCH derives the date from a counter; neither has a two-digit year."""
        backed = textwrap.dedent("""
        xy_hal_error_t xy_hal_rtc_get_date(void *rtc, xy_hal_rtc_date_t *date, xy_hal_rtc_format_t f)
        {
            date->year = (uint16_t)(1970 + s_years);
            return XY_HAL_OK;
        }
        """)
        self.assertEqual([], guard.year_findings(backed, "counter"))


class BackupTests(unittest.TestCase):
    def test_bounded_registers_pass(self):
        self.assertEqual([], guard.backup_findings(CORRECT, "ok"))

    def test_unbounded_index_is_named(self):
        broken = """
        xy_hal_error_t xy_hal_rtc_backup_read(uint32_t index, uint32_t *value)
        {
            (void)index;
            return XY_HAL_OK;
        }
        """
        problems = guard.backup_findings(broken, "bad")
        self.assertEqual(1, len(problems))
        self.assertIn("does not bound the register index", problems[0])


class CoverageTests(unittest.TestCase):
    def test_a_gain_requires_the_pin_to_move(self):
        problems = guard.coverage_findings(CORRECT, "growing", 2, real_declared())
        self.assertEqual(1, len(problems))
        self.assertIn("raise the pin", problems[0])

    def test_a_loss_is_named(self):
        problems = guard.coverage_findings(CORRECT, "shrinking", 99, real_declared())
        self.assertEqual(1, len(problems))
        self.assertIn("an entry was lost", problems[0])

    def test_helpers_do_not_count_as_coverage(self):
        source = CORRECT + """
        void xy_hal_rtc_pc_tick(void *rtc) { (void)rtc; }
        """
        with_helper = guard.defined_entries(source, real_declared())
        without = guard.defined_entries(CORRECT, real_declared())
        self.assertEqual(without, with_helper)


class HeaderTests(unittest.TestCase):
    def test_the_real_header_states_both_contracts(self):
        problems = [p for p in guard.findings() if "xy_hal_rtc.h" in p]
        self.assertEqual([], problems)

    def test_the_inventory_is_not_empty(self):
        declared = guard.declared_entries()
        self.assertGreater(len(declared), 10)


if __name__ == "__main__":
    unittest.main()
