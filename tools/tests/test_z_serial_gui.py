import unittest

from xy_host_tools.gui.z_serial_app import _load_qt_widgets, render_startup_lines, run_offscreen_smoke, ZSerialMainWindow
from xy_host_tools.gui.z_serial_tabs import ZSerialTabManager
from xy_host_tools.serial_cli import DEFAULT_WORKSPACE
from xy_host_tools.serial_transport import SerialPortInfo


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
            self.assertEqual(len(widgets), 20)

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

    def test_refresh_ports_does_not_expand_window_for_long_port_descriptions(self):
        try:
            widgets = _load_qt_widgets()
        except RuntimeError as exc:
            self.assertIn("PySide6 is required", str(exc))
            return

        app = widgets[0].instance() or widgets[0]([])
        long_description = "USB Serial Device " + "very-long-description-" * 12
        manager = ZSerialTabManager(DEFAULT_WORKSPACE)
        window = ZSerialMainWindow(widgets, tab_manager=manager, restore_session=False)
        pane = window.active_pane()
        pane.view_model.port_provider = lambda: (
            SerialPortInfo("/dev/ttyACM0", long_description, "USB VID:PID=1234:5678 SER=abcdef"),
        )
        window.window.setGeometry(120, 90, 720, 480)
        before = window.window.geometry()

        pane.refresh_ports_button.click()
        app.processEvents()
        after = window.window.geometry()

        self.assertEqual(after, before)
        self.assertLessEqual(pane.port_input.maximumWidth(), 240)
        self.assertEqual(pane.port_input.itemText(0), "/dev/ttyACM0")
        self.assertIn(long_description, pane.port_input.itemData(0, widgets[16].ItemDataRole.ToolTipRole))

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
            self.assertIn("menu_framework_present=true", lines)
            self.assertIn("top_buttons_hidden=true", lines)
            self.assertIn("window_close_icon_present=true", lines)
            self.assertIn("tab_close_enabled=true", lines)
            self.assertIn("default_window_compact=true", lines)
            self.assertIn("layout_allows_smaller_resize=true", lines)
            self.assertIn("add_after_all_closed_possible=true", lines)
            self.assertIn("add_after_all_closed_works=true", lines)
            self.assertIn("add_tab_toolbar_removed=true", lines)
            self.assertIn("connection_button_toggles_to_close=true", lines)
            self.assertIn("open_button_resets_after_close=true", lines)
            self.assertIn("has_error=true", lines)
            self.assertIn("has_second_error=true", lines)
            self.assertIn("has_red=true", lines)
            self.assertIn("has_warn=true", lines)
            self.assertIn("main_log_keeps_filtered_lines=true", lines)
            self.assertIn("tx_visible_in_log=true", lines)
            self.assertIn("filter_window_contains_matches=true", lines)
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
