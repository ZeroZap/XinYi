import unittest
import tempfile
from pathlib import Path

from xy_host_tools.gui.z_serial_tabs import ZSerialTabManager
from xy_host_tools.gui.z_serial_view_model import ZSerialWindowViewModel
from xy_host_tools.serial_cli import DEFAULT_WORKSPACE
from xy_host_tools.serial_config import FilterRule
from xy_host_tools.serial_transport import MemorySerialTransport, SerialPortInfo
from xy_host_tools.serial_virtual import VirtualSerialPair, pump_virtual_pair


class ZSerialViewModelTests(unittest.TestCase):
    def test_view_model_opens_memory_transport_and_filters_rx(self):
        transport = MemorySerialTransport()
        view_model = ZSerialWindowViewModel(transport_factory=lambda _port, _baudrate: transport)

        view_model.open_port("virtual", 115200)
        payload = view_model.send_button("version")
        transport.feed_rx(b"Boot FW=0.1.0\nERROR gui timeout\n")
        lines = view_model.poll_rx()

        self.assertTrue(view_model.is_open)
        self.assertEqual(payload, b"version\r\n")
        self.assertEqual(transport.drain_tx(), b"version\r\n")
        self.assertEqual([line.text for line in lines], ["Boot FW=0.1.0", "ERROR gui timeout"])
        self.assertIn("rules=error | ERROR gui timeout", view_model.render_output_text())

        view_model.close_port()
        self.assertFalse(view_model.is_open)

    def test_view_model_can_drive_pty_virtual_serial_pair(self):
        with VirtualSerialPair.create() as pair:
            view_model = ZSerialWindowViewModel(transport_factory=lambda _port, _baudrate: pair.host_transport)
            view_model.open_port(pair.host_path, 115200)
            pair.device_transport.open()

            view_model.send_button("version")
            pump_virtual_pair(pair)
            self.assertEqual(pair.device_transport.read(128), b"version\r\n")

            pair.device_transport.write(b"ERROR gui virtual\n")
            pump_virtual_pair(pair)
            lines = view_model.poll_rx()

            self.assertEqual(len(lines), 1)
            self.assertEqual(lines[0].matched_rules, ("error",))
            self.assertIn("ERROR gui virtual", view_model.render_output_text())

    def test_view_model_requires_port_before_open(self):
        view_model = ZSerialWindowViewModel(transport_factory=lambda _port, _baudrate: MemorySerialTransport())

        with self.assertRaisesRegex(ValueError, "serial port is required"):
            view_model.open_port("")

    def test_view_model_sends_custom_text_and_clears_output(self):
        transport = MemorySerialTransport()
        view_model = ZSerialWindowViewModel(transport_factory=lambda _port, _baudrate: transport)
        view_model.open_port("virtual", 115200)

        payload = view_model.send_text("ping")
        transport.feed_rx(b"ERROR clear me\n")
        view_model.poll_rx()
        view_model.clear_output()

        self.assertEqual(payload, b"ping\r\n")
        self.assertEqual(transport.drain_tx(), b"ping\r\n")
        self.assertEqual(view_model.output_lines, [])

    def test_virtual_demo_mode_opens_sends_and_simulates_response(self):
        view_model = ZSerialWindowViewModel()

        demo = view_model.open_virtual_demo()
        payload = view_model.send_button("version")
        command, lines = view_model.simulate_virtual_response()

        self.assertTrue(view_model.is_open)
        self.assertTrue(demo.host_path.startswith("/dev/pts/"))
        self.assertTrue(demo.device_path.startswith("/dev/pts/"))
        self.assertEqual(payload, b"version\r\n")
        self.assertEqual(command, b"version\r\n")
        self.assertEqual([line.text for line in lines], ["Boot FW=0.1.0", "ERROR virtual demo timeout"])
        self.assertIn("rules=error | ERROR virtual demo timeout", view_model.render_output_text())

        view_model.close_port()
        self.assertFalse(view_model.is_open)
        self.assertIsNone(view_model.virtual_demo)

    def test_virtual_demo_response_requires_open_demo(self):
        view_model = ZSerialWindowViewModel()

        with self.assertRaisesRegex(RuntimeError, "virtual demo is not open"):
            view_model.simulate_virtual_response()

    def test_view_model_saves_and_loads_profile_selection(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            path = Path(tmpdir) / "z-serial.json"
            view_model = ZSerialWindowViewModel(transport_factory=lambda _port, _baudrate: MemorySerialTransport())
            view_model.open_port("/dev/ttyUSB9", 921600)

            view_model.save_profile(str(path))
            view_model.close_port()

            loaded = ZSerialWindowViewModel()
            windows = loaded.load_profile(str(path))

        self.assertEqual(len(windows), 1)
        self.assertEqual(loaded.selected_port, "/dev/ttyUSB9")
        self.assertEqual(loaded.baudrate, 921600)
        self.assertEqual(loaded.workspace.name, "XinYi Serial Demo")
        self.assertFalse(loaded.is_open)

    def test_profile_editor_updates_filters_before_opening_port(self):
        transport = MemorySerialTransport()
        view_model = ZSerialWindowViewModel(transport_factory=lambda _port, _baudrate: transport)

        rule = view_model.upsert_filter("warn", ("WARN",), foreground="yellow", priority=50)
        view_model.open_port("virtual", 115200)
        transport.feed_rx(b"WARN battery low\n")
        lines = view_model.poll_rx()

        self.assertEqual(rule.name, "warn")
        self.assertEqual(lines[0].matched_rules, ("warn",))
        self.assertEqual(lines[0].foreground, "yellow")

    def test_profile_editor_updates_buttons_before_opening_port(self):
        transport = MemorySerialTransport()
        view_model = ZSerialWindowViewModel(transport_factory=lambda _port, _baudrate: transport)

        button = view_model.upsert_button("ping", "Ping", "text", "ping", append_newline=True)
        view_model.open_port("virtual", 115200)
        payload = view_model.send_button("ping")

        self.assertEqual(button.label, "Ping")
        self.assertEqual(payload, b"ping\n")
        self.assertEqual(transport.drain_tx(), b"ping\n")

    def test_profile_editor_removes_filters_and_buttons(self):
        view_model = ZSerialWindowViewModel()

        self.assertTrue(view_model.remove_filter("boot"))
        self.assertTrue(view_model.remove_button("boot"))
        self.assertFalse(view_model.remove_filter("missing"))
        self.assertNotIn("boot", [rule.name for rule in view_model.filter_rows()])
        self.assertNotIn("boot", [button.name for button in view_model.button_rows()])

    def test_profile_editor_requires_closed_port(self):
        view_model = ZSerialWindowViewModel(transport_factory=lambda _port, _baudrate: MemorySerialTransport())
        view_model.open_port("virtual", 115200)

        with self.assertRaisesRegex(RuntimeError, "close serial port"):
            view_model.upsert_filter("warn", ("WARN",))

    def test_port_provider_exposes_rich_port_info(self):
        view_model = ZSerialWindowViewModel(
            port_provider=lambda: (
                SerialPortInfo("/dev/ttyUSB0", "USB UART", "hw0"),
                SerialPortInfo("/dev/ttyACM0", "CMSIS-DAP", "hw1"),
            )
        )

        infos = view_model.available_port_infos()

        self.assertEqual([port.port for port in infos], ["/dev/ttyUSB0", "/dev/ttyACM0"])
        self.assertEqual(view_model.available_ports(), ("/dev/ttyUSB0", "/dev/ttyACM0"))
        self.assertEqual(infos[0].description, "USB UART")

    def test_filter_profiles_save_overwrite_save_as_and_load(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            first_path = Path(tmpdir) / "filters-a.json"
            second_path = Path(tmpdir) / "filters-b.json"
            view_model = ZSerialWindowViewModel(transport_factory=lambda _port, _baudrate: MemorySerialTransport())

            view_model.upsert_filter("warn", ("WARN",), foreground="yellow")
            view_model.save_filter_profile_as(str(first_path), name="Warnings")
            view_model.upsert_filter("fault", ("FAULT",), background="red")
            view_model.save_filter_profile()
            view_model.save_filter_profile_as(str(second_path), name="Faults")

            loaded = ZSerialWindowViewModel(transport_factory=lambda _port, _baudrate: MemorySerialTransport())
            filters = loaded.load_filter_profile(str(first_path))
            first_exists = first_path.exists()
            second_exists = second_path.exists()

        self.assertEqual(view_model.filter_profile_path, str(second_path))
        self.assertTrue(first_exists)
        self.assertTrue(second_exists)
        self.assertEqual(loaded.filter_profile_name, "Warnings")
        self.assertEqual([rule.name for rule in filters], ["error", "boot", "warn", "fault"])

    def test_each_tab_can_load_different_filter_profile(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            warn_path = Path(tmpdir) / "warn.json"
            gps_path = Path(tmpdir) / "gps.json"
            warn_vm = ZSerialWindowViewModel()
            warn_vm.workspace = DEFAULT_WORKSPACE.__class__(
                name=DEFAULT_WORKSPACE.name,
                filters=(FilterRule("warn", ("WARN",), foreground="yellow"),),
                buttons=DEFAULT_WORKSPACE.buttons,
            )
            warn_vm.save_filter_profile_as(str(warn_path), name="Warn")
            gps_vm = ZSerialWindowViewModel()
            gps_vm.workspace = DEFAULT_WORKSPACE.__class__(
                name=DEFAULT_WORKSPACE.name,
                filters=(FilterRule("gps", ("GGA",), foreground="green"),),
                buttons=DEFAULT_WORKSPACE.buttons,
            )
            gps_vm.save_filter_profile_as(str(gps_path), name="GPS")

            first_transport = MemorySerialTransport()
            second_transport = MemorySerialTransport()
            transports = iter((first_transport, second_transport))
            manager = ZSerialTabManager(
                DEFAULT_WORKSPACE,
                transport_factory=lambda _port, _baudrate: next(transports),
            )
            first = manager.add_tab("USB0")
            second = manager.add_tab("USB1")
            first.view_model.load_filter_profile(str(warn_path))
            second.view_model.load_filter_profile(str(gps_path))
            first.view_model.open_port("virtual0", 115200)
            second.view_model.open_port("virtual1", 115200)
            first_transport.feed_rx(b"WARN low\nGGA fix\n")
            second_transport.feed_rx(b"WARN low\nGGA fix\n")
            first_lines = first.view_model.poll_rx()
            second_lines = second.view_model.poll_rx()

        self.assertEqual(first.view_model.filter_profile_name, "Warn")
        self.assertEqual(second.view_model.filter_profile_name, "GPS")
        self.assertEqual([line.matched_rules for line in first_lines], [("warn",), ()])
        self.assertEqual([line.matched_rules for line in second_lines], [(), ("gps",)])

    def test_output_lines_are_trimmed_to_configured_limit(self):
        transport = MemorySerialTransport()
        view_model = ZSerialWindowViewModel(
            transport_factory=lambda _port, _baudrate: transport,
            max_output_lines=3,
        )
        view_model.open_port("virtual", 115200)

        transport.feed_rx(b"line0\nline1\nline2\nline3\nline4\n")
        returned = view_model.poll_rx(size=128)

        self.assertEqual([line.text for line in returned], ["line0", "line1", "line2", "line3", "line4"])
        self.assertEqual([line.text for line in view_model.output_lines], ["line2", "line3", "line4"])


if __name__ == "__main__":
    unittest.main()
