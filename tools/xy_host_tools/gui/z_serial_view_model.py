from __future__ import annotations

from dataclasses import dataclass, field, replace
from typing import Callable

from ..serial_cli import DEFAULT_WORKSPACE
from ..serial_config import ActionButton, FilterRule, SerialWindowProfile, SerialWorkspaceProfile
from ..serial_filter_profile import load_filter_profile, save_filter_profile
from ..serial_profile import load_workspace_profile, save_workspace_profile
from ..serial_service import SerialWindowSession, SerialWorkspaceService
from ..serial_transport import PySerialTransport, SerialPortInfo, SerialTransport, list_serial_ports
from ..serial_virtual import VirtualSerialPair, pump_virtual_pair

TransportFactory = Callable[[str, int], SerialTransport]
PortProvider = Callable[[], tuple[SerialPortInfo, ...]]


@dataclass(frozen=True)
class RenderedLine:
    text: str
    foreground: str
    background: str
    matched_rules: tuple[str, ...]

    def as_plain_text(self) -> str:
        return f"fg={self.foreground} bg={self.background} rules={','.join(self.matched_rules) or '-'} | {self.text}"


@dataclass
class VirtualDemoSession:
    pair: VirtualSerialPair

    @property
    def host_path(self) -> str:
        return self.pair.host_path

    @property
    def device_path(self) -> str:
        return self.pair.device_path

    def open_device(self) -> None:
        self.pair.device_transport.open()

    def read_device_command(self, size: int = 256) -> bytes:
        pump_virtual_pair(self.pair)
        return self.pair.device_transport.read(size)

    def write_device_response(self, data: bytes) -> None:
        self.pair.device_transport.write(data)
        pump_virtual_pair(self.pair)

    def close(self) -> None:
        self.pair.close()


@dataclass
class ZSerialWindowViewModel:
    workspace: SerialWorkspaceProfile = DEFAULT_WORKSPACE
    transport_factory: TransportFactory | None = None
    port_provider: PortProvider = list_serial_ports
    max_output_lines: int = 2000
    service: SerialWorkspaceService = field(init=False)
    session: SerialWindowSession | None = field(default=None, init=False)
    selected_port: str = ""
    baudrate: int = 115200
    output_lines: list[RenderedLine] = field(default_factory=list)
    virtual_demo: VirtualDemoSession | None = field(default=None, init=False)
    filter_profile_path: str | None = None
    filter_profile_name: str = "XinYi Serial Filters"

    def __post_init__(self) -> None:
        self.service = SerialWorkspaceService(self.workspace)

    @property
    def is_open(self) -> bool:
        return bool(self.session and self.session.is_open)

    def available_port_infos(self) -> tuple[SerialPortInfo, ...]:
        return self.port_provider()

    def available_ports(self) -> tuple[str, ...]:
        return tuple(port.port for port in self.available_port_infos())

    def open_port(self, port: str | None = None, baudrate: int | None = None) -> None:
        if self.is_open:
            return
        if port is not None:
            self.selected_port = port
        if baudrate is not None:
            self.baudrate = baudrate
        if not self.selected_port:
            raise ValueError("serial port is required")

        transport = self._make_transport(self.selected_port, self.baudrate)
        window = SerialWindowProfile(window_id="main", title="Main", port=self.selected_port, baudrate=self.baudrate)
        self.session = self.service.attach_window(window, transport, open_immediately=True)

    def close_port(self) -> None:
        if self.session is None:
            return
        self.service.detach_window(self.session.window.window_id)
        self.session = None
        if self.virtual_demo is not None:
            self.virtual_demo.close()
            self.virtual_demo = None

    def open_virtual_demo(self) -> VirtualDemoSession:
        if self.is_open:
            self.close_port()
        demo = VirtualDemoSession(VirtualSerialPair.create())
        demo.open_device()
        self.virtual_demo = demo
        previous_factory = self.transport_factory
        self.transport_factory = lambda _port, _baudrate: demo.pair.host_transport
        try:
            self.open_port(demo.host_path, self.baudrate)
        finally:
            self.transport_factory = previous_factory
        return demo

    def simulate_virtual_response(self, response: bytes | None = None) -> tuple[bytes, tuple[RenderedLine, ...]]:
        if self.virtual_demo is None:
            raise RuntimeError("virtual demo is not open")
        command = self.virtual_demo.read_device_command()
        self.virtual_demo.write_device_response(response or b"Boot FW=0.1.0\nERROR virtual demo timeout\n")
        return command, self.poll_rx()

    def send_button(self, button_name: str) -> bytes:
        session = self._require_session()
        return session.send_button(button_name)

    def send_text(self, text: str, append_newline: bool = True) -> bytes:
        session = self._require_session()
        payload = text + ("\r\n" if append_newline else "")
        data = payload.encode("utf-8")
        session.transport.write(data)
        return data

    def poll_rx(self, size: int = 4096) -> tuple[RenderedLine, ...]:
        session = self._require_session()
        rendered = tuple(
            RenderedLine(
                text=received.text,
                foreground=received.result.foreground,
                background=received.result.background,
                matched_rules=received.result.matched_rules,
            )
            for received in session.read_available(size)
        )
        self.output_lines.extend(rendered)
        self._trim_output_lines()
        return rendered

    def render_output_text(self) -> str:
        return "\n".join(line.as_plain_text() for line in self.output_lines)

    def clear_output(self) -> None:
        self.output_lines.clear()

    def filter_rows(self) -> tuple[FilterRule, ...]:
        return self.workspace.filters

    def button_rows(self) -> tuple[ActionButton, ...]:
        return self.workspace.buttons

    def upsert_filter(
        self,
        name: str,
        keywords: tuple[str, ...],
        match: str = "any",
        foreground: str = "default",
        background: str = "default",
        case_sensitive: bool = False,
        action: str = "highlight",
        priority: int = 0,
        enabled: bool = True,
    ) -> FilterRule:
        self._assert_profile_editable()
        filter_name = name.strip()
        if not filter_name:
            raise ValueError("filter name is required")
        normalized_keywords = tuple(keyword.strip() for keyword in keywords if keyword.strip())
        if not normalized_keywords:
            raise ValueError("filter keywords are required")
        rule = FilterRule(
            name=filter_name,
            keywords=normalized_keywords,
            match=match,  # type: ignore[arg-type]
            case_sensitive=case_sensitive,
            foreground=foreground.strip() or "default",
            background=background.strip() or "default",
            action=action,  # type: ignore[arg-type]
            priority=priority,
            enabled=enabled,
        )
        self.workspace = replace(
            self.workspace,
            filters=tuple(existing for existing in self.workspace.filters if existing.name != rule.name) + (rule,),
        )
        self._reset_service_for_profile_edit()
        return rule

    def remove_filter(self, name: str) -> bool:
        self._assert_profile_editable()
        updated = tuple(rule for rule in self.workspace.filters if rule.name != name)
        changed = len(updated) != len(self.workspace.filters)
        if changed:
            self.workspace = replace(self.workspace, filters=updated)
            self._reset_service_for_profile_edit()
        return changed

    def upsert_button(
        self,
        name: str,
        label: str,
        mode: str,
        payload: str,
        append_newline: bool = False,
        confirm: bool = False,
    ) -> ActionButton:
        self._assert_profile_editable()
        button_name = name.strip()
        if not button_name:
            raise ValueError("button name is required")
        if mode not in ("text", "hex", "script"):
            raise ValueError("button mode must be text, hex, or script")
        button = ActionButton(
            name=button_name,
            label=label.strip() or button_name,
            mode=mode,  # type: ignore[arg-type]
            payload=payload,
            append_newline=append_newline,
            confirm=confirm,
        )
        self.workspace = replace(
            self.workspace,
            buttons=tuple(existing for existing in self.workspace.buttons if existing.name != button.name) + (button,),
        )
        self._reset_service_for_profile_edit()
        return button

    def remove_button(self, name: str) -> bool:
        self._assert_profile_editable()
        updated = tuple(button for button in self.workspace.buttons if button.name != name)
        changed = len(updated) != len(self.workspace.buttons)
        if changed:
            self.workspace = replace(self.workspace, buttons=updated)
            self._reset_service_for_profile_edit()
        return changed

    def save_filter_profile(self, path: str | None = None, *, name: str | None = None) -> str:
        profile_path = path or self.filter_profile_path
        if not profile_path:
            raise ValueError("filter profile path is required")
        profile_name = name or self.filter_profile_name
        save_filter_profile(profile_path, profile_name, self.workspace.filters)
        self.filter_profile_path = profile_path
        self.filter_profile_name = profile_name
        return profile_path

    def save_filter_profile_as(self, path: str, *, name: str | None = None) -> str:
        return self.save_filter_profile(path, name=name)

    def load_filter_profile(self, path: str) -> tuple[FilterRule, ...]:
        self._assert_profile_editable()
        profile_name, filters = load_filter_profile(path)
        self.workspace = replace(self.workspace, name=self.workspace.name, filters=filters)
        self.filter_profile_path = path
        self.filter_profile_name = profile_name
        self._reset_service_for_profile_edit()
        return filters

    def save_profile(self, path: str) -> None:
        save_workspace_profile(path, self.workspace, windows=self._profile_windows())

    def load_profile(self, path: str) -> tuple[SerialWindowProfile, ...]:
        if self.is_open:
            self.close_port()
        workspace, windows = load_workspace_profile(path)
        self.workspace = workspace
        self.service = SerialWorkspaceService(self.workspace)
        self.output_lines.clear()
        if windows:
            self.selected_port = windows[0].port
            self.baudrate = windows[0].baudrate
        return windows

    def _profile_windows(self) -> tuple[SerialWindowProfile, ...]:
        if self.session is not None:
            return (self.session.window,)
        if self.selected_port:
            return (
                SerialWindowProfile(window_id="main", title="Main", port=self.selected_port, baudrate=self.baudrate),
            )
        return ()

    def _make_transport(self, port: str, baudrate: int) -> SerialTransport:
        if self.transport_factory is not None:
            return self.transport_factory(port, baudrate)
        return PySerialTransport(port=port, baudrate=baudrate)

    def _reset_service_for_profile_edit(self) -> None:
        self.service = SerialWorkspaceService(self.workspace)

    def _trim_output_lines(self) -> None:
        if self.max_output_lines <= 0:
            self.output_lines.clear()
            return
        overflow = len(self.output_lines) - self.max_output_lines
        if overflow > 0:
            del self.output_lines[:overflow]

    def _assert_profile_editable(self) -> None:
        if self.is_open:
            raise RuntimeError("close serial port before editing profile")

    def _require_session(self) -> SerialWindowSession:
        if self.session is None or not self.session.is_open:
            raise RuntimeError("serial port is not open")
        return self.session
