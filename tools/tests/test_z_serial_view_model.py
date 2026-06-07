import unittest

from xy_host_tools.gui.z_serial_view_model import ZSerialWindowViewModel
from xy_host_tools.serial_transport import MemorySerialTransport
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


if __name__ == "__main__":
    unittest.main()
