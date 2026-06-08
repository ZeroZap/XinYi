import io
import tempfile
import unittest
from contextlib import redirect_stderr, redirect_stdout
from pathlib import Path

from xy_host_tools.serial_cli import main, run_send_demo, run_stress_smoke
from xy_host_tools.serial_profile import load_workspace_profile


class SerialCliTests(unittest.TestCase):
    def test_send_demo_renders_button_payload_through_memory_transport(self):
        self.assertEqual(run_send_demo("version"), b"version\r\n")
        self.assertEqual(run_send_demo("boot"), b"boot\r\n")

    def test_sample_profile_writes_loadable_json(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            path = Path(tmpdir) / "profile.json"
            output = io.StringIO()
            with redirect_stdout(output):
                exit_code = main(["sample-profile", str(path)])
            workspace, windows = load_workspace_profile(path)

        self.assertEqual(exit_code, 0)
        self.assertIn("profile.json", output.getvalue())
        self.assertEqual(workspace.name, "XinYi Serial Demo")
        self.assertEqual(windows[0].window_id, "u5")

    def test_list_command_is_safe_on_hosts_without_serial_devices(self):
        output = io.StringIO()
        with redirect_stdout(output):
            exit_code = main(["list"])

        self.assertEqual(exit_code, 0)

    def test_gui_command_reports_missing_qt_without_traceback(self):
        try:
            import PySide6  # type: ignore[import-not-found]  # noqa: F401
        except ImportError:
            pass
        else:
            self.skipTest("PySide6 is installed; avoid starting the interactive GUI in unit tests")

        error = io.StringIO()
        with redirect_stderr(error):
            exit_code = main(["gui"])

        if exit_code == 0:
            self.assertEqual(error.getvalue(), "")
        else:
            self.assertEqual(exit_code, 1)
            self.assertIn("PySide6 is required", error.getvalue())
            self.assertNotIn("Traceback", error.getvalue())

    def test_gui_smoke_command_reports_clear_result_or_missing_qt(self):
        output = io.StringIO()
        error = io.StringIO()

        with redirect_stdout(output), redirect_stderr(error):
            exit_code = main(["gui-smoke"])

        if exit_code == 0:
            self.assertIn("window=z-serial", output.getvalue())
            self.assertIn("has_error=true", output.getvalue())
        else:
            self.assertEqual(exit_code, 1)
            self.assertIn("PySide6 is required", error.getvalue())
            self.assertNotIn("Traceback", error.getvalue())

    def test_stress_smoke_reports_bounded_retention(self):
        lines = run_stress_smoke(lines=120, retain=25, chunk_size=17)

        self.assertIn("received=120", lines)
        self.assertIn("retained=25", lines)
        self.assertIn("retain_limit=25", lines)
        self.assertIn("first_retained=stress line 95", lines)
        self.assertIn("last_retained=stress line 119", lines)
        self.assertIn("error_lines=2", lines)

    def test_stress_smoke_command_validates_arguments(self):
        output = io.StringIO()
        with redirect_stdout(output):
            exit_code = main(["stress-smoke", "--lines", "120", "--retain", "25", "--chunk-size", "17"])

        self.assertEqual(exit_code, 0)
        self.assertIn("retained=25", output.getvalue())

        error = io.StringIO()
        with redirect_stderr(error):
            exit_code = main(["stress-smoke", "--lines", "0"])

        self.assertEqual(exit_code, 2)
        self.assertIn("lines must be positive", error.getvalue())


if __name__ == "__main__":
    unittest.main()
