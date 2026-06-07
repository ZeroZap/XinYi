import unittest

from xy_host_tools.gui.z_serial_tabs import ZSerialTabManager
from xy_host_tools.serial_cli import DEFAULT_WORKSPACE
from xy_host_tools.serial_transport import MemorySerialTransport


class ZSerialTabsTests(unittest.TestCase):
    def test_tab_manager_adds_and_switches_tabs(self):
        manager = ZSerialTabManager(DEFAULT_WORKSPACE)

        first = manager.add_tab("USB0")
        second = manager.add_tab("USB1")
        manager.set_active_index(0)

        self.assertEqual(first.tab_id, "tab-1")
        self.assertEqual(second.tab_id, "tab-2")
        self.assertEqual(manager.active_tab().title, "USB0")
        self.assertEqual(manager.active_index, 0)

    def test_tab_manager_closes_tab_and_closes_port(self):
        transport = MemorySerialTransport()
        manager = ZSerialTabManager(DEFAULT_WORKSPACE, transport_factory=lambda _port, _baudrate: transport)
        tab = manager.add_tab()
        tab.view_model.open_port("virtual", 115200)

        closed = manager.close_tab(0)

        self.assertEqual(closed.tab_id, tab.tab_id)
        self.assertFalse(transport.is_open)
        self.assertEqual(manager.tabs, [])

    def test_tab_manager_polls_all_open_tabs(self):
        transports = [MemorySerialTransport(), MemorySerialTransport()]
        manager = ZSerialTabManager(DEFAULT_WORKSPACE, transport_factory=lambda port, _baudrate: transports[int(port[-1])])
        first = manager.add_tab("A")
        second = manager.add_tab("B")
        first.view_model.open_port("virtual0", 115200)
        second.view_model.open_port("virtual1", 115200)
        transports[0].feed_rx(b"Boot FW=0.1.0\n")
        transports[1].feed_rx(b"ERROR tab timeout\n")

        rendered = manager.poll_all()

        self.assertEqual(rendered[first.tab_id][0].matched_rules, ("boot",))
        self.assertEqual(rendered[second.tab_id][0].matched_rules, ("error",))
        self.assertIn("ERROR tab timeout", second.view_model.render_output_text())

    def test_close_all_cleans_open_tabs(self):
        transport = MemorySerialTransport()
        manager = ZSerialTabManager(DEFAULT_WORKSPACE, transport_factory=lambda _port, _baudrate: transport)
        tab = manager.add_tab()
        tab.view_model.open_port("virtual", 115200)

        manager.close_all()

        self.assertFalse(transport.is_open)
        self.assertEqual(manager.tabs, [])


if __name__ == "__main__":
    unittest.main()
