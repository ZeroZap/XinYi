from __future__ import annotations

from dataclasses import dataclass, field
from typing import Callable

from ..serial_cli import DEFAULT_WORKSPACE
from ..serial_config import SerialWindowProfile, SerialWorkspaceProfile
from ..serial_service import SerialWindowSession, SerialWorkspaceService
from ..serial_transport import PySerialTransport, SerialTransport, list_serial_ports
from ..serial_virtual import VirtualSerialPair, pump_virtual_pair

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
    service: SerialWorkspaceService = field(init=False)
    session: SerialWindowSession | None = field(default=None, init=False)
    selected_port: str = ""
    baudrate: int = 115200
    output_lines: list[RenderedLine] = field(default_factory=list)
    virtual_demo: VirtualDemoSession | None = field(default=None, init=False)

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
