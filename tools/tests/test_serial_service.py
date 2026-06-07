import unittest

from xy_host_tools.serial_config import ActionButton, FilterRule, SerialWindowProfile, SerialWorkspaceProfile
from xy_host_tools.serial_service import SerialWorkspaceService
from xy_host_tools.serial_transport import MemorySerialTransport


class SerialServiceTests(unittest.TestCase):
    def make_workspace(self):
        return SerialWorkspaceProfile(
            name="lab",
            filters=(
                FilterRule(name="error", keywords=("ERROR",), foreground="white", background="red", priority=100),
                FilterRule(name="noise", keywords=("debug",), action="hide"),
            ),
            buttons=(
                ActionButton(name="version", label="版本", mode="text", payload="version\r\n"),
                ActionButton(name="hello", label="Hello", mode="script", payload='f"hello {window_id}\\n"'),
            ),
        )

    def test_attach_window_can_open_and_detach_session(self):
        service = SerialWorkspaceService(self.make_workspace())
        transport = MemorySerialTransport()
        window = SerialWindowProfile(window_id="u5", title="U5", port="virtual")

        session = service.attach_window(window, transport, open_immediately=True)
        detached = service.detach_window("u5")

        self.assertIs(session, detached)
        self.assertFalse(transport.is_open)

    def test_send_button_uses_effective_profile_and_transport(self):
        service = SerialWorkspaceService(self.make_workspace())
        transport = MemorySerialTransport()
        window = SerialWindowProfile(window_id="u5", title="U5", port="virtual")
        session = service.attach_window(window, transport, open_immediately=True)

        payload = session.send_button("version")

        self.assertEqual(payload, b"version\r\n")
        self.assertEqual(session.sent_bytes, len(payload))
        self.assertEqual(transport.drain_tx(), b"version\r\n")

    def test_script_button_receives_window_context(self):
        service = SerialWorkspaceService(self.make_workspace())
        transport = MemorySerialTransport()
        window = SerialWindowProfile(window_id="gps", title="GPS", port="virtual")
        session = service.attach_window(window, transport, open_immediately=True)

        payload = session.send_button("hello")

        self.assertEqual(payload, b"hello gps\n")
        self.assertEqual(transport.drain_tx(), b"hello gps\n")

    def test_accept_rx_bytes_filters_hidden_lines_and_records_visible_lines(self):
        service = SerialWorkspaceService(self.make_workspace())
        session = service.attach_window(
            SerialWindowProfile(window_id="u5", title="U5", port="virtual"),
            MemorySerialTransport(),
            open_immediately=True,
        )

        received = session.accept_rx_bytes(b"boot ok\ndebug verbose\nERROR timeout\n")

        self.assertEqual([line.text for line in received], ["boot ok", "ERROR timeout"])
        self.assertEqual(received[0].result.matched_rules, ())
        self.assertEqual(received[1].result.matched_rules, ("error",))
        self.assertEqual(received[1].result.foreground, "white")
        self.assertEqual(len(session.received_lines), 2)

    def test_read_available_drains_transport_rx(self):
        service = SerialWorkspaceService(self.make_workspace())
        transport = MemorySerialTransport()
        session = service.attach_window(
            SerialWindowProfile(window_id="u5", title="U5", port="virtual"),
            transport,
            open_immediately=True,
        )
        transport.feed_rx(b"ERROR one\n")

        received = session.read_available()

        self.assertEqual([line.text for line in received], ["ERROR one"])
        self.assertEqual(session.last_rx, b"ERROR one\n")
        self.assertEqual(transport.read(10), b"")

    def test_duplicate_window_ids_are_rejected(self):
        service = SerialWorkspaceService(self.make_workspace())
        window = SerialWindowProfile(window_id="u5", title="U5", port="virtual")
        service.attach_window(window, MemorySerialTransport())

        with self.assertRaises(ValueError):
            service.attach_window(window, MemorySerialTransport())


if __name__ == "__main__":
    unittest.main()
