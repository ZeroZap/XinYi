import unittest

from xy_host_tools.serial_actions import render_button_payload
from xy_host_tools.serial_config import ActionButton


class SerialActionTests(unittest.TestCase):
    def test_text_button_renders_utf8_and_optional_newline(self):
        button = ActionButton(name="version", label="版本", mode="text", payload="version", append_newline=True)

        self.assertEqual(render_button_payload(button), b"version\n")

    def test_hex_button_ignores_spaces_and_renders_bytes(self):
        button = ActionButton(name="read", label="读", mode="hex", payload="01 03 00 00 00 02 C4 0B")

        self.assertEqual(render_button_payload(button), bytes.fromhex("010300000002C40B"))

    def test_script_button_allows_return_string(self):
        button = ActionButton(name="boot", label="Boot", mode="script", payload='return "boot\\r\\n"')

        self.assertEqual(render_button_payload(button, context={"port": "/dev/ttyUSB0"}), b"boot\r\n")

    def test_script_button_allows_context_expression(self):
        button = ActionButton(name="hello", label="Hello", mode="script", payload='f"hello {port}\\n"')

        self.assertEqual(render_button_payload(button, context={"port": "u5"}), b"hello u5\n")

    def test_script_button_rejects_imports(self):
        button = ActionButton(name="bad", label="Bad", mode="script", payload='__import__("os").system("echo bad")')

        with self.assertRaises(ValueError):
            render_button_payload(button)


if __name__ == "__main__":
    unittest.main()
