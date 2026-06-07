import io
import tempfile
import unittest
from contextlib import redirect_stdout
from pathlib import Path

from xy_host_tools.serial_cli import main, run_send_demo
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


if __name__ == "__main__":
    unittest.main()
