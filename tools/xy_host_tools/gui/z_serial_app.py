from __future__ import annotations

import os
import tempfile
from pathlib import Path
from typing import Sequence

from ..serial_cli import DEFAULT_WORKSPACE
from ..serial_transport import SerialPortInfo
from .z_serial_rendering import lines_to_html
from .z_serial_tabs import ZSerialTab, ZSerialTabManager
from .z_serial_view_model import ZSerialWindowViewModel


def render_startup_lines() -> tuple[str, ...]:
    return ()


def _load_qt_widgets():
    try:
        from PySide6.QtCore import Qt, QTimer  # type: ignore[import-not-found]
        from PySide6.QtWidgets import (  # type: ignore[import-not-found]
            QApplication,
            QFileDialog,
            QCheckBox,
            QComboBox,
            QHBoxLayout,
            QLabel,
            QLineEdit,
            QMainWindow,
            QPushButton,
            QSizePolicy,
            QSplitter,
            QTabWidget,
            QTextEdit,
            QVBoxLayout,
            QWidget,
        )
    except ImportError as exc:
        raise RuntimeError("PySide6 is required for z-serial GUI; install the 'PySide6' package") from exc
    return (
        QApplication,
        QFileDialog,
        QCheckBox,
        QComboBox,
        QHBoxLayout,
        QLabel,
        QLineEdit,
        QMainWindow,
        QPushButton,
        QSizePolicy,
        QSplitter,
        QTabWidget,
        QTextEdit,
        Qt,
        QTimer,
        QVBoxLayout,
        QWidget,
    )


class ZSerialTabPane:
    def __init__(self, widgets, tab: ZSerialTab):
        (
            _QApplication,
            _QFileDialog,
            QCheckBox,
            QComboBox,
            QHBoxLayout,
            QLabel,
            QLineEdit,
            _QMainWindow,
            QPushButton,
            QSizePolicy,
            QSplitter,
            _QTabWidget,
            QTextEdit,
            Qt,
            _QTimer,
            QVBoxLayout,
            QWidget,
        ) = widgets
        self.tab = tab
        self.view_model = tab.view_model
        self.widget = QWidget()

        self.port_input = QComboBox()
        self.port_input.setEditable(True)
        self.port_input.setMinimumWidth(220)
        self.port_input.setSizeAdjustPolicy(QComboBox.SizeAdjustPolicy.AdjustToMinimumContentsLengthWithIcon)
        self.port_input.setMinimumContentsLength(28)
        self.port_input.setSizePolicy(QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Fixed)
        self.port_input.view().setMinimumWidth(720)
        self.port_input.lineEdit().setPlaceholderText("/dev/ttyACM0 or /dev/ttyUSB0")
        self.baud_input = QLineEdit(str(self.view_model.baudrate))
        self.refresh_ports_button = QPushButton("刷新端口")
        self.open_button = QPushButton("打开")
        self.virtual_demo_button = QPushButton("打开虚拟演示")
        self.close_button = QPushButton("关闭")
        self.send_version_button = QPushButton("发送 version")
        self.simulate_response_button = QPushButton("模拟回包")
        self.poll_button = QPushButton("读取")
        self.clear_button = QPushButton("清屏")
        self.send_input = QLineEdit()
        self.send_input.setPlaceholderText("输入自定义命令，回车或点击发送")
        self.append_crlf_checkbox = QCheckBox("追加 CRLF")
        self.append_crlf_checkbox.setChecked(False)
        self.append_crlf_checkbox.setToolTip("发送文本后追加 \\r\\n；默认关闭，避免设备收到额外换行符")
        self.send_text_button = QPushButton("发送")
        self.output = QTextEdit()
        self.output.setReadOnly(True)
        self.output.setHtml(lines_to_html(()))
        self.filter_output = QTextEdit()
        self.filter_output.setReadOnly(True)
        self.filter_output.setPlaceholderText("滤波命中内容")
        self.filter_output.setHtml(lines_to_html(()))
        self.last_status = "ready"

        connection_row = QVBoxLayout()
        connection_row.addWidget(QLabel("端口"))
        connection_row.addWidget(self.port_input)
        connection_row.addWidget(QLabel("波特率"))
        connection_row.addWidget(self.baud_input)
        connection_row.addWidget(self.refresh_ports_button)
        connection_row.addWidget(self.open_button)
        connection_row.addWidget(self.close_button)

        action_row = QVBoxLayout()
        action_row.addWidget(QLabel("操作"))
        action_row.addWidget(self.virtual_demo_button)
        action_row.addWidget(self.send_version_button)
        action_row.addWidget(self.simulate_response_button)
        action_row.addWidget(self.poll_button)
        action_row.addWidget(self.clear_button)
        action_row.addStretch(1)

        send_row = QVBoxLayout()
        send_row.addWidget(QLabel("发送"))
        send_row.addWidget(self.send_input)
        send_row.addWidget(self.append_crlf_checkbox)
        send_row.addWidget(self.send_text_button)

        settings_panel = QWidget()
        settings_panel.setObjectName("zserial_settings_panel")
        settings_panel.setMinimumWidth(260)
        settings_panel.setMaximumWidth(360)
        settings_layout = QVBoxLayout()
        settings_layout.addLayout(connection_row)
        settings_layout.addSpacing(12)
        settings_layout.addLayout(send_row)
        settings_layout.addSpacing(12)
        settings_layout.addLayout(action_row)
        settings_panel.setLayout(settings_layout)

        log_panel = QWidget()
        log_layout = QVBoxLayout()
        log_layout.addWidget(QLabel("Log"))
        log_layout.addWidget(self.output)
        log_panel.setLayout(log_layout)

        filter_panel = QWidget()
        filter_layout = QVBoxLayout()
        filter_layout.addWidget(QLabel("滤波窗口"))
        filter_layout.addWidget(self.filter_output)
        filter_panel.setLayout(filter_layout)

        self.main_splitter = QSplitter(Qt.Orientation.Horizontal)
        self.main_splitter.setObjectName("zserial_main_splitter")
        self.content_splitter = QSplitter(Qt.Orientation.Vertical)
        self.content_splitter.setObjectName("zserial_content_splitter")
        self.content_splitter.addWidget(log_panel)
        self.content_splitter.addWidget(filter_panel)
        self.content_splitter.setSizes([620, 180])
        self.main_splitter.addWidget(settings_panel)
        self.main_splitter.addWidget(self.content_splitter)
        self.main_splitter.setSizes([300, 980])

        layout = QVBoxLayout()
        layout.addWidget(self.main_splitter)
        self.widget.setLayout(layout)
        self._connect_signals()

    def _connect_signals(self) -> None:
        self.refresh_ports_button.clicked.connect(self.refresh_ports)
        self.open_button.clicked.connect(self.open_port)
        self.virtual_demo_button.clicked.connect(self.open_virtual_demo)
        self.close_button.clicked.connect(self.close_port)
        self.send_version_button.clicked.connect(self.send_version)
        self.simulate_response_button.clicked.connect(self.simulate_response)
        self.poll_button.clicked.connect(self.poll_rx)
        self.clear_button.clicked.connect(self.clear_output)
        self.send_text_button.clicked.connect(self.send_text)
        self.send_input.returnPressed.connect(self.send_text)

    def refresh_ports(self) -> None:
        ports = self.view_model.available_port_infos()
        current_port = self.port_input.currentText().strip()
        self.port_input.clear()
        for port in ports:
            label = self._port_label(port)
            index = self.port_input.count()
            self.port_input.addItem(label, port.port)
            item = self.port_input.model().item(index)
            if item is not None:
                item.setToolTip(self._port_tooltip(port))
        if current_port:
            self.port_input.setEditText(current_port)
        elif ports:
            self.port_input.setCurrentIndex(0)
        if ports:
            self._append_status("ports " + ", ".join(port.port for port in ports))
        else:
            self._append_status("no serial ports found")

    def selected_port_text(self) -> str:
        data = self.port_input.currentData()
        text = self.port_input.currentText().strip()
        if data and text and text.startswith(str(data)):
            return str(data)
        return text

    @staticmethod
    def _port_label(port: SerialPortInfo) -> str:
        parts = [port.port]
        if port.description:
            parts.append(port.description)
        if port.hwid:
            parts.append(port.hwid)
        return "  |  ".join(parts)

    @staticmethod
    def _port_tooltip(port: SerialPortInfo) -> str:
        lines = [f"Port: {port.port}"]
        if port.description:
            lines.append(f"Description: {port.description}")
        if port.hwid:
            lines.append(f"HWID: {port.hwid}")
        return "\n".join(lines)

    def open_port(self) -> None:
        try:
            self.view_model.open_port(self.selected_port_text(), int(self.baud_input.text().strip()))
            self._append_status(f"opened {self.view_model.selected_port} @ {self.view_model.baudrate}")
        except Exception as exc:
            self._append_status(f"open failed: {exc}")

    def close_port(self) -> None:
        self.view_model.close_port()
        self._append_status("closed")

    def open_virtual_demo(self) -> None:
        try:
            demo = self.view_model.open_virtual_demo()
            self.port_input.setEditText(demo.host_path)
            self._append_status(f"virtual demo opened host={demo.host_path} device={demo.device_path}")
        except Exception as exc:
            self._append_status(f"virtual demo failed: {exc}")

    def send_version(self) -> None:
        try:
            payload = self.view_model.send_button("version")
            self._append_status(f"tx {payload.hex()}")
        except Exception as exc:
            self._append_status(f"send failed: {exc}")

    def send_text(self) -> None:
        text = self.send_input.text()
        if not text:
            self._append_status("send skipped: empty input")
            return
        try:
            payload = self.view_model.send_text(text, append_newline=self.append_crlf_checkbox.isChecked())
            self.send_input.clear()
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
                self._append_status(f"rx {len(lines)} line(s)")
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

    def clear_output(self) -> None:
        self.view_model.clear_output()
        self.output.clear()
        self.filter_output.clear()
        self._append_status("cleared")

    def apply_profile_fields(self) -> None:
        self.port_input.setEditText(self.view_model.selected_port)
        self.baud_input.setText(str(self.view_model.baudrate))
        self._refresh_output()

    def _append_status(self, text: str) -> None:
        self.last_status = text

    def _refresh_output(self) -> None:
        self.output.setHtml(lines_to_html(self.view_model.output_lines))
        self.filter_output.setHtml(lines_to_html(tuple(line for line in self.view_model.output_lines if line.matched_rules)))
        self._scroll_to_bottom()

    def _scroll_to_bottom(self) -> None:
        scrollbar = self.output.verticalScrollBar()
        scrollbar.setValue(scrollbar.maximum())
        filter_scrollbar = self.filter_output.verticalScrollBar()
        filter_scrollbar.setValue(filter_scrollbar.maximum())


class ZSerialMainWindow:
    def __init__(self, widgets, tab_manager: ZSerialTabManager | None = None):
        (
            _QApplication,
            QFileDialog,
            _QCheckBox,
            _QComboBox,
            QHBoxLayout,
            _QLabel,
            _QLineEdit,
            QMainWindow,
            QPushButton,
            _QSizePolicy,
            _QSplitter,
            QTabWidget,
            _QTextEdit,
            _Qt,
            QTimer,
            QVBoxLayout,
            QWidget,
        ) = widgets
        self.widgets = widgets
        self.QFileDialog = QFileDialog
        self.tab_manager = tab_manager or ZSerialTabManager(DEFAULT_WORKSPACE)
        self.panes: dict[str, ZSerialTabPane] = {}
        self.window = QMainWindow()
        self.window.setWindowTitle("z-serial")

        self.add_tab_button = QPushButton("➕")
        self.add_tab_button.setToolTip("新增串口 Tab")
        self.add_tab_button.setFixedWidth(36)
        self.close_tab_button = QPushButton("关闭 Tab")
        self.load_profile_button = QPushButton("打开 Profile")
        self.save_profile_button = QPushButton("保存 Profile")
        self.load_filters_button = QPushButton("打开滤波器")
        self.save_filters_button = QPushButton("覆盖滤波器")
        self.save_filters_as_button = QPushButton("滤波器另存为")
        self.edit_filter_button = QPushButton("编辑过滤器")
        self.edit_button_button = QPushButton("编辑按钮")
        self.tabs = QTabWidget()
        self.tabs.setTabsClosable(False)
        self.tabs.setCornerWidget(self.add_tab_button)
        self.status_line = QPushButton("状态: ready")
        self.status_line.setEnabled(False)

        toolbar = QHBoxLayout()
        toolbar.addWidget(self.close_tab_button)
        toolbar.addWidget(self.load_profile_button)
        toolbar.addWidget(self.save_profile_button)
        toolbar.addWidget(self.load_filters_button)
        toolbar.addWidget(self.save_filters_button)
        toolbar.addWidget(self.save_filters_as_button)
        toolbar.addWidget(self.edit_filter_button)
        toolbar.addWidget(self.edit_button_button)
        toolbar.addStretch()

        layout = QVBoxLayout()
        layout.addLayout(toolbar)
        layout.addWidget(self.tabs)
        layout.addWidget(self.status_line)
        central = QWidget()
        central.setLayout(layout)
        self.window.setCentralWidget(central)
        self.window.resize(1280, 800)

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
        self.add_tab_button.clicked.connect(self.add_tab)
        self.close_tab_button.clicked.connect(self.close_active_tab)
        self.load_profile_button.clicked.connect(self.load_profile_dialog)
        self.save_profile_button.clicked.connect(self.save_profile_dialog)
        self.load_filters_button.clicked.connect(self.load_filter_profile_dialog)
        self.save_filters_button.clicked.connect(self.save_filter_profile)
        self.save_filters_as_button.clicked.connect(self.save_filter_profile_as_dialog)
        self.edit_filter_button.clicked.connect(self.edit_filter_dialog)
        self.edit_button_button.clicked.connect(self.edit_button_dialog)
        self.tabs.currentChanged.connect(self._set_active_index)

    def add_tab(self) -> ZSerialTabPane:
        tab = self.tab_manager.add_tab()
        pane = ZSerialTabPane(self.widgets, tab)
        self.panes[tab.tab_id] = pane
        self.tabs.addTab(pane.widget, tab.title)
        self.tabs.setCurrentIndex(self.tabs.count() - 1)
        self._set_status(f"added {tab.title}")
        return pane

    def close_active_tab(self) -> None:
        if self.tabs.count() <= 1:
            self.active_pane().close_port()
            self._set_status("closed active port")
            return
        index = self.tabs.currentIndex()
        tab = self.tab_manager.close_tab(index)
        self.tabs.removeTab(index)
        self.panes.pop(tab.tab_id, None)
        self._set_status(f"closed {tab.title}")

    def active_pane(self) -> ZSerialTabPane:
        tab = self.tab_manager.active_tab()
        return self.panes[tab.tab_id]

    def _set_active_index(self, index: int) -> None:
        if index >= 0 and index < len(self.tab_manager.tabs):
            self.tab_manager.set_active_index(index)
            self._set_status(f"active {self.tab_manager.active_tab().title}")

    def poll_all_tabs(self) -> None:
        changed = self.tab_manager.poll_all()
        for tab_id in changed:
            pane = self.panes.get(tab_id)
            if pane is not None:
                pane.refresh_from_poll()
        if changed:
            self._set_status(f"rx updated {len(changed)} tab(s)")

    def open_virtual_demo(self) -> None:
        self.active_pane().open_virtual_demo()
        self._set_status(self.active_pane().last_status)

    def send_version(self) -> None:
        self.active_pane().send_version()
        self._set_status(self.active_pane().last_status)

    def send_text(self, text: str) -> None:
        pane = self.active_pane()
        pane.send_input.setText(text)
        pane.send_text()
        self._set_status(pane.last_status)

    def simulate_response(self) -> None:
        self.active_pane().simulate_response()
        self._set_status(self.active_pane().last_status)

    def close_port(self) -> None:
        self.active_pane().close_port()
        self._set_status(self.active_pane().last_status)

    def clear_output(self) -> None:
        self.active_pane().clear_output()
        self._set_status(self.active_pane().last_status)

    def save_profile_dialog(self) -> None:
        path, _selected = self.QFileDialog.getSaveFileName(self.window, "保存 z-serial Profile", "z-serial-profile.json", "JSON (*.json)")
        if path:
            self.save_profile(path)

    def load_profile_dialog(self) -> None:
        path, _selected = self.QFileDialog.getOpenFileName(self.window, "打开 z-serial Profile", "", "JSON (*.json)")
        if path:
            self.load_profile(path)

    def load_filter_profile_dialog(self) -> None:
        path, _selected = self.QFileDialog.getOpenFileName(self.window, "打开滤波器设定", "", "JSON (*.json)")
        if path:
            self.load_filter_profile(path)

    def save_filter_profile_as_dialog(self) -> None:
        path, _selected = self.QFileDialog.getSaveFileName(self.window, "滤波器另存为", "z-serial-filters.json", "JSON (*.json)")
        if path:
            self.save_filter_profile_as(path)

    def save_profile(self, path: str) -> None:
        self.view_model.save_profile(path)
        self._set_status(f"profile saved {path}")

    def load_filter_profile(self, path: str) -> None:
        filters = self.view_model.load_filter_profile(path)
        self._set_status(f"filters loaded {path} rules={len(filters)}")

    def save_filter_profile(self) -> None:
        try:
            path = self.view_model.save_filter_profile()
            self._set_status(f"filters saved {path}")
        except ValueError:
            self.save_filter_profile_as_dialog()

    def save_filter_profile_as(self, path: str) -> None:
        saved_path = self.view_model.save_filter_profile_as(path)
        self._set_status(f"filters saved {saved_path}")

    def load_profile(self, path: str) -> None:
        windows = self.view_model.load_profile(path)
        self.active_pane().apply_profile_fields()
        self._set_status(f"profile loaded {path} windows={len(windows)}")

    def add_filter(
        self,
        name: str,
        keywords: str,
        foreground: str = "yellow",
        background: str = "default",
        match: str = "any",
    ) -> None:
        keyword_tuple = tuple(part.strip() for part in keywords.replace(";", ",").split(","))
        rule = self.view_model.upsert_filter(name, keyword_tuple, match=match, foreground=foreground, background=background)
        self._set_status(f"filter saved {rule.name}")

    def add_button(
        self,
        name: str,
        label: str,
        mode: str,
        payload: str,
        append_newline: bool = False,
    ) -> None:
        button = self.view_model.upsert_button(name, label, mode, payload, append_newline=append_newline)
        self._set_status(f"button saved {button.name}")

    def edit_filter_dialog(self) -> None:
        try:
            from PySide6.QtWidgets import QDialog, QDialogButtonBox, QFormLayout, QLineEdit

            dialog = QDialog(self.window)
            dialog.setWindowTitle("编辑过滤器")
            name_input = QLineEdit("warn")
            keywords_input = QLineEdit("WARN,WARNING")
            foreground_input = QLineEdit("yellow")
            background_input = QLineEdit("default")
            form = QFormLayout()
            form.addRow("名称", name_input)
            form.addRow("关键词(逗号分隔)", keywords_input)
            form.addRow("前景色", foreground_input)
            form.addRow("背景色", background_input)
            buttons = QDialogButtonBox(QDialogButtonBox.StandardButton.Ok | QDialogButtonBox.StandardButton.Cancel)
            buttons.accepted.connect(dialog.accept)
            buttons.rejected.connect(dialog.reject)
            form.addWidget(buttons)
            dialog.setLayout(form)
            if dialog.exec() == QDialog.DialogCode.Accepted:
                self.add_filter(
                    name_input.text(),
                    keywords_input.text(),
                    foreground_input.text(),
                    background_input.text(),
                )
        except Exception as exc:
            self._set_status(f"filter edit failed: {exc}")

    def edit_button_dialog(self) -> None:
        try:
            from PySide6.QtWidgets import QCheckBox, QComboBox, QDialog, QDialogButtonBox, QFormLayout, QLineEdit

            dialog = QDialog(self.window)
            dialog.setWindowTitle("编辑按钮")
            name_input = QLineEdit("ping")
            label_input = QLineEdit("Ping")
            mode_input = QComboBox()
            mode_input.addItems(["text", "hex", "script"])
            payload_input = QLineEdit("ping")
            newline_input = QCheckBox("追加换行")
            form = QFormLayout()
            form.addRow("名称", name_input)
            form.addRow("标签", label_input)
            form.addRow("模式", mode_input)
            form.addRow("Payload", payload_input)
            form.addRow("", newline_input)
            buttons = QDialogButtonBox(QDialogButtonBox.StandardButton.Ok | QDialogButtonBox.StandardButton.Cancel)
            buttons.accepted.connect(dialog.accept)
            buttons.rejected.connect(dialog.reject)
            form.addWidget(buttons)
            dialog.setLayout(form)
            if dialog.exec() == QDialog.DialogCode.Accepted:
                self.add_button(
                    name_input.text(),
                    label_input.text(),
                    mode_input.currentText(),
                    payload_input.text(),
                    newline_input.isChecked(),
                )
        except Exception as exc:
            self._set_status(f"button edit failed: {exc}")

    def _set_status(self, text: str) -> None:
        self.status_line.setText(f"状态: {text}")


def run_offscreen_smoke() -> tuple[str, ...]:
    os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")
    widgets = _load_qt_widgets()
    QApplication = widgets[0]

    app = QApplication.instance() or QApplication([])
    window = ZSerialMainWindow(widgets)
    window.add_filter("warn", "WARN", foreground="yellow")
    window.add_button("ping_btn", "Ping", "text", "ping", append_newline=True)
    add_tab_corner = window.tabs.cornerWidget() is window.add_tab_button and window.add_tab_button.text() == "➕"
    add_tab_toolbar_removed = getattr(window, "new_tab_button", None) is None
    window.open_virtual_demo()
    window.view_model.send_button("ping_btn")
    window.send_version()
    window.send_text("ping")
    demo = window.active_pane().view_model.virtual_demo
    if demo is None:
        raise RuntimeError("virtual demo missing during GUI smoke")
    custom_payload = demo.read_device_command()
    custom_no_crlf = custom_payload.endswith(b"ping") and not custom_payload.endswith(b"ping\r\n")
    window.simulate_response()
    window.active_pane().view_model.simulate_virtual_response(b"WARN gui editor\n")
    window.active_pane()._refresh_output()
    html = window.active_pane().output.toHtml()
    filter_html = window.active_pane().filter_output.toHtml()
    zed_sidebar_layout = window.active_pane().main_splitter.count() == 2
    zed_bottom_filter = window.active_pane().content_splitter.count() == 2 and "WARN gui editor" in filter_html
    editor_button_present = "ping_btn" in [button.name for button in window.view_model.button_rows()]
    custom_status = window.active_pane().last_status
    tmpdir_handle = tempfile.TemporaryDirectory()
    filter_path = str(Path(tmpdir_handle.name) / "warn-filters.json")
    window.save_filter_profile_as(filter_path)
    saved_filter_exists = Path(filter_path).exists()
    window.clear_output()
    cleared = len(window.active_pane().view_model.output_lines) == 0
    first_tab_count = window.tabs.count()
    second = window.add_tab()
    second.view_model.load_filter_profile(filter_path)
    second_filter_path = second.view_model.filter_profile_path == filter_path
    first_filter_is_independent = second.view_model is not window.panes[window.tab_manager.tabs[0].tab_id].view_model
    second.open_virtual_demo()
    second.send_version()
    second.simulate_response()
    second_html = second.output.toHtml()
    window.poll_all_tabs()
    window.close_port()
    is_open_after_close = window.view_model.is_open
    window.tab_manager.close_all()
    tmpdir_handle.cleanup()
    app.processEvents()
    return (
        f"window={window.window.windowTitle()}",
        f"tabs={first_tab_count + 1}",
        f"add_tab_corner={str(add_tab_corner).lower()}",
        f"add_tab_toolbar_removed={str(add_tab_toolbar_removed).lower()}",
        f"open={is_open_after_close}",
        f"has_error={str('ERROR virtual demo timeout' in html).lower()}",
        f"has_second_error={str('ERROR virtual demo timeout' in second_html).lower()}",
        f"has_red={str('#d70000' in html or 'red' in html).lower()}",
        f"has_warn={str('WARN gui editor' in html).lower()}",
        f"zed_sidebar_layout={str(zed_sidebar_layout).lower()}",
        f"zed_bottom_filter={str(zed_bottom_filter).lower()}",
        f"has_editor_button={str(editor_button_present).lower()}",
        f"saved_filter_exists={str(saved_filter_exists).lower()}",
        f"second_filter_path={str(second_filter_path).lower()}",
        f"tab_filters_independent={str(first_filter_is_independent).lower()}",
        f"custom_tx={str(custom_status.startswith('device saw')).lower()}",
        f"custom_no_crlf={str(custom_no_crlf).lower()}",
        f"status_outside_rx={str('tx ' not in html and 'device saw' not in html).lower()}",
        f"cleared={str(cleared).lower()}",
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
