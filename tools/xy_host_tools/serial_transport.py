from __future__ import annotations

from dataclasses import dataclass, field
import os
from typing import Any, Protocol


@dataclass(frozen=True)
class SerialPortInfo:
    port: str
    description: str = ""
    hwid: str = ""


class SerialTransport(Protocol):
    @property
    def is_open(self) -> bool:
        ...

    def open(self) -> None:
        ...

    def close(self) -> None:
        ...

    def write(self, data: bytes) -> int:
        ...

    def read(self, size: int = 1) -> bytes:
        ...


@dataclass
class MemorySerialTransport:
    rx_buffer: bytearray = field(default_factory=bytearray)
    tx_buffer: bytearray = field(default_factory=bytearray)
    opened: bool = False

    @property
    def is_open(self) -> bool:
        return self.opened

    def open(self) -> None:
        self.opened = True

    def close(self) -> None:
        self.opened = False

    def write(self, data: bytes) -> int:
        if not self.opened:
            raise RuntimeError("serial transport is not open")
        self.tx_buffer.extend(data)
        return len(data)

    def read(self, size: int = 1) -> bytes:
        if not self.opened:
            raise RuntimeError("serial transport is not open")
        if size <= 0:
            return b""
        data = bytes(self.rx_buffer[:size])
        del self.rx_buffer[:size]
        return data

    def feed_rx(self, data: bytes) -> None:
        self.rx_buffer.extend(data)

    def drain_tx(self) -> bytes:
        data = bytes(self.tx_buffer)
        self.tx_buffer.clear()
        return data


@dataclass
class PySerialTransport:
    port: str
    baudrate: int = 115200
    timeout: float = 0.05
    write_timeout: float = 1.0
    serial_factory: Any | None = None
    _serial: Any | None = None

    @property
    def is_open(self) -> bool:
        return bool(self._serial and self._serial.is_open)

    def open(self) -> None:
        try:
            if self._serial is None:
                factory = self.serial_factory or _load_pyserial_factory()
                self._serial = factory(
                    port=self.port,
                    baudrate=self.baudrate,
                    timeout=self.timeout,
                    write_timeout=self.write_timeout,
                )
            elif not self._serial.is_open:
                self._serial.open()
        except Exception as exc:
            raise RuntimeError(describe_serial_open_error(self.port, exc)) from exc

    def close(self) -> None:
        if self._serial is not None and self._serial.is_open:
            self._serial.close()

    def write(self, data: bytes) -> int:
        if not self.is_open:
            raise RuntimeError("serial transport is not open")
        serial_port = self._serial
        if serial_port is None:
            raise RuntimeError("serial transport is not open")
        return int(serial_port.write(data))

    def read(self, size: int = 1) -> bytes:
        if not self.is_open:
            raise RuntimeError("serial transport is not open")
        if size <= 0:
            return b""
        serial_port = self._serial
        if serial_port is None:
            raise RuntimeError("serial transport is not open")
        return bytes(serial_port.read(size))


def _load_pyserial_factory() -> Any:
    try:
        import serial  # type: ignore[import-not-found]
    except ImportError as exc:
        raise RuntimeError("pyserial is required for real serial ports; install the 'pyserial' package") from exc
    return serial.Serial


def describe_serial_open_error(port: str, exc: Exception) -> str:
    message = str(exc)
    details = [f"could not open serial port {port}: {message}"]
    if _looks_like_permission_denied(exc):
        group_hint = ""
        try:
            stat_info = os.stat(port)
            import grp

            group_hint = grp.getgrgid(stat_info.st_gid).gr_name
        except Exception:
            group_hint = "dialout"
        if group_hint:
            details.append(f"permission denied; add the user to the '{group_hint}' group and log out/in")
        else:
            details.append("permission denied; check device group permissions and log out/in after group changes")
    return "; ".join(details)


def _looks_like_permission_denied(exc: Exception) -> bool:
    errno_value = getattr(exc, "errno", None)
    if errno_value == 13:
        return True
    return "Permission denied" in str(exc) or "Errno 13" in str(exc)


def list_serial_ports() -> tuple[SerialPortInfo, ...]:
    try:
        from serial.tools import list_ports  # type: ignore[import-not-found]
    except ImportError:
        return ()

    return tuple(
        SerialPortInfo(port=item.device, description=item.description, hwid=item.hwid)
        for item in list_ports.comports()
    )
