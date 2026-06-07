from __future__ import annotations

from dataclasses import dataclass, field
from typing import Callable

from ..serial_cli import DEFAULT_WORKSPACE
from ..serial_config import SerialWindowProfile, SerialWorkspaceProfile
from ..serial_service import SerialWindowSession, SerialWorkspaceService
from ..serial_transport import PySerialTransport, SerialTransport, list_serial_ports

TransportFactory = Callable[[str, int], SerialTransport]


@dataclass(frozen=True)
class RenderedLine:
    text: str
    foreground: str
    background: str
    matched_rules: tuple[str, ...]

    def as_plain_text(self) -> str:
        return f"fg={self.foreground} bg={self.background} rules={','.join(self.matched_rules) or '-'} | {self.text}"


@dataclass
class ZSerialWindowViewModel:
    workspace: SerialWorkspaceProfile = DEFAULT_WORKSPACE
    transport_factory: TransportFactory | None = None
    service: SerialWorkspaceService = field(init=False)
    session: SerialWindowSession | None = field(default=None, init=False)
    selected_port: str = ""
    baudrate: int = 115200
    output_lines: list[RenderedLine] = field(default_factory=list)

    def __post_init__(self) -> None:
        self.service = SerialWorkspaceService(self.workspace)

    @property
    def is_open(self) -> bool:
        return bool(self.session and self.session.is_open)

    def available_ports(self) -> tuple[str, ...]:
        return tuple(port.port for port in list_serial_ports())

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

    def send_button(self, button_name: str) -> bytes:
        session = self._require_session()
        return session.send_button(button_name)

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
        return rendered

    def render_output_text(self) -> str:
        return "\n".join(line.as_plain_text() for line in self.output_lines)

    def _make_transport(self, port: str, baudrate: int) -> SerialTransport:
        if self.transport_factory is not None:
            return self.transport_factory(port, baudrate)
        return PySerialTransport(port=port, baudrate=baudrate)

    def _require_session(self) -> SerialWindowSession:
        if self.session is None or not self.session.is_open:
            raise RuntimeError("serial port is not open")
        return self.session
