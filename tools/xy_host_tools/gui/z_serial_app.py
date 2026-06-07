from __future__ import annotations

from typing import Sequence

from .z_serial_view_model import ZSerialWindowViewModel


def render_startup_lines() -> tuple[str, ...]:
    view_model = ZSerialWindowViewModel()
    return tuple(
        f"[demo] {line.as_plain_text()}"
        for line in _render_demo_lines(view_model)
    )


def _render_demo_lines(view_model: ZSerialWindowViewModel):
    from ..serial_cli import _demo_lines
    from ..serial_transport import MemorySerialTransport

    memory_transport = MemorySerialTransport()
    view_model.transport_factory = lambda _port, _baudrate: memory_transport
    view_model.open_port("virtual", 115200)
    rendered = []
    for line in _demo_lines():
        memory_transport.feed_rx((line + "\n").encode("utf-8"))
        rendered.extend(view_model.poll_rx())
    view_model.close_port()
    return tuple(rendered)


def _load_qt_widgets():
    try:
        from PySide6.QtWidgets import (  # type: ignore[import-not-found]
            QApplication,
            QHBoxLayout,
            QLabel,
            QLineEdit,
            QMainWindow,
            QPushButton,
            QPlainTextEdit,
            QVBoxLayout,
            QWidget,
        )
    except ImportError as exc:
        raise RuntimeError("PySide6 is required for z-serial GUI; install the 'PySide6' package") from exc
    return QApplication, QHBoxLayout, QLabel, QLineEdit, QMainWindow, QPushButton, QPlainTextEdit, QVBoxLayout, QWidget


class ZSerialMainWindow:
    def __init__(self, widgets, view_model: ZSerialWindowViewModel | None = None):
        (
            _QApplication,
            QHBoxLayout,
            QLabel,
            QLineEdit,
            QMainWindow,
            QPushButton,
            QPlainTextEdit,
            QVBoxLayout,
            QWidget,
        ) = widgets
        self.view_model = view_model or ZSerialWindowViewModel()
        self.window = QMainWindow()
        self.window.setWindowTitle("z-serial")

        self.port_input = QLineEdit()
        self.port_input.setPlaceholderText("/dev/ttyUSB0 or virtual PTY path")
        self.baud_input = QLineEdit(str(self.view_model.baudrate))
        self.open_button = QPushButton("打开")
        self.close_button = QPushButton("关闭")
        self.send_version_button = QPushButton("发送 version")
        self.poll_button = QPushButton("读取")
        self.output = QPlainTextEdit()
        self.output.setReadOnly(True)
        self.output.setPlainText("\n".join(render_startup_lines()))

        top = QHBoxLayout()
        top.addWidget(QLabel("端口"))
        top.addWidget(self.port_input)
        top.addWidget(QLabel("波特率"))
        top.addWidget(self.baud_input)
        top.addWidget(self.open_button)
        top.addWidget(self.close_button)
        top.addWidget(self.send_version_button)
        top.addWidget(self.poll_button)

        layout = QVBoxLayout()
        layout.addLayout(top)
        layout.addWidget(self.output)

        central = QWidget()
        central.setLayout(layout)
        self.window.setCentralWidget(central)
        self.window.resize(1080, 720)
        self._connect_signals()

    def show(self) -> None:
        self.window.show()

    def _connect_signals(self) -> None:
        self.open_button.clicked.connect(self.open_port)
        self.close_button.clicked.connect(self.close_port)
        self.send_version_button.clicked.connect(self.send_version)
        self.poll_button.clicked.connect(self.poll_rx)

    def open_port(self) -> None:
        try:
            self.view_model.open_port(self.port_input.text().strip(), int(self.baud_input.text().strip()))
            self._append_status(f"opened {self.view_model.selected_port} @ {self.view_model.baudrate}")
        except Exception as exc:
            self._append_status(f"open failed: {exc}")

    def close_port(self) -> None:
        self.view_model.close_port()
        self._append_status("closed")

    def send_version(self) -> None:
        try:
            payload = self.view_model.send_button("version")
            self._append_status(f"tx {payload.hex()}")
        except Exception as exc:
            self._append_status(f"send failed: {exc}")

    def poll_rx(self) -> None:
        try:
            lines = self.view_model.poll_rx()
            if not lines:
                self._append_status("rx empty")
            else:
                self._refresh_output()
        except Exception as exc:
            self._append_status(f"read failed: {exc}")

    def _append_status(self, text: str) -> None:
        self.output.appendPlainText(f"# {text}")

    def _refresh_output(self) -> None:
        self.output.setPlainText(self.view_model.render_output_text())


def main(argv: Sequence[str] | None = None) -> int:
    widgets = _load_qt_widgets()
    QApplication = widgets[0]

    app = QApplication(list(argv or []))
    window = ZSerialMainWindow(widgets)
    window.show()
    return int(app.exec())


if __name__ == "__main__":
    raise SystemExit(main())
