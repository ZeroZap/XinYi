import unittest

from xy_host_tools.serial_config import FilterRule
from xy_host_tools.serial_filter import apply_filters


class SerialFilterTests(unittest.TestCase):
    def test_any_rule_highlights_line_with_configured_colors(self):
        rules = (FilterRule(name="error", keywords=("ERROR", "FAIL"), foreground="white", background="red"),)

        result = apply_filters("sensor ERROR timeout", rules)

        self.assertTrue(result.visible)
        self.assertEqual(result.foreground, "white")
        self.assertEqual(result.background, "red")
        self.assertEqual(result.matched_rules, ("error",))

    def test_all_rule_requires_every_keyword_case_insensitive(self):
        rules = (FilterRule(name="uart_timeout", keywords=("UART", "timeout"), match="all", foreground="yellow"),)

        self.assertEqual(apply_filters("uart rx timeout", rules).matched_rules, ("uart_timeout",))
        self.assertEqual(apply_filters("uart rx ok", rules).matched_rules, ())

    def test_sequence_rule_requires_keywords_in_order(self):
        rules = (FilterRule(name="boot_ok", keywords=("boot", "ok"), match="sequence", foreground="cyan"),)

        self.assertEqual(apply_filters("Boot stage 1 OK", rules).matched_rules, ("boot_ok",))
        self.assertEqual(apply_filters("OK before boot", rules).matched_rules, ())

    def test_regex_rule_matches_pattern(self):
        rules = (FilterRule(name="temperature", keywords=(r"temp=\d+\.\d+",), match="regex", foreground="green"),)

        self.assertEqual(apply_filters("temp=25.6", rules).matched_rules, ("temperature",))
        self.assertEqual(apply_filters("temp=bad", rules).matched_rules, ())

    def test_hide_action_wins_over_highlight(self):
        rules = (
            FilterRule(name="error", keywords=("ERROR",), foreground="white", background="red", priority=10),
            FilterRule(name="noise", keywords=("ERROR",), action="hide", priority=1),
        )

        result = apply_filters("ERROR noisy line", rules)

        self.assertFalse(result.visible)
        self.assertEqual(result.matched_rules, ("error", "noise"))

    def test_highest_priority_highlight_controls_color(self):
        rules = (
            FilterRule(name="low", keywords=("FAULT",), foreground="blue", priority=1),
            FilterRule(name="high", keywords=("FAULT",), foreground="white", background="red", priority=100),
        )

        result = apply_filters("FAULT", rules)

        self.assertTrue(result.visible)
        self.assertEqual(result.foreground, "white")
        self.assertEqual(result.background, "red")
        self.assertEqual(result.matched_rules, ("low", "high"))


if __name__ == "__main__":
    unittest.main()
