from __future__ import annotations

from typing import Sequence

from ..serial_cli import DEFAULT_WORKSPACE, _demo_lines
from ..serial_config import SerialWindowProfile
from ..serial_service import SerialWorkspaceService
from ..serial_transport import MemorySerialTransport


def render_startup_lines() -> tuple[str, ...]:
    window = SerialWindowProfile(window_id="demo", title="Demo", port="virtual")
    service = SerialWorkspaceService(DEFAULT_WORKSPACE)
    session = service.attach_window(window, MemorySerialTransport(), open_immediately=True)
    lines: list[str] = []

    for line in _demo_lines():
        for received in session.accept_rx_bytes((line + "\n").encode("utf-8")):
            result = received.result
            lines.append(
                f"[{received.window_id}] fg={result.foreground} bg={result.background} "
                f"rules={','.join(result.matched_rules) or '-'} | {received.text}"
            )
    return tuple(lines)


def _load_qt_widgets():
    try:
        from PySide6.QtWidgets import (  # type: ignore[import-not-found]
            QApplication,
            QLabel,
            QMainWindow,
            QPlainTextEdit,
            QVBoxLayout,
            QWidget,
        )
    except ImportError as exc:
        raise RuntimeError("PySide6 is required for z-serial GUI; install the 'PySide6' package") from exc
    return QApplication, QLabel, QMainWindow, QPlainTextEdit, QVBoxLayout, QWidget


def main(argv: Sequence[str] | None = None) -> int:
    QApplication, QLabel, QMainWindow, QPlainTextEdit, QVBoxLayout, QWidget = _load_qt_widgets()

    app = QApplication(list(argv or []))
    window = QMainWindow()
    window.setWindowTitle("z-serial")

    title = QLabel("z-serial - GUI shell using service/core backend")
    output = QPlainTextEdit()
    output.setReadOnly(True)
    output.setPlainText("\n".join(render_startup_lines()))

    layout = QVBoxLayout()
    layout.addWidget(title)
    layout.addWidget(output)

    central = QWidget()
    central.setLayout(layout)
    window.setCentralWidget(central)
    window.resize(960, 640)
    window.show()
    return int(app.exec())


if __name__ == "__main__":
    raise SystemExit(main())
