from __future__ import annotations

import os
from typing import Sequence

from ..serial_cli import DEFAULT_WORKSPACE
from .z_serial_rendering import lines_to_html
from .z_serial_tabs import ZSerialTab, ZSerialTabManager
from .z_serial_view_model import ZSerialWindowViewModel


def render_startup_lines() -> tuple[str, ...]:
    view_model = ZSerialWindowViewModel()
    return tuple(f"[demo] {line.as_plain_text()}" for line in _render_demo_lines(view_model))


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
        from PySide6.QtCore import QTimer  # type: ignore[import-not-found]
        from PySide6.QtWidgets import (  # type: ignore[import-not-found]
            QApplication,
            QHBoxLayout,
            QLabel,
            QLineEdit,
            QMainWindow,
            QPushButton,
            QTabWidget,
            QTextEdit,
            QVBoxLayout,
            QWidget,
        )
    except ImportError as exc:
        raise RuntimeError("PySide6 is required for z-serial GUI; install the 'PySide6' package") from exc
    return QApplication, QHBoxLayout, QLabel, QLineEdit, QMainWindow, QPushButton, QTabWidget, QTextEdit, QTimer, QVBoxLayout, QWidget


class ZSerialTabPane:
    def __init__(self, widgets, tab: ZSerialTab):
        (
            _QApplication,
            QHBoxLayout,
            QLabel,
            QLineEdit,
            _QMainWindow,
            QPushButton,
            _QTabWidget,
            QTextEdit,
            _QTimer,
            QVBoxLayout,
            QWidget,
        ) = widgets
        self.tab = tab
        self.view_model = tab.view_model
        self.widget = QWidget()

        self.port_input = QLineEdit()
        self.port_input.setPlaceholderText("/dev/ttyUSB0 or virtual PTY path")
        self.baud_input = QLineEdit(str(self.view_model.baudrate))
        self.open_button = QPushButton("打开")
        self.virtual_demo_button = QPushButton("打开虚拟演示")
        self.close_button = QPushButton("关闭")
        self.send_version_button = QPushButton("发送 version")
        self.simulate_response_button = QPushButton("模拟回包")
        self.poll_button = QPushButton("读取")
        self.output = QTextEdit()
        self.output.setReadOnly(True)
        self.output.setHtml(lines_to_html(tuple(_render_demo_lines(ZSerialWindowViewModel()))))

        top = QHBoxLayout()
        top.addWidget(QLabel("端口"))
        top.addWidget(self.port_input)
        top.addWidget(QLabel("波特率"))
        top.addWidget(self.baud_input)
        top.addWidget(self.open_button)
        top.addWidget(self.virtual_demo_button)
        top.addWidget(self.close_button)
        top.addWidget(self.send_version_button)
        top.addWidget(self.simulate_response_button)
        top.addWidget(self.poll_button)

        layout = QVBoxLayout()
        layout.addLayout(top)
        layout.addWidget(self.output)
        self.widget.setLayout(layout)
        self._connect_signals()

    def _connect_signals(self) -> None:
        self.open_button.clicked.connect(self.open_port)
        self.virtual_demo_button.clicked.connect(self.open_virtual_demo)
        self.close_button.clicked.connect(self.close_port)
        self.send_version_button.clicked.connect(self.send_version)
        self.simulate_response_button.clicked.connect(self.simulate_response)
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

    def open_virtual_demo(self) -> None:
        try:
            demo = self.view_model.open_virtual_demo()
            self.port_input.setText(demo.host_path)
            self._append_status(f"virtual demo opened host={demo.host_path} device={demo.device_path}")
        except Exception as exc:
            self._append_status(f"virtual demo failed: {exc}")

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

    def refresh_from_poll(self) -> None:
        if self.view_model.output_lines:
            self._refresh_output()

    def simulate_response(self) -> None:
        try:
            command, lines = self.view_model.simulate_virtual_response()
            self._append_status(f"device saw {command.hex() or '<empty>'}")
            if lines:
                self._refresh_output()
        except Exception as exc:
            self._append_status(f"simulate failed: {exc}")

    def _append_status(self, text: str) -> None:
        self.output.append(f"# {text}")

    def _refresh_output(self) -> None:
        self.output.setHtml(lines_to_html(self.view_model.output_lines))


class ZSerialMainWindow:
    def __init__(self, widgets, tab_manager: ZSerialTabManager | None = None):
        (
            _QApplication,
            QHBoxLayout,
            _QLabel,
            _QLineEdit,
            QMainWindow,
            QPushButton,
            QTabWidget,
            _QTextEdit,
            QTimer,
            QVBoxLayout,
            QWidget,
        ) = widgets
        self.widgets = widgets
        self.tab_manager = tab_manager or ZSerialTabManager(DEFAULT_WORKSPACE)
        self.panes: dict[str, ZSerialTabPane] = {}
        self.window = QMainWindow()
        self.window.setWindowTitle("z-serial")

        self.new_tab_button = QPushButton("新建 Tab")
        self.close_tab_button = QPushButton("关闭 Tab")
        self.tabs = QTabWidget()
        self.tabs.setTabsClosable(False)

        toolbar = QHBoxLayout()
        toolbar.addWidget(self.new_tab_button)
        toolbar.addWidget(self.close_tab_button)
        toolbar.addStretch()

        layout = QVBoxLayout()
        layout.addLayout(toolbar)
        layout.addWidget(self.tabs)
        central = QWidget()
        central.setLayout(layout)
        self.window.setCentralWidget(central)
        self.window.resize(1180, 760)

        self.poll_timer = QTimer()
        self.poll_timer.setInterval(200)
        self.poll_timer.timeout.connect(self.poll_all_tabs)
        self.poll_timer.start()
        self._connect_signals()
        self.add_tab()

    @property
    def view_model(self) -> ZSerialWindowViewModel:
        return self.active_pane().view_model

    def show(self) -> None:
        self.window.show()

    def _connect_signals(self) -> None:
        self.new_tab_button.clicked.connect(self.add_tab)
        self.close_tab_button.clicked.connect(self.close_active_tab)
        self.tabs.currentChanged.connect(self._set_active_index)

    def add_tab(self) -> ZSerialTabPane:
        tab = self.tab_manager.add_tab()
        pane = ZSerialTabPane(self.widgets, tab)
        self.panes[tab.tab_id] = pane
        self.tabs.addTab(pane.widget, tab.title)
        self.tabs.setCurrentIndex(self.tabs.count() - 1)
        return pane

    def close_active_tab(self) -> None:
        if self.tabs.count() <= 1:
            self.active_pane().close_port()
            return
        index = self.tabs.currentIndex()
        tab = self.tab_manager.close_tab(index)
        self.tabs.removeTab(index)
        self.panes.pop(tab.tab_id, None)

    def active_pane(self) -> ZSerialTabPane:
        tab = self.tab_manager.active_tab()
        return self.panes[tab.tab_id]

    def _set_active_index(self, index: int) -> None:
        if index >= 0 and index < len(self.tab_manager.tabs):
            self.tab_manager.set_active_index(index)

    def poll_all_tabs(self) -> None:
        changed = self.tab_manager.poll_all()
        for tab_id in changed:
            pane = self.panes.get(tab_id)
            if pane is not None:
                pane.refresh_from_poll()

    def open_virtual_demo(self) -> None:
        self.active_pane().open_virtual_demo()

    def send_version(self) -> None:
        self.active_pane().send_version()

    def simulate_response(self) -> None:
        self.active_pane().simulate_response()

    def close_port(self) -> None:
        self.active_pane().close_port()


def run_offscreen_smoke() -> tuple[str, ...]:
    os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")
    widgets = _load_qt_widgets()
    QApplication = widgets[0]

    app = QApplication.instance() or QApplication([])
    window = ZSerialMainWindow(widgets)
    window.open_virtual_demo()
    window.send_version()
    window.simulate_response()
    html = window.active_pane().output.toHtml()
    first_tab_count = window.tabs.count()
    second = window.add_tab()
    second.open_virtual_demo()
    second.send_version()
    second.simulate_response()
    second_html = second.output.toHtml()
    window.poll_all_tabs()
    window.close_port()
    is_open_after_close = window.view_model.is_open
    window.tab_manager.close_all()
    app.processEvents()
    return (
        f"window={window.window.windowTitle()}",
        f"tabs={first_tab_count + 1}",
        f"open={is_open_after_close}",
        f"has_error={str('ERROR virtual demo timeout' in html).lower()}",
        f"has_second_error={str('ERROR virtual demo timeout' in second_html).lower()}",
        f"has_red={str('#d70000' in html or 'red' in html).lower()}",
    )


def main(argv: Sequence[str] | None = None) -> int:
    widgets = _load_qt_widgets()
    QApplication = widgets[0]

    app = QApplication(list(argv or []))
    window = ZSerialMainWindow(widgets)
    window.show()
    return int(app.exec())


if __name__ == "__main__":
    raise SystemExit(main())
