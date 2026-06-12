import unittest

from xy_host_tools.gui.z_serial_rendering import color_to_css, line_to_html, lines_to_html
from xy_host_tools.gui.z_serial_view_model import RenderedLine


class ZSerialRenderingTests(unittest.TestCase):
    def test_color_names_and_hex_values_convert_to_css(self):
        self.assertEqual(color_to_css("red"), "#d70000")
        self.assertEqual(color_to_css("default", "transparent"), "transparent")
        self.assertEqual(color_to_css("#123456"), "#123456")
        self.assertEqual(color_to_css("unknown"), "#d4d4d4")

    def test_default_foreground_stays_visible_with_transparent_background(self):
        html = line_to_html(RenderedLine("normal rx", "default", "default", (), direction="rx"))

        self.assertIn("color: #d4d4d4", html)
        self.assertIn("background-color: transparent", html)
        self.assertIn(">rx normal rx<", html)

    def test_line_to_html_escapes_text_and_applies_filter_colors(self):
        line = RenderedLine(
            text="ERROR <uart>",
            foreground="white",
            background="red",
            matched_rules=("error",),
        )

        html = line_to_html(line)

        self.assertIn("color: #ffffff", html)
        self.assertIn("background-color: #d70000", html)
        self.assertIn('data-rules="error"', html)
        self.assertIn('data-direction="RX"', html)
        self.assertIn("ERROR &lt;uart&gt;", html)
        self.assertNotIn("[RX]", html)

    def test_line_to_html_marks_tx_and_rx_in_main_log(self):
        tx_html = line_to_html(RenderedLine("ping", "cyan", "default", (), direction="tx"))
        unmatched_rx_html = line_to_html(RenderedLine("pong", "default", "default", (), direction="rx"))
        matched_rx_html = line_to_html(RenderedLine("ERROR", "white", "red", ("error",), direction="rx"))

        self.assertIn(">tx ping<", tx_html)
        self.assertIn(">rx pong<", unmatched_rx_html)
        self.assertIn(">rx ERROR<", matched_rx_html)
        self.assertNotIn("[RX]", matched_rx_html)

    def test_lines_to_html_joins_lines_for_qt_rich_text(self):
        html = lines_to_html(
            [
                RenderedLine("Boot FW=0.1.0", "cyan", "default", ("boot",)),
                RenderedLine("ERROR timeout", "white", "red", ("error",)),
            ]
        )

        self.assertIn("<br/>", html)
        self.assertIn("#0087af", html)
        self.assertIn("#d70000", html)


if __name__ == "__main__":
    unittest.main()
