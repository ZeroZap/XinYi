import unittest

from xy_host_tools.gui.z_serial_app import _load_qt_widgets, render_startup_lines, run_offscreen_smoke, ZSerialMainWindow
from xy_host_tools.gui.z_serial_tabs import ZSerialTabManager
from xy_host_tools.serial_cli import DEFAULT_WORKSPACE


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
            self.assertEqual(len(widgets), 17)

    def test_open_button_syncs_failure_to_main_status(self):
        try:
            widgets = _load_qt_widgets()
        except RuntimeError as exc:
            self.assertIn("PySide6 is required", str(exc))
            return

        app = widgets[0].instance() or widgets[0]([])

        def factory(_port, _baudrate):
            raise PermissionError(13, "Permission denied", "/dev/ttyACM0")

        manager = ZSerialTabManager(DEFAULT_WORKSPACE, transport_factory=factory)
        window = ZSerialMainWindow(widgets, tab_manager=manager, restore_session=False)
        pane = window.active_pane()
        pane.port_input.setEditText("/dev/ttyACM0")
        pane.open_button.click()
        app.processEvents()

        self.assertIn("open failed", window.status_line.text())
        self.assertIn("Permission denied", window.status_line.text())

    def test_offscreen_smoke_runs_or_reports_missing_qt(self):
        try:
            lines = run_offscreen_smoke()
        except RuntimeError as exc:
            self.assertIn("PySide6 is required", str(exc))
        else:
            self.assertIn("window=z-serial", lines)
            self.assertIn("tabs=2", lines)
            self.assertIn("plus_tab_adjacent=true", lines)
            self.assertIn("plus_tab_moves_right=true", lines)
            self.assertIn("add_tab_toolbar_removed=true", lines)
            self.assertIn("has_error=true", lines)
            self.assertIn("has_second_error=true", lines)
            self.assertIn("has_red=true", lines)
            self.assertIn("has_warn=true", lines)
            self.assertIn("log_panel_visible=true", lines)
            self.assertIn("zed_sidebar_layout=true", lines)
            self.assertIn("zed_bottom_filter=true", lines)
            self.assertIn("filter_summary_visible=true", lines)
            self.assertIn("search_finds_timeout=true", lines)
            self.assertIn("session_restored=true", lines)
            self.assertIn("has_editor_button=true", lines)
            self.assertIn("saved_filter_exists=true", lines)
            self.assertIn("second_filter_path=true", lines)
            self.assertIn("tab_filters_independent=true", lines)
            self.assertIn("custom_tx=true", lines)
            self.assertIn("custom_no_crlf=true", lines)
            self.assertIn("status_outside_rx=true", lines)
            self.assertIn("cleared=true", lines)


if __name__ == "__main__":
    unittest.main()
