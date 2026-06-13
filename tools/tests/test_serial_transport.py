import unittest
from unittest import mock

from xy_host_tools.serial_transport import MemorySerialTransport, PySerialTransport, list_serial_ports


class FakeSerial:
    def __init__(self, *, port, baudrate, timeout, write_timeout):
        self.port = port
        self.baudrate = baudrate
        self.timeout = timeout
        self.write_timeout = write_timeout
        self.is_open = True
        self.rx = bytearray(b"OK\n")
        self.tx = bytearray()

    def open(self):
        self.is_open = True

    def close(self):
        self.is_open = False

    def write(self, data):
        self.tx.extend(data)
        return len(data)

    def read(self, size):
        data = bytes(self.rx[:size])
        del self.rx[:size]
        return data


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

    def test_pyserial_transport_uses_injected_factory(self):
        created = []

        def factory(**kwargs):
            serial = FakeSerial(**kwargs)
            created.append(serial)
            return serial

        transport = PySerialTransport(port="/dev/ttyUSB0", baudrate=9600, serial_factory=factory)
        transport.open()

        self.assertTrue(transport.is_open)
        self.assertEqual(created[0].port, "/dev/ttyUSB0")
        self.assertEqual(created[0].baudrate, 9600)
        self.assertEqual(transport.write(b"AT\r\n"), 4)
        self.assertEqual(created[0].tx, bytearray(b"AT\r\n"))
        self.assertEqual(transport.read(3), b"OK\n")
        transport.close()
        self.assertFalse(transport.is_open)

    def test_pyserial_transport_open_failure_is_actionable_without_hardware(self):
        transport = PySerialTransport(port="/dev/ttyUSB0")

        with self.assertRaises(Exception) as context:
            transport.open()

        self.assertRegex(str(context.exception), "pyserial is required|could not open serial port|No such file")

    def test_pyserial_transport_permission_error_mentions_group_hint(self):
        def factory(**_kwargs):
            raise PermissionError(13, "Permission denied", "/dev/ttyACM0")

        transport = PySerialTransport(port="/dev/ttyACM0", serial_factory=factory)

        with mock.patch("xy_host_tools.serial_transport.getpass.getuser", return_value="eugene"), mock.patch(
            "xy_host_tools.serial_transport._serial_device_group", return_value="dialout"
        ), mock.patch("xy_host_tools.serial_transport._current_user_groups", return_value=("eugene", "plugdev")), mock.patch(
            "xy_host_tools.serial_transport.os.path.exists", return_value=True
        ):
            with self.assertRaises(RuntimeError) as context:
                transport.open()

        message = str(context.exception)
        self.assertIn("device group is 'dialout'", message)
        self.assertIn("current user 'eugene' is not in the 'dialout' group", message)
        self.assertIn("sudo usermod -aG dialout eugene", message)
        self.assertIn("sudo setfacl -m u:eugene:rw /dev/ttyACM0", message)


if __name__ == "__main__":
    unittest.main()
