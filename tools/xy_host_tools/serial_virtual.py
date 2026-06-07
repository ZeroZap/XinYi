from __future__ import annotations

import os
import pty
import select
import tty
from dataclasses import dataclass


@dataclass
class PtySerialTransport:
    path: str
    timeout: float = 0.05
    _fd: int | None = None

    @property
    def is_open(self) -> bool:
        return self._fd is not None

    def open(self) -> None:
        if self._fd is None:
            self._fd = os.open(self.path, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)

    def close(self) -> None:
        if self._fd is not None:
            os.close(self._fd)
            self._fd = None

    def write(self, data: bytes) -> int:
        fd = self._require_open()
        return os.write(fd, data)

    def read(self, size: int = 1) -> bytes:
        fd = self._require_open()
        if size <= 0:
            return b""
        readable, _, _ = select.select([fd], [], [], self.timeout)
        if not readable:
            return b""
        try:
            return os.read(fd, size)
        except BlockingIOError:
            return b""

    def _require_open(self) -> int:
        if self._fd is None:
            raise RuntimeError("serial transport is not open")
        return self._fd


@dataclass
class VirtualSerialPair:
    host_path: str
    device_path: str
    host_transport: PtySerialTransport
    device_transport: PtySerialTransport
    _master_a: int
    _master_b: int
    _closed: bool = False

    @classmethod
    def create(cls, timeout: float = 0.05) -> "VirtualSerialPair":
        master_a, slave_a = pty.openpty()
        master_b, slave_b = pty.openpty()
        tty.setraw(slave_a)
        tty.setraw(slave_b)
        host_path = os.ttyname(slave_a)
        device_path = os.ttyname(slave_b)
        os.close(slave_a)
        os.close(slave_b)
        bridge_pty_masters(master_a, master_b)
        return cls(
            host_path=host_path,
            device_path=device_path,
            host_transport=PtySerialTransport(host_path, timeout=timeout),
            device_transport=PtySerialTransport(device_path, timeout=timeout),
            _master_a=master_a,
            _master_b=master_b,
        )

    def close(self) -> None:
        if self._closed:
            return
        self.host_transport.close()
        self.device_transport.close()
        os.close(self._master_a)
        os.close(self._master_b)
        self._closed = True

    def __enter__(self) -> "VirtualSerialPair":
        return self

    def __exit__(self, exc_type, exc, tb) -> None:
        self.close()


def bridge_pty_masters(master_a: int, master_b: int) -> None:
    for master in (master_a, master_b):
        os.set_blocking(master, False)


def pump_virtual_pair(pair: VirtualSerialPair, *, max_bytes: int = 4096, timeout: float = 0.05) -> int:
    return pump_pty_masters(pair._master_a, pair._master_b, max_bytes=max_bytes, timeout=timeout)


def pump_pty_masters(master_a: int, master_b: int, *, max_bytes: int = 4096, timeout: float = 0.05) -> int:
    moved = 0
    readable, _, _ = select.select([master_a, master_b], [], [], timeout)
    for source in readable:
        target = master_b if source == master_a else master_a
        while True:
            try:
                data = os.read(source, max_bytes)
            except BlockingIOError:
                break
            if not data:
                break
            os.write(target, data)
            moved += len(data)
            if len(data) < max_bytes:
                break
    return moved
