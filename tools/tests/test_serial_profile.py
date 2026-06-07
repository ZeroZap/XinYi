import tempfile
import unittest
from pathlib import Path

from xy_host_tools.serial_config import (
    ActionButton,
    FilterRule,
    SerialWindowProfile,
    SerialWorkspaceProfile,
)
from xy_host_tools.serial_profile import load_workspace_profile, save_workspace_profile, workspace_from_mapping


class SerialProfileTests(unittest.TestCase):
    def test_window_inherits_global_filters_and_buttons(self):
        workspace = SerialWorkspaceProfile(
            name="lab",
            filters=(
                FilterRule(name="error", keywords=("ERROR",), foreground="white", background="red"),
            ),
            buttons=(ActionButton(name="version", label="版本", mode="text", payload="version\r\n"),),
        )
        window = SerialWindowProfile(window_id="u5", title="U5", port="/dev/ttyUSB0")

        effective_filters = workspace.effective_filters_for(window)
        effective_buttons = workspace.effective_buttons_for(window)

        self.assertEqual([rule.name for rule in effective_filters], ["error"])
        self.assertEqual([button.name for button in effective_buttons], ["version"])

    def test_window_can_override_and_disable_global_filters(self):
        workspace = SerialWorkspaceProfile(
            name="lab",
            filters=(
                FilterRule(name="error", keywords=("ERROR",), foreground="white", background="red"),
                FilterRule(name="boot", keywords=("BOOT",), foreground="cyan"),
            ),
        )
        window = SerialWindowProfile(
            window_id="u5",
            title="U5",
            port="/dev/ttyUSB0",
            disabled_filter_names=("boot",),
            local_filters=(
                FilterRule(name="error", keywords=("FAULT",), foreground="yellow", background="black"),
                FilterRule(name="sensor", keywords=("SENSOR",), foreground="green"),
            ),
        )

        effective = workspace.effective_filters_for(window)

        self.assertEqual([rule.name for rule in effective], ["error", "sensor"])
        self.assertEqual(effective[0].keywords, ("FAULT",))
        self.assertEqual(effective[0].foreground, "yellow")
        self.assertEqual(effective[0].background, "black")

    def test_json_profile_roundtrip_preserves_nested_rules_and_buttons(self):
        workspace = SerialWorkspaceProfile(
            name="lab",
            filters=(FilterRule(name="error", keywords=("ERROR", "FAIL"), priority=100),),
            buttons=(ActionButton(name="reset", label="复位", mode="text", payload="reset\r\n"),),
        )
        window = SerialWindowProfile(
            window_id="u5",
            title="U5",
            port="/dev/ttyUSB0",
            local_filters=(FilterRule(name="boot", keywords=("Boot",), foreground="cyan"),),
            local_buttons=(ActionButton(name="version", label="版本", mode="text", payload="version\r\n"),),
        )

        with tempfile.TemporaryDirectory() as tmpdir:
            path = Path(tmpdir) / "profile.json"
            save_workspace_profile(path, workspace, windows=(window,))
            loaded_workspace, loaded_windows = load_workspace_profile(path)

        self.assertEqual(loaded_workspace.name, "lab")
        self.assertEqual(loaded_workspace.filters[0].keywords, ("ERROR", "FAIL"))
        self.assertEqual(loaded_workspace.buttons[0].payload, "reset\r\n")
        self.assertEqual(loaded_windows[0].local_filters[0].foreground, "cyan")
        self.assertEqual(loaded_windows[0].local_buttons[0].name, "version")

    def test_profile_rejects_unknown_schema(self):
        with self.assertRaises(ValueError):
            workspace_from_mapping({"schema": "bad", "name": "lab"})


if __name__ == "__main__":
    unittest.main()
