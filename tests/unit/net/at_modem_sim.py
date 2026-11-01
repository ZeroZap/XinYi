#!/usr/bin/env python3
"""An AT modem that lives on a pseudo-terminal, so the AT client can be tested without one.

XinYi's AT client has to survive a modem that is slow, wedged, chatty, or splitting its answers across
reads. None of those are reachable with hardware in the loop on a desk, and none of them are reachable at
all in a gate. This module is the modem: a byte-stream responder that speaks enough of EC2X, SIM76xx and
ESP-AT to exercise the client's command, prompt, URC and recovery paths.

Behaviour was informed by the AT server simulator in `ZeroZap/bareos`
(`tool/at_server_sim/`, commit 40c50707), which is a reference and not a dependency: this is XinYi's own
implementation, keeps only what XinYi's client needs, and is deliberately smaller. Nothing is vendored
from it and no code is shared.

Two things make it usable:
  * it is transport-agnostic - it takes a `write` callable and a `feed(bytes)` method, so tests drive it
    in-process or over a pty;
  * `PtyTransport` needs only the standard library, so no pyserial and no serial port.
"""

from __future__ import annotations

import os
import pty
import re
import time
import tty
from dataclasses import dataclass, field
from typing import Callable, Pattern

WriteFn = Callable[[bytes], None]


@dataclass
class ModemProfile:
    """What a modem answers. Command patterns are matched in order; first match wins."""

    name: str
    #: (pattern, response lines) pairs. A response of None means "stay silent" - a wedged modem.
    responses: list[tuple[Pattern[str], list[str] | None]] = field(default_factory=list)
    #: The keyword this modem uses to open a length-framed upload, and what it answers afterwards.
    send_prompt: Pattern[str] | None = None
    send_result: str = "SEND OK"


@dataclass
class ModemConfig:
    """Everything about the modem that is not the profile: how it behaves when things go wrong."""

    fragment_size: int = 0
    fragment_delay_ms: int = 0
    drop_commands: list[Pattern[str]] = field(default_factory=list)
    error_commands: list[Pattern[str]] = field(default_factory=list)
    echo: bool = False


def _profile(name: str, table: list[tuple[str, list[str] | None]], send: str | None,
             result: str) -> ModemProfile:
    return ModemProfile(
        name=name,
        responses=[(re.compile(pattern), lines) for pattern, lines in table],
        send_prompt=re.compile(send) if send else None,
        send_result=result,
    )


#: The three modems this project might put behind the AT client. Deliberately short: a command that no
#: XinYi test issues is not listed, because a simulator that answers everything answers nothing about the
#: client.
PROFILES: dict[str, ModemProfile] = {
    "ec2x": _profile("ec2x", [
        (r"^AT$", ["OK"]),
        (r"^ATE[01]$", ["OK"]),
        (r"^AT\+CSQ$", ["+CSQ: 18,0", "OK"]),
        (r"^AT\+CREG\?$", ["+CREG: 0,1", "OK"]),
        (r"^AT\+CEREG\?$", ["+CEREG: 0,1", "OK"]),
        (r"^AT\+CIMI$", ["460001234567890", "OK"]),
        (r"^AT\+CCID$", ["89860000000000000000", "OK"]),
        (r"^AT\+ICCID$", ["89860000000000000000", "OK"]),
        (r"^AT\+CMEE=\d$", ["OK"]),
        (r"^AT\+CGATT\?$", ["+CGATT: 1", "OK"]),
        (r"^AT\+QIOPEN=", ["OK", "+QIOPEN: 0,0"]),
        (r"^AT\+QICLOSE=", ["OK"]),
    ], send=r"^AT\+QISEND=(\d+),(\d+)$", result="SEND OK"),
    "sim76": _profile("sim76", [
        (r"^AT$", ["OK"]),
        (r"^ATE[01]$", ["OK"]),
        (r"^AT\+CSQ$", ["+CSQ: 22,99", "OK"]),
        (r"^AT\+CREG\?$", ["+CREG: 0,1", "OK"]),
        (r"^AT\+CIMI$", ["460001234567890", "OK"]),
        (r"^AT\+CIPSTART=", ["OK", "CONNECT OK"]),
        (r"^AT\+CIPCLOSE=", ["CLOSE OK"]),
    ], send=r"^AT\+CIPSEND=(\d+),(\d+)$", result="SEND OK"),
    "esp_at": _profile("esp_at", [
        (r"^AT$", ["OK"]),
        (r"^ATE[01]$", ["OK"]),
        (r"^AT\+RST$", ["OK", "ready"]),
        (r"^AT\+CWJAP=", ["WIFI CONNECTED", "WIFI GOT IP", "OK"]),
        (r"^AT\+CIPSTART=", ["CONNECT", "OK"]),
        (r"^AT\+CIPCLOSE$", ["CLOSED", "OK"]),
    ], send=r"^AT\+CIPSEND=(\d+)$", result="SEND OK"),
}


class AtModem:
    """An incremental AT responder: feed it bytes, it writes whatever a modem would write back."""

    def __init__(self, write: WriteFn, profile: ModemProfile | None = None,
                 config: ModemConfig | None = None,
                 sleep: Callable[[float], None] = time.sleep) -> None:
        self.write = write
        self.profile = profile or PROFILES["ec2x"]
        self.config = config or ModemConfig()
        self.sleep = sleep
        self._command = bytearray()
        self._skip_lf = False
        self._upload_remaining = 0
        self._upload_buffer = bytearray()
        self._upload_result = ""
        self.commands: list[str] = []
        self.uploads: list[bytes] = []

    # -- byte stream -------------------------------------------------------------------------------

    def feed(self, data: bytes) -> None:
        """Consume a serial chunk, which may hold a command, upload bytes, or neither in full."""
        offset = 0
        while offset < len(data):
            if self._skip_lf:
                self._skip_lf = False
                if data[offset] == 0x0A:
                    offset += 1
                    continue

            if self._upload_remaining:
                take = min(self._upload_remaining, len(data) - offset)
                self._upload_buffer.extend(data[offset:offset + take])
                self._upload_remaining -= take
                offset += take
                if self._upload_remaining == 0:
                    self.uploads.append(bytes(self._upload_buffer))
                    self._upload_buffer.clear()
                    self._send(f"\r\n{self._upload_result}\r\n")
                continue

            byte = data[offset]
            offset += 1
            if byte in (0x0D, 0x0A):
                if self._command:
                    line = bytes(self._command).decode("ascii", errors="replace")
                    self._command.clear()
                    self._skip_lf = byte == 0x0D
                    self._handle(line)
            else:
                self._command.append(byte)

    def inject(self, text: str) -> None:
        """Send an unsolicited result code, as a modem does whenever it feels like it."""
        self._send(f"\r\n{text}\r\n")

    # -- internals ---------------------------------------------------------------------------------

    def _send(self, text: str) -> None:
        data = text.encode("ascii", errors="replace")
        if self.config.fragment_size > 0:
            for start in range(0, len(data), self.config.fragment_size):
                self.write(data[start:start + self.config.fragment_size])
                if self.config.fragment_delay_ms:
                    self.sleep(self.config.fragment_delay_ms / 1000.0)
            return
        self.write(data)

    def _matches(self, patterns: list[Pattern[str]]) -> bool:
        return any(pattern.search(self.commands[-1]) for pattern in patterns)

    def _handle(self, line: str) -> None:
        self.commands.append(line)
        if self.config.echo:
            self._send(f"{line}\r\n")

        if self._matches(self.config.drop_commands):
            return  # a wedged modem: accepted on the wire, never answered
        if self._matches(self.config.error_commands):
            self._send("\r\nERROR\r\n")
            return

        prompt = self.profile.send_prompt
        if prompt is not None:
            match = prompt.search(line)
            if match:
                length = int(match.group(match.lastindex or 1))
                self._upload_remaining = length
                self._upload_result = self.profile.send_result
                self._send("\r\n> ")
                return

        for pattern, lines in self.profile.responses:
            if pattern.search(line):
                if lines is None:
                    return
                # A real modem puts a blank line between the information lines and the final result, so
                # a client that keys on the empty line sees the same thing here as on hardware.
                if len(lines) > 1:
                    self._send("\r\n" + "\r\n".join(lines[:-1]) + "\r\n\r\n" + lines[-1] + "\r\n")
                else:
                    self._send("\r\n" + lines[0] + "\r\n")
                return

        self._send("\r\nERROR\r\n")


class PtyTransport:
    """A pseudo-terminal standing in for a modem's serial port, using only the standard library."""

    def __init__(self) -> None:
        self.master, self.slave = pty.openpty()
        tty.setraw(self.slave)
        tty.setraw(self.master)
        os.set_blocking(self.master, False)
        self.path = os.ttyname(self.slave)

    def read(self, size: int = 4096) -> bytes:
        try:
            return os.read(self.master, size)
        except (BlockingIOError, OSError):
            return b""

    def write(self, data: bytes) -> None:
        try:
            os.write(self.master, data)
        except OSError:
            pass

    def close(self) -> None:
        for descriptor in (self.master, self.slave):
            try:
                os.close(descriptor)
            except OSError:
                pass
