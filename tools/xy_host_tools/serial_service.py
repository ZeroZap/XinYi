from __future__ import annotations

from dataclasses import dataclass, field
from typing import Mapping

from .serial_actions import render_button_payload
from .serial_config import ActionButton, SerialWindowProfile, SerialWorkspaceProfile
from .serial_filter import FilterResult, apply_filters
from .serial_transport import SerialTransport


@dataclass(frozen=True)
class ReceivedLine:
    window_id: str
    text: str
    result: FilterResult


@dataclass
class SerialWindowSession:
    workspace: SerialWorkspaceProfile
    window: SerialWindowProfile
    transport: SerialTransport
    last_rx: bytes = b""
    sent_bytes: int = 0
    received_lines: list[ReceivedLine] = field(default_factory=list)

    def open(self) -> None:
        self.transport.open()

    def close(self) -> None:
        self.transport.close()

    @property
    def is_open(self) -> bool:
        return self.transport.is_open

    def effective_buttons(self) -> tuple[ActionButton, ...]:
        return self.workspace.effective_buttons_for(self.window)

    def send_bytes(self, payload: bytes) -> int:
        written = self.transport.write(payload)
        self.sent_bytes += written
        return written

    def send_button(self, button_name: str, extra_context: Mapping[str, object] | None = None) -> bytes:
        buttons = {button.name: button for button in self.effective_buttons()}
        if button_name not in buttons:
            raise ValueError(f"unknown button for window {self.window.window_id}: {button_name}")

        context: dict[str, object] = {
            "port": self.window.port,
            "window_id": self.window.window_id,
            "last_rx": self.last_rx,
        }
        if extra_context:
            context.update(extra_context)

        payload = render_button_payload(buttons[button_name], context=context)
        self.send_bytes(payload)
        return payload

    def accept_rx_bytes(self, data: bytes, encoding: str = "utf-8") -> tuple[ReceivedLine, ...]:
        self.last_rx = data
        text = data.decode(encoding, errors="replace")
        lines = tuple(line for line in text.splitlines() if line)
        accepted: list[ReceivedLine] = []
        rules = self.workspace.effective_filters_for(self.window)

        for line in lines:
            result = apply_filters(line, rules)
            received = ReceivedLine(window_id=self.window.window_id, text=line, result=result)
            accepted.append(received)
            self.received_lines.append(received)
        return tuple(accepted)

    def read_available(self, size: int = 4096, encoding: str = "utf-8") -> tuple[ReceivedLine, ...]:
        data = self.transport.read(size)
        if not data:
            return ()
        return self.accept_rx_bytes(data, encoding=encoding)


@dataclass
class SerialWorkspaceService:
    workspace: SerialWorkspaceProfile
    sessions: dict[str, SerialWindowSession] = field(default_factory=dict)

    def attach_window(
        self,
        window: SerialWindowProfile,
        transport: SerialTransport,
        *,
        open_immediately: bool = False,
    ) -> SerialWindowSession:
        if window.window_id in self.sessions:
            raise ValueError(f"window session already exists: {window.window_id}")
        session = SerialWindowSession(workspace=self.workspace, window=window, transport=transport)
        if open_immediately:
            session.open()
        self.sessions[window.window_id] = session
        return session

    def detach_window(self, window_id: str) -> SerialWindowSession:
        try:
            session = self.sessions.pop(window_id)
        except KeyError as exc:
            raise KeyError(f"unknown window session: {window_id}") from exc
        if session.is_open:
            session.close()
        return session

    def get_session(self, window_id: str) -> SerialWindowSession:
        try:
            return self.sessions[window_id]
        except KeyError as exc:
            raise KeyError(f"unknown window session: {window_id}") from exc
