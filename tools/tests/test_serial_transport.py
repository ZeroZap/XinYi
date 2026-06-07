import unittest

from xy_host_tools.serial_transport import MemorySerialTransport, list_serial_ports


class SerialTransportTests(unittest.TestCase):
    def test_memory_transport_requires_open_for_io(self):
        transport = MemorySerialTransport()

        with self.assertRaises(RuntimeError):
            transport.write(b"AT\r\n")
        with self.assertRaises(RuntimeError):
            transport.read()

    def test_memory_transport_records_tx_and_drains_rx(self):
        transport = MemorySerialTransport()
        transport.open()
        transport.feed_rx(b"hello")

        self.assertEqual(transport.write(b"version\r\n"), 9)
        self.assertEqual(transport.read(2), b"he")
        self.assertEqual(transport.read(99), b"llo")
        self.assertEqual(transport.drain_tx(), b"version\r\n")
        self.assertEqual(transport.drain_tx(), b"")

    def test_list_serial_ports_is_safe_without_pyserial_requirement(self):
        ports = list_serial_ports()

        self.assertIsInstance(ports, tuple)


if __name__ == "__main__":
    unittest.main()
