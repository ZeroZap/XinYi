import unittest

from xy_host_tools.gui.z_serial_app import _load_qt_widgets, render_startup_lines, run_offscreen_smoke


class ZSerialGuiShellTests(unittest.TestCase):
    def test_startup_lines_do_not_inject_demo_faults(self):
        lines = render_startup_lines()

        self.assertEqual(lines, ())

    def test_qt_dependency_boundary_is_explicit(self):
        try:
            widgets = _load_qt_widgets()
        except RuntimeError as exc:
            self.assertIn("PySide6 is required", str(exc))
        else:
            self.assertEqual(len(widgets), 15)

    def test_offscreen_smoke_runs_or_reports_missing_qt(self):
        try:
            lines = run_offscreen_smoke()
        except RuntimeError as exc:
            self.assertIn("PySide6 is required", str(exc))
        else:
            self.assertIn("window=z-serial", lines)
            self.assertIn("tabs=2", lines)
            self.assertIn("has_error=true", lines)
            self.assertIn("has_second_error=true", lines)
            self.assertIn("has_red=true", lines)
            self.assertIn("has_warn=true", lines)
            self.assertIn("has_editor_button=true", lines)
            self.assertIn("custom_tx=true", lines)
            self.assertIn("custom_no_crlf=true", lines)
            self.assertIn("status_outside_rx=true", lines)
            self.assertIn("cleared=true", lines)


if __name__ == "__main__":
    unittest.main()
