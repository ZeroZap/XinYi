from __future__ import annotations

import os
os.environ.setdefault("QT_QPA_PLATFORMTHEME", "generic")
import tempfile
from pathlib import Path
from typing import Sequence

from ..serial_cli import DEFAULT_WORKSPACE
from ..serial_session_state import SerialSessionState, SerialTabSessionState, load_session_state, save_session_state
from ..serial_transport import SerialPortInfo
from .z_serial_rendering import lines_to_html
from .z_serial_tabs import ZSerialTab, ZSerialTabManager
from .z_serial_view_model import ZSerialWindowViewModel


FILTER_DIALOG_MIN_WIDTH = 360


def render_startup_lines() -> tuple[str, ...]:
    return ()


def _load_qt_widgets():
    try:
        from PySide6.QtCore import Qt, QTimer  # type: ignore[import-not-found]
        from PySide6.QtGui import QAction  # type: ignore[import-not-found]
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
            QStyle,
            QTabWidget,
            QTextEdit,
            QToolButton,
            QVBoxLayout,
            QWidget,
        )
    except ImportError as exc:
        raise RuntimeError("PySide6 is required for z-serial GUI; install the 'PySide6' package") from exc
    return (
        QApplication,
        QAction,
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
        QStyle,
        QTabWidget,
        QTextEdit,
        QToolButton,
        Qt,
        QTimer,
        QVBoxLayout,
        QWidget,
    )


class ZSerialTabPane:
    def __init__(self, widgets, tab: ZSerialTab):
        (
            _QApplication,
            _QAction,
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
            _QStyle,
            _QTabWidget,
            QTextEdit,
            _QToolButton,
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
        self.port_input.setMinimumWidth(160)
        self.port_input.setMaximumWidth(240)
        self.port_input.setSizeAdjustPolicy(QComboBox.SizeAdjustPolicy.AdjustToMinimumContentsLengthWithIcon)
        self.port_input.setMinimumContentsLength(14)
        self.port_input.setSizePolicy(QSizePolicy.Policy.Fixed, QSizePolicy.Policy.Fixed)
        self.port_input.view().setMinimumWidth(420)
        self.port_input.lineEdit().setPlaceholderText("/dev/ttyACM0 or /dev/ttyUSB0")
        self.baud_input = QLineEdit(str(self.view_model.baudrate))
        self.refresh_ports_button = QPushButton("刷新端口")
        self.open_button = QPushButton("打开")
        self.connect_button = self.open_button
        self.virtual_demo_button = QPushButton("打开虚拟演示")
        self.close_button = QPushButton("关闭")
        self.close_button.hide()
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
        self.search_input = QLineEdit()
        self.search_input.setPlaceholderText("搜索日志")
        self.search_button = QPushButton("搜索")
        self.search_clear_button = QPushButton("清除搜索")
        self.output = QTextEdit()
        self.output.setReadOnly(True)
        self.output.setMinimumHeight(180)
        self.output.setSizePolicy(QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Expanding)
        self.output.setHtml(lines_to_html(()))
        self.filter_output = QTextEdit()
        self.filter_output.setReadOnly(True)
        self.filter_output.setMinimumHeight(80)
        self.filter_output.setSizePolicy(QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Expanding)
        self.filter_output.setPlaceholderText("滤波命中内容")
        self.filter_output.setHtml(lines_to_html(()))
        self.last_status = "ready"

        connection_row = QVBoxLayout()
        connection_row.addWidget(QLabel("端口"))
        connection_row.addWidget(self.port_input)
        connection_row.addWidget(QLabel("波特率"))
        connection_row.addWidget(self.baud_input)
        connection_row.addWidget(self.refresh_ports_button)
        connection_row.addWidget(self.connect_button)

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

        search_row = QVBoxLayout()
        search_row.addWidget(QLabel("搜索"))
        search_row.addWidget(self.search_input)
        search_row.addWidget(self.search_button)
        search_row.addWidget(self.search_clear_button)

        settings_panel = QWidget()
        settings_panel.setObjectName("zserial_settings_panel")
        settings_panel.setMinimumWidth(180)
        settings_panel.setMaximumWidth(320)
        settings_layout = QVBoxLayout()
        settings_layout.addLayout(connection_row)
        settings_layout.addSpacing(12)
        settings_layout.addLayout(send_row)
        settings_layout.addSpacing(12)
        settings_layout.addLayout(search_row)
        settings_layout.addSpacing(12)
        settings_layout.addLayout(action_row)
        settings_panel.setLayout(settings_layout)

        log_panel = QWidget()
        log_panel.setMinimumHeight(220)
        log_panel.setSizePolicy(QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Expanding)
        log_layout = QVBoxLayout()
        log_layout.addWidget(QLabel("Log"))
        log_layout.addWidget(self.output)
        log_panel.setLayout(log_layout)

        filter_panel = QWidget()
        filter_panel.setMinimumHeight(100)
        filter_panel.setSizePolicy(QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Expanding)
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
        self.content_splitter.setStretchFactor(0, 5)
        self.content_splitter.setStretchFactor(1, 1)
        self.content_splitter.setSizes([420, 120])
        self.main_splitter.addWidget(settings_panel)
        self.main_splitter.addWidget(self.content_splitter)
        self.main_splitter.setStretchFactor(0, 0)
        self.main_splitter.setStretchFactor(1, 1)
        self.main_splitter.setSizes([220, 720])

        layout = QVBoxLayout()
        layout.addWidget(self.main_splitter)
        self.widget.setLayout(layout)
        self._connect_signals()
        self.update_connection_buttons()

    def _connect_signals(self) -> None:
        self.refresh_ports_button.clicked.connect(self.refresh_ports)
        self.open_button.clicked.connect(self.toggle_port)
        self.virtual_demo_button.clicked.connect(self.open_virtual_demo)
        self.close_button.clicked.connect(self.close_port)
        self.send_version_button.clicked.connect(self.send_version)
        self.simulate_response_button.clicked.connect(self.simulate_response)
        self.poll_button.clicked.connect(self.poll_rx)
        self.clear_button.clicked.connect(self.clear_output)
        self.send_text_button.clicked.connect(self.send_text)
        self.send_input.returnPressed.connect(self.send_text)
        self.search_button.clicked.connect(self.search_output)
        self.search_clear_button.clicked.connect(self.clear_search)
        self.search_input.returnPressed.connect(self.search_output)

    def refresh_ports(self) -> None:
        top_level = self.widget.window()
        geometry = top_level.geometry() if top_level is not None else None
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
        if geometry is not None:
            top_level.setGeometry(geometry)
        if ports:
            self._append_status(f"ports {len(ports)} found")
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
        return port.port

    @staticmethod
    def _port_tooltip(port: SerialPortInfo) -> str:
        lines = [f"Port: {port.port}"]
        if port.description:
            lines.append(f"Description: {port.description}")
        if port.hwid:
            lines.append(f"HWID: {port.hwid}")
        return "\n".join(lines)

    def toggle_port(self) -> None:
        if self.view_model.is_open:
            self.close_port()
        else:
            self.open_port()

    def open_port(self) -> None:
        try:
            self.view_model.open_port(self.selected_port_text(), int(self.baud_input.text().strip()))
            self._append_status(f"opened {self.view_model.selected_port} @ {self.view_model.baudrate}")
        except Exception as exc:
            self._append_status(f"open failed: {exc}")
        finally:
            self.update_connection_buttons()

    def close_port(self) -> None:
        self.view_model.close_port()
        self._append_status("closed")
        self.update_connection_buttons()

    def open_virtual_demo(self) -> None:
        try:
            demo = self.view_model.open_virtual_demo()
            self.port_input.setEditText(demo.host_path)
            self._append_status(f"virtual demo opened host={demo.host_path} device={demo.device_path}")
        except Exception as exc:
            self._append_status(f"virtual demo failed: {exc}")
        finally:
            self.update_connection_buttons()

    def send_version(self) -> None:
        try:
            payload = self.view_model.send_button("version")
            self._refresh_output()
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
            self._refresh_output()
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

    def search_output(self) -> None:
        query = self.search_input.text().strip()
        results = self.view_model.search_output(query)
        self.filter_output.setPlainText("\n".join(f"#{result.index + 1} {result.text}" for result in results))
        self._append_status(f"search {len(results)} match(es)")

    def clear_search(self) -> None:
        self.search_input.clear()
        self._refresh_output()
        self._append_status("search cleared")

    def apply_profile_fields(self) -> None:
        self.port_input.setEditText(self.view_model.selected_port)
        self.baud_input.setText(str(self.view_model.baudrate))
        self._refresh_output()
        self.update_connection_buttons()

    def update_connection_buttons(self) -> None:
        is_open = self.view_model.is_open
        self.open_button.setText("关闭" if is_open else "打开")
        self.open_button.setEnabled(True)
        self.close_button.setVisible(False)

    def _append_status(self, text: str) -> None:
        self.last_status = text

    def _refresh_output(self) -> None:
        self.output.setHtml(lines_to_html(self.view_model.output_lines))
        self.filter_output.setPlainText(self._filter_panel_text())
        self._scroll_to_bottom()

    def _filter_panel_text(self) -> str:
        return "\n".join(line.text for line in self.view_model.output_lines if line.matched_rules)

    def _scroll_to_bottom(self) -> None:
        scrollbar = self.output.verticalScrollBar()
        scrollbar.setValue(scrollbar.maximum())
        filter_scrollbar = self.filter_output.verticalScrollBar()
        filter_scrollbar.setValue(filter_scrollbar.maximum())


class ZSerialMainWindow:
    def __init__(
        self,
        widgets,
        tab_manager: ZSerialTabManager | None = None,
        session_state_path: str | Path | None = None,
        restore_session: bool = True,
    ):
        (
            _QApplication,
            QAction,
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
            QStyle,
            QTabWidget,
            _QTextEdit,
            QToolButton,
            Qt,
            QTimer,
            QVBoxLayout,
            QWidget,
        ) = widgets
        self.widgets = widgets
        self.QFileDialog = QFileDialog
        self.tab_manager = tab_manager or ZSerialTabManager(DEFAULT_WORKSPACE)
        self.session_state_path = Path(session_state_path) if session_state_path is not None else self.default_session_state_path()
        self.restore_session = restore_session
        self._restoring_session = False
        self._mutating_tabs = False
        self.panes: dict[str, ZSerialTabPane] = {}
        self.window = QMainWindow()
        self.window.setWindowTitle("z-serial")
        self.window.setWindowFlags(
            Qt.WindowType.Window
            | Qt.WindowType.WindowTitleHint
            | Qt.WindowType.WindowSystemMenuHint
            | Qt.WindowType.WindowMinimizeButtonHint
            | Qt.WindowType.WindowMaximizeButtonHint
            | Qt.WindowType.WindowCloseButtonHint
        )
        self.window.setWindowIcon(self.window.style().standardIcon(QStyle.StandardPixmap.SP_TitleBarCloseButton))

        self.add_tab_action = QAction("新增串口", self.window)
        self.quit_action = QAction("关闭窗口", self.window)
        self.load_profile_action = QAction("打开 Profile", self.window)
        self.save_profile_action = QAction("保存 Profile", self.window)
        self.load_filters_action = QAction("打开滤波器", self.window)
        self.save_filters_action = QAction("覆盖滤波器", self.window)
        self.save_filters_as_action = QAction("滤波器另存为", self.window)
        self.edit_filter_action = QAction("编辑过滤器", self.window)
        self.edit_button_action = QAction("编辑按钮", self.window)
        self.add_tab_button = QPushButton("➕")
        self.add_tab_button.setToolTip("新增串口 Tab")
        self.add_tab_button.setFixedWidth(36)
        self.close_tab_button = QPushButton("关闭 Tab")
        self.close_tab_button.hide()
        self.load_profile_button = QPushButton("打开 Profile")
        self.load_profile_button.hide()
        self.save_profile_button = QPushButton("保存 Profile")
        self.save_profile_button.hide()
        self.load_filters_button = QPushButton("打开滤波器")
        self.load_filters_button.hide()
        self.save_filters_button = QPushButton("覆盖滤波器")
        self.save_filters_button.hide()
        self.save_filters_as_button = QPushButton("滤波器另存为")
        self.save_filters_as_button.hide()
        self.edit_filter_button = QPushButton("编辑过滤器")
        self.edit_filter_button.hide()
        self.edit_button_button = QPushButton("编辑按钮")
        self.edit_button_button.hide()
        self.tabs = QTabWidget()
        self.tabs.setTabsClosable(True)
        self._plus_tab_widget = QWidget()
        self._plus_tab_index: int | None = None
        self.status_line = QPushButton("状态: ready")
        self.status_line.setEnabled(False)
        self._build_menu_bar()

        layout = QVBoxLayout()
        layout.addWidget(self.tabs)
        layout.addWidget(self.status_line)
        central = QWidget()
        central.setLayout(layout)
        self.window.setCentralWidget(central)
        self.window.resize(960, 600)

        self.poll_timer = QTimer()
        self.poll_timer.setInterval(200)
        self.poll_timer.timeout.connect(self.poll_all_tabs)
        self.poll_timer.start()
        self._connect_signals()
        if not self._restore_session_if_available():
            self.add_tab()

    @staticmethod
    def default_session_state_path() -> Path:
        base = Path(os.environ.get("XDG_STATE_HOME", Path.home() / ".local" / "state"))
        return base / "xinyi" / "z-serial-session.json"

    @property
    def view_model(self) -> ZSerialWindowViewModel:
        return self.active_pane().view_model

    def show(self) -> None:
        self.window.show()

    def _build_menu_bar(self) -> None:
        serial_menu = self.window.menuBar().addMenu("串口")
        serial_menu.addAction(self.add_tab_action)
        serial_menu.addSeparator()
        serial_menu.addAction(self.quit_action)
        profile_menu = self.window.menuBar().addMenu("Profile")
        profile_menu.addAction(self.load_profile_action)
        profile_menu.addAction(self.save_profile_action)
        filter_menu = self.window.menuBar().addMenu("滤波器")
        filter_menu.addAction(self.load_filters_action)
        filter_menu.addAction(self.save_filters_action)
        filter_menu.addAction(self.save_filters_as_action)
        filter_menu.addSeparator()
        filter_menu.addAction(self.edit_filter_action)
        filter_menu.addAction(self.edit_button_action)

    def _restore_session_if_available(self) -> bool:
        if not self.restore_session or not self.session_state_path.exists():
            return False
        try:
            state = load_session_state(self.session_state_path)
            if not state.tabs:
                return False
            self._restoring_session = True
            self.tab_manager.close_all()
            self.panes.clear()
            self.tabs.clear()
            self._plus_tab_index = None
            for tab_state in state.tabs:
                pane = self.add_tab(tab_state.title)
                pane.view_model.selected_port = tab_state.port
                pane.view_model.baudrate = tab_state.baudrate
                if tab_state.filter_profile_path:
                    try:
                        pane.view_model.load_filter_profile(tab_state.filter_profile_path)
                    except Exception as exc:
                        pane.last_status = f"filter restore skipped: {exc}"
                pane.apply_profile_fields()
            self.tabs.setCurrentIndex(state.active_index)
            self.tab_manager.set_active_index(state.active_index)
            self._set_status(f"session restored tabs={len(state.tabs)}")
            return True
        except Exception as exc:
            self._set_status(f"session restore skipped: {exc}")
            return False
        finally:
            self._restoring_session = False

    def save_session_state(self) -> None:
        state = SerialSessionState(
            active_index=self.tab_manager.active_index,
            tabs=tuple(
                SerialTabSessionState(
                    title=tab.title,
                    port=tab.view_model.selected_port,
                    baudrate=tab.view_model.baudrate,
                    filter_profile_path=tab.view_model.filter_profile_path,
                )
                for tab in self.tab_manager.tabs
            ),
        )
        save_session_state(self.session_state_path, state)

    def _save_session_state_if_ready(self) -> None:
        if self._restoring_session:
            return
        try:
            self.save_session_state()
        except Exception as exc:
            self._set_status(f"session save skipped: {exc}")

    def _connect_signals(self) -> None:
        self.add_tab_action.triggered.connect(self.add_tab)
        self.quit_action.triggered.connect(self.window.close)
        self.load_profile_action.triggered.connect(self.load_profile_dialog)
        self.save_profile_action.triggered.connect(self.save_profile_dialog)
        self.load_filters_action.triggered.connect(self.load_filter_profile_dialog)
        self.save_filters_action.triggered.connect(self.save_filter_profile)
        self.save_filters_as_action.triggered.connect(self.save_filter_profile_as_dialog)
        self.edit_filter_action.triggered.connect(self.edit_filter_dialog)
        self.edit_button_action.triggered.connect(self.edit_button_dialog)
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
        self.tabs.tabBarClicked.connect(self._handle_tab_clicked)
        self.tabs.tabCloseRequested.connect(self.close_tab_at_index)

    def _ensure_plus_tab(self) -> None:
        if self._plus_tab_index is not None:
            current = self.tabs.indexOf(self._plus_tab_widget)
            if current >= 0:
                self._plus_tab_index = current
                return
        self._plus_tab_index = self.tabs.addTab(self._plus_tab_widget, "＋")
        self.tabs.tabBar().setTabButton(self._plus_tab_index, self.tabs.tabBar().ButtonPosition.LeftSide, None)
        self.tabs.tabBar().setTabButton(self._plus_tab_index, self.tabs.tabBar().ButtonPosition.RightSide, None)

    def _remove_plus_tab(self) -> None:
        if self._plus_tab_index is None:
            return
        current = self.tabs.indexOf(self._plus_tab_widget)
        if current >= 0:
            self.tabs.removeTab(current)
        self._plus_tab_index = None

    def _is_plus_tab_index(self, index: int) -> bool:
        return self._plus_tab_index is not None and index == self._plus_tab_index

    def add_tab(self, title: str | None = None) -> ZSerialTabPane:
        previous_mutating = self._mutating_tabs
        self._mutating_tabs = True
        self._remove_plus_tab()
        tab = self.tab_manager.add_tab(title)
        pane = ZSerialTabPane(self.widgets, tab)
        for button in (
            pane.refresh_ports_button,
            pane.open_button,
            pane.virtual_demo_button,
            pane.close_button,
            pane.send_version_button,
            pane.simulate_response_button,
            pane.poll_button,
            pane.clear_button,
            pane.send_text_button,
            pane.search_button,
            pane.search_clear_button,
        ):
            button.clicked.connect(lambda _checked=False, pane=pane: self._sync_pane_status(pane))
        pane.send_input.returnPressed.connect(lambda pane=pane: self._sync_pane_status(pane))
        pane.search_input.returnPressed.connect(lambda pane=pane: self._sync_pane_status(pane))
        pane.open_button.clicked.connect(self._save_session_state_if_ready)
        pane.close_button.clicked.connect(self._save_session_state_if_ready)
        pane.refresh_ports_button.clicked.connect(self._save_session_state_if_ready)
        self.panes[tab.tab_id] = pane
        tab_index = self.tabs.addTab(pane.widget, tab.title)
        self._ensure_plus_tab()
        self.tabs.setCurrentIndex(tab_index)
        self._mutating_tabs = previous_mutating
        self._set_status(f"added {tab.title}")
        self._save_session_state_if_ready()
        return pane

    def close_active_tab(self) -> None:
        self.close_tab_at_index(self.tabs.currentIndex())

    def close_tab_at_index(self, index: int) -> None:
        if index < 0:
            return
        if self._is_plus_tab_index(index):
            return
        if index >= len(self.tab_manager.tabs):
            return
        previous_mutating = self._mutating_tabs
        self._mutating_tabs = True
        self._remove_plus_tab()
        tab = self.tab_manager.close_tab(index)
        self.tabs.removeTab(index)
        self.panes.pop(tab.tab_id, None)
        self._ensure_plus_tab()
        if self.tab_manager.tabs:
            self.tabs.setCurrentIndex(self.tab_manager.active_index)
            self._set_status(f"closed {tab.title}")
        else:
            self.tabs.setCurrentIndex(self._plus_tab_index if self._plus_tab_index is not None else -1)
            self._set_status("all tabs closed")
        self._mutating_tabs = previous_mutating
        self._save_session_state_if_ready()

    def active_pane(self) -> ZSerialTabPane:
        if not self.tab_manager.tabs:
            return self.add_tab()
        tab = self.tab_manager.active_tab()
        return self.panes[tab.tab_id]

    def _sync_pane_status(self, pane: ZSerialTabPane) -> None:
        self._set_status(pane.last_status)

    def _set_active_index(self, index: int) -> None:
        if self._mutating_tabs:
            return
        if self._is_plus_tab_index(index):
            self.add_tab()
            return
        if index >= 0 and index < len(self.tab_manager.tabs):
            self.tab_manager.set_active_index(index)
            self._set_status(f"active {self.tab_manager.active_tab().title}")
            self._save_session_state_if_ready()

    def _handle_tab_clicked(self, index: int) -> None:
        if not self._mutating_tabs and self._is_plus_tab_index(index):
            self.add_tab()

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
        self._save_session_state_if_ready()

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
        self._save_session_state_if_ready()

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
        self._save_session_state_if_ready()

    def save_filter_profile(self) -> None:
        try:
            path = self.view_model.save_filter_profile()
            self._set_status(f"filters saved {path}")
        except ValueError:
            self.save_filter_profile_as_dialog()

    def save_filter_profile_as(self, path: str) -> None:
        saved_path = self.view_model.save_filter_profile_as(path)
        self._set_status(f"filters saved {saved_path}")
        self._save_session_state_if_ready()

    def load_profile(self, path: str) -> None:
        windows = self.view_model.load_profile(path)
        self.active_pane().apply_profile_fields()
        self._set_status(f"profile loaded {path} windows={len(windows)}")
        self._save_session_state_if_ready()

    def add_filter(
        self,
        name: str,
        keywords: str,
        foreground: str = "yellow",
        background: str = "default",
        match: str = "any",
        action: str = "highlight",
    ) -> None:
        keyword_tuple = tuple(part.strip() for part in keywords.replace(";", ",").split(","))
        rule = self.view_model.upsert_filter(
            name,
            keyword_tuple,
            match=match,
            foreground=foreground,
            background=background,
            action=action,
        )
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

    def _build_color_picker_label(self, initial_color: str):
        from PySide6.QtCore import Qt
        from PySide6.QtGui import QColor
        from PySide6.QtWidgets import QColorDialog, QPushButton

        selected_color = {"value": self._normalize_filter_color(initial_color)}
        label = QPushButton()
        label.setFixedSize(96, 30)
        label.setCursor(Qt.CursorShape.PointingHandCursor)
        label.setToolTip("点击选择颜色")

        def render_label() -> None:
            color = QColor(selected_color["value"])
            text = color.name() if color.isValid() else "#d4d4d4"
            label.setText(text)
            label.setStyleSheet(
                f"background-color: {text}; color: {self._contrast_text_color(text)};"
                "border: 1px solid #555555; border-radius: 4px; padding: 4px;"
            )

        def choose_color() -> None:
            current = QColor(selected_color["value"])
            if not current.isValid():
                current = QColor("#d4d4d4")
            selected = QColorDialog.getColor(current, self.window, "选择过滤器颜色")
            if selected.isValid():
                selected_color["value"] = selected.name()
                render_label()

        label.clicked.connect(choose_color)
        render_label()
        return label, lambda: selected_color["value"]

    @staticmethod
    def _normalize_filter_color(value: str) -> str:
        from PySide6.QtGui import QColor

        text = value.strip()
        if not text or text.lower() == "default":
            return "#d4d4d4"
        color = QColor(text)
        if color.isValid():
            return color.name()
        return "#d4d4d4"

    @staticmethod
    def _contrast_text_color(color_value: str) -> str:
        from PySide6.QtGui import QColor

        color = QColor(color_value)
        if not color.isValid():
            return "#111111"
        brightness = (color.red() * 299 + color.green() * 587 + color.blue() * 114) / 1000
        return "#111111" if brightness >= 140 else "#f5f5f5"

    def edit_filter_dialog(self) -> None:
        try:
            from PySide6.QtWidgets import QDialog, QDialogButtonBox, QFormLayout, QLineEdit

            dialog = QDialog(self.window)
            dialog.setWindowTitle("编辑过滤器")
            dialog.setMinimumWidth(FILTER_DIALOG_MIN_WIDTH)
            name_input = QLineEdit("warn")
            keywords_input = QLineEdit("WARN,WARNING")
            foreground_label, foreground_value = self._build_color_picker_label("yellow")
            background_label, background_value = self._build_color_picker_label("#d4d4d4")
            form = QFormLayout()
            form.addRow("名称", name_input)
            form.addRow("关键词(逗号分隔)", keywords_input)
            form.addRow("前景色", foreground_label)
            form.addRow("背景色", background_label)
            buttons = QDialogButtonBox(QDialogButtonBox.StandardButton.Ok | QDialogButtonBox.StandardButton.Cancel)
            buttons.accepted.connect(dialog.accept)
            buttons.rejected.connect(dialog.reject)
            form.addWidget(buttons)
            dialog.setLayout(form)
            dialog.adjustSize()
            if dialog.exec() == QDialog.DialogCode.Accepted:
                self.add_filter(
                    name_input.text(),
                    keywords_input.text(),
                    foreground_value(),
                    background_value(),
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
    tmpdir_handle = tempfile.TemporaryDirectory()
    session_path = str(Path(tmpdir_handle.name) / "session.json")
    window = ZSerialMainWindow(widgets, session_state_path=session_path, restore_session=False)
    menu_bar = window.window.menuBar()
    menu_titles = [action.text() for action in menu_bar.actions()]
    menu_framework_present = "串口" in menu_titles and "Profile" in menu_titles and "滤波器" in menu_titles
    top_buttons_hidden = all(
        button.isHidden()
        for button in (
            window.close_tab_button,
            window.load_profile_button,
            window.save_profile_button,
            window.load_filters_button,
            window.save_filters_button,
            window.save_filters_as_button,
            window.edit_filter_button,
            window.edit_button_button,
        )
    )
    window_close_icon_present = not window.window.windowIcon().isNull()
    window_flags = window.window.windowFlags()
    window_controls_present = all(
        bool(window_flags & flag)
        for flag in (
            widgets[16].WindowType.WindowSystemMenuHint,
            widgets[16].WindowType.WindowMinimizeButtonHint,
            widgets[16].WindowType.WindowMaximizeButtonHint,
            widgets[16].WindowType.WindowCloseButtonHint,
        )
    )
    tab_close_enabled = window.tabs.tabsClosable()
    default_window_compact = window.window.size().width() <= 960 and window.window.size().height() <= 600
    layout_allows_smaller_resize = (
        window.active_pane().output.minimumHeight() <= 180
        and window.active_pane().filter_output.minimumHeight() <= 80
        and window.active_pane().port_input.minimumWidth() <= 160
    )
    window.add_filter("warn", "WARN", foreground="yellow")
    window.add_filter("noise", "debug", action="hide")
    color_label, color_value = window._build_color_picker_label("yellow")
    color_label_hex_preview = color_label.text() == "#ffff00" and "background-color: #ffff00" in color_label.styleSheet()
    color_label_fixed_size = color_label.minimumSize() == color_label.maximumSize()
    color_label_value_saves_hex = color_value() == "#ffff00"
    filter_dialog_min_width_stable = FILTER_DIALOG_MIN_WIDTH >= 360
    window.add_button("ping_btn", "Ping", "text", "ping", append_newline=True)
    plus_tab_initial_index = window.tabs.count() - 1
    plus_tab_adjacent = window.tabs.tabText(plus_tab_initial_index) == "＋" and plus_tab_initial_index == len(window.tab_manager.tabs)
    add_tab_toolbar_removed = getattr(window, "new_tab_button", None) is None
    window.open_virtual_demo()
    connection_button_toggles_to_close = window.active_pane().open_button.text() == "关闭" and window.active_pane().open_button.isEnabled()
    window.view_model.send_button("ping_btn")
    window.send_version()
    window.send_text("ping")
    demo = window.active_pane().view_model.virtual_demo
    if demo is None:
        raise RuntimeError("virtual demo missing during GUI smoke")
    custom_payload = demo.read_device_command()
    custom_no_crlf = custom_payload.endswith(b"ping") and not custom_payload.endswith(b"ping\r\n")
    window.simulate_response()
    window.active_pane().view_model.simulate_virtual_response(b"WARN gui editor\ndebug noisy raw log\nplain unmatched rx\n")
    window.active_pane()._refresh_output()
    html = window.active_pane().output.toHtml()
    filter_html = window.active_pane().filter_output.toHtml()
    zed_sidebar_layout = window.active_pane().main_splitter.count() == 2
    zed_bottom_filter = window.active_pane().content_splitter.count() == 2 and "WARN gui editor" in filter_html
    main_log_keeps_filtered_lines = "debug noisy raw log" in html
    tx_visible_in_log = "tx ping" in html or "tx version" in html
    filter_window_contains_matches = "WARN gui editor" in filter_html and "debug noisy raw log" in filter_html
    main_log_shows_unmatched_rx = "rx plain unmatched rx" in html
    filter_window_excludes_unmatched_rx = "plain unmatched rx" not in filter_html
    log_panel_visible = window.active_pane().output.minimumHeight() >= 180 and "ERROR virtual demo timeout" in html
    custom_status = window.active_pane().last_status
    filter_summary_hidden = "warn: hits=1" not in window.active_pane().filter_output.toPlainText()
    window.active_pane().search_input.setText("timeout")
    window.active_pane().search_output()
    search_finds_timeout = "ERROR virtual demo timeout" in window.active_pane().filter_output.toPlainText()
    editor_button_present = "ping_btn" in [button.name for button in window.view_model.button_rows()]
    filter_path = str(Path(tmpdir_handle.name) / "warn-filters.json")
    window.save_filter_profile_as(filter_path)
    saved_filter_exists = Path(filter_path).exists()
    window.clear_output()
    cleared = len(window.active_pane().view_model.output_lines) == 0
    first_tab_count = window.tabs.count()
    second = window.add_tab()
    plus_tab_moves_right = window.tabs.tabText(window.tabs.count() - 1) == "＋" and window.tabs.count() == first_tab_count + 1
    second.view_model.load_filter_profile(filter_path)
    second_filter_path = second.view_model.filter_profile_path == filter_path
    first_filter_is_independent = second.view_model is not window.panes[window.tab_manager.tabs[0].tab_id].view_model
    second.open_virtual_demo()
    second.send_version()
    second.simulate_response()
    second_html = second.output.toHtml()
    window.poll_all_tabs()
    window.save_session_state()
    restored = ZSerialMainWindow(widgets, session_state_path=session_path, restore_session=True)
    session_restored = len(restored.tab_manager.tabs) == 2 and restored.tab_manager.active_index == window.tab_manager.active_index
    real_tab_count = len(window.tab_manager.tabs)
    restored.tab_manager.close_all()
    window.close_port()
    is_open_after_close = window.view_model.is_open
    open_button_resets_after_close = window.active_pane().open_button.text() == "打开" and window.active_pane().open_button.isEnabled()
    while window.tab_manager.tabs:
        window.close_tab_at_index(0)
    add_after_all_closed_possible = window.tabs.count() == 1 and window.tabs.tabText(0) == "＋" and len(window.tab_manager.tabs) == 0
    window._handle_tab_clicked(0)
    add_after_all_closed_works = len(window.tab_manager.tabs) == 1 and window.tabs.tabText(window.tabs.count() - 1) == "＋"
    window.tab_manager.close_all()
    tmpdir_handle.cleanup()
    app.processEvents()
    return (
        f"window={window.window.windowTitle()}",
        f"tabs={real_tab_count}",
        f"plus_tab_adjacent={str(plus_tab_adjacent).lower()}",
        f"plus_tab_moves_right={str(plus_tab_moves_right).lower()}",
        f"menu_framework_present={str(menu_framework_present).lower()}",
        f"top_buttons_hidden={str(top_buttons_hidden).lower()}",
        f"window_close_icon_present={str(window_close_icon_present).lower()}",
        f"window_controls_present={str(window_controls_present).lower()}",
        f"tab_close_enabled={str(tab_close_enabled).lower()}",
        f"default_window_compact={str(default_window_compact).lower()}",
        f"layout_allows_smaller_resize={str(layout_allows_smaller_resize).lower()}",
        f"add_after_all_closed_possible={str(add_after_all_closed_possible).lower()}",
        f"add_after_all_closed_works={str(add_after_all_closed_works).lower()}",
        f"add_tab_toolbar_removed={str(add_tab_toolbar_removed).lower()}",
        f"open={is_open_after_close}",
        f"connection_button_toggles_to_close={str(connection_button_toggles_to_close).lower()}",
        f"open_button_resets_after_close={str(open_button_resets_after_close).lower()}",
        f"has_error={str('ERROR virtual demo timeout' in html).lower()}",
        f"has_second_error={str('ERROR virtual demo timeout' in second_html).lower()}",
        f"has_red={str('#d70000' in html or 'red' in html).lower()}",
        f"has_warn={str('WARN gui editor' in html).lower()}",
        f"main_log_keeps_filtered_lines={str(main_log_keeps_filtered_lines).lower()}",
        f"tx_visible_in_log={str(tx_visible_in_log).lower()}",
        f"filter_window_contains_matches={str(filter_window_contains_matches).lower()}",
        f"main_log_shows_unmatched_rx={str(main_log_shows_unmatched_rx).lower()}",
        f"filter_window_excludes_unmatched_rx={str(filter_window_excludes_unmatched_rx).lower()}",
        f"color_label_hex_preview={str(color_label_hex_preview).lower()}",
        f"color_label_fixed_size={str(color_label_fixed_size).lower()}",
        f"color_label_value_saves_hex={str(color_label_value_saves_hex).lower()}",
        f"filter_dialog_min_width_stable={str(filter_dialog_min_width_stable).lower()}",
        f"log_panel_visible={str(log_panel_visible).lower()}",
        f"zed_sidebar_layout={str(zed_sidebar_layout).lower()}",
        f"zed_bottom_filter={str(zed_bottom_filter).lower()}",
        f"filter_summary_hidden={str(filter_summary_hidden).lower()}",
        f"search_finds_timeout={str(search_finds_timeout).lower()}",
        f"session_restored={str(session_restored).lower()}",
        f"has_editor_button={str(editor_button_present).lower()}",
        f"saved_filter_exists={str(saved_filter_exists).lower()}",
        f"second_filter_path={str(second_filter_path).lower()}",
        f"tab_filters_independent={str(first_filter_is_independent).lower()}",
        f"custom_tx={str(custom_status.startswith('device saw')).lower()}",
        f"custom_no_crlf={str(custom_no_crlf).lower()}",
        f"status_outside_rx={str('device saw' not in html).lower()}",
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
