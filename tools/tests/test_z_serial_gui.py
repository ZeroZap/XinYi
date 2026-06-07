import unittest

from xy_host_tools.gui.z_serial_app import _load_qt_widgets, render_startup_lines


class ZSerialGuiShellTests(unittest.TestCase):
    def test_startup_lines_are_rendered_from_service_backend(self):
        lines = render_startup_lines()

        self.assertEqual(len(lines), 5)
        self.assertIn("[demo]", lines[0])
        self.assertIn("rules=boot", lines[0])
        self.assertIn("ERROR uart timeout", lines[2])

    def test_qt_dependency_boundary_is_explicit(self):
        try:
            widgets = _load_qt_widgets()
        except RuntimeError as exc:
            self.assertIn("PySide6 is required", str(exc))
        else:
            self.assertEqual(len(widgets), 9)


if __name__ == "__main__":
    unittest.main()
