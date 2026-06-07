import os
import unittest

from xy_host_tools.serial_cli import run_virtual_smoke
from xy_host_tools.serial_virtual import VirtualSerialPair, pump_virtual_pair


class VirtualSerialTests(unittest.TestCase):
    def test_virtual_pair_exposes_two_pty_paths(self):
        with VirtualSerialPair.create() as pair:
            self.assertTrue(os.path.exists(pair.host_path))
            self.assertTrue(os.path.exists(pair.device_path))
            self.assertNotEqual(pair.host_path, pair.device_path)

    def test_virtual_pair_moves_bytes_between_host_and_device(self):
        with VirtualSerialPair.create() as pair:
            pair.host_transport.open()
            pair.device_transport.open()

            pair.host_transport.write(b"version\r\n")
            self.assertGreater(pump_virtual_pair(pair), 0)
            self.assertEqual(pair.device_transport.read(64), b"version\r\n")

            pair.device_transport.write(b"OK\n")
            self.assertGreater(pump_virtual_pair(pair), 0)
            self.assertEqual(pair.host_transport.read(64), b"OK\n")

    def test_virtual_smoke_uses_service_filters(self):
        lines = run_virtual_smoke()

        self.assertTrue(lines[0].startswith("host=/dev/pts/"))
        self.assertEqual(lines[1], "device_rx=76657273696f6e0d0a")
        self.assertIn("rules=boot | Boot FW=0.1.0", lines[2])
        self.assertIn("rules=error | ERROR virtual timeout", lines[3])


if __name__ == "__main__":
    unittest.main()
