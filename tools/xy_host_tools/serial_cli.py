from __future__ import annotations

import argparse
import sys
from typing import Sequence

from .serial_config import ActionButton, FilterRule, SerialWindowProfile, SerialWorkspaceProfile
from .serial_profile import save_workspace_profile
from .serial_service import SerialWorkspaceService
from .serial_transport import MemorySerialTransport, list_serial_ports
from .serial_virtual import VirtualSerialPair, pump_virtual_pair


DEFAULT_WORKSPACE = SerialWorkspaceProfile(
    name="XinYi Serial Demo",
    filters=(
        FilterRule(
            name="error",
            keywords=("ERROR", "FAIL", "HardFault"),
            match="any",
            foreground="white",
            background="red",
            priority=100,
        ),
        FilterRule(
            name="boot",
            keywords=("boot", "version", "fw"),
            match="any",
            foreground="cyan",
            background="default",
            priority=10,
        ),
    ),
    buttons=(
        ActionButton(name="version", label="版本", mode="text", payload="version\r\n"),
        ActionButton(name="boot", label="进入 Boot", mode="script", payload='return "boot\\r\\n"'),
    ),
)


def _demo_lines() -> tuple[str, ...]:
    return (
        "Boot FW=0.1.0",
        "sensor init ok",
        "ERROR uart timeout",
        "net connected",
        "HardFault at 0x08001234",
    )


def run_filter_demo(lines: Sequence[str] | None = None) -> list[str]:
    window = SerialWindowProfile(window_id="demo", title="Demo", port="virtual")
    service = SerialWorkspaceService(DEFAULT_WORKSPACE)
    session = service.attach_window(window, MemorySerialTransport(), open_immediately=True)
    rendered: list[str] = []

    for line in lines or _demo_lines():
        for received in session.accept_rx_bytes((line + "\n").encode("utf-8")):
            result = received.result
            rendered.append(
                f"fg={result.foreground} bg={result.background} rules={','.join(result.matched_rules) or '-'} | {line}"
            )
    return rendered


def run_send_demo(button_name: str) -> bytes:
    window = SerialWindowProfile(window_id="demo", title="Demo", port="virtual")
    transport = MemorySerialTransport()
    service = SerialWorkspaceService(DEFAULT_WORKSPACE)
    session = service.attach_window(window, transport, open_immediately=True)
    session.send_button(button_name)
    return transport.drain_tx()


def _print_serial_ports() -> int:
    ports = list_serial_ports()
    if not ports:
        print("no serial ports found or pyserial is not installed")
        return 0
    for port in ports:
        detail = f" {port.description}" if port.description else ""
        hwid = f" [{port.hwid}]" if port.hwid else ""
        print(f"{port.port}{detail}{hwid}")
    return 0


def run_virtual_smoke() -> tuple[str, ...]:
    window = SerialWindowProfile(window_id="virtual", title="Virtual PTY", port="pty")
    service = SerialWorkspaceService(DEFAULT_WORKSPACE)
    rendered: list[str] = []

    with VirtualSerialPair.create() as pair:
        session = service.attach_window(window, pair.host_transport, open_immediately=True)
        pair.device_transport.open()

        session.send_button("version")
        pump_virtual_pair(pair)
        command = pair.device_transport.read(128)
        rendered.append(f"host={pair.host_path} device={pair.device_path}")
        rendered.append(f"device_rx={command.hex()}")

        pair.device_transport.write(b"Boot FW=0.1.0\nERROR virtual timeout\n")
        pump_virtual_pair(pair)
        for received in session.read_available(512):
            result = received.result
            rendered.append(
                f"rx fg={result.foreground} bg={result.background} "
                f"rules={','.join(result.matched_rules) or '-'} | {received.text}"
            )
    return tuple(rendered)


def main(argv: Sequence[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="XinYi host serial tool prototype")
    subparsers = parser.add_subparsers(dest="command", required=True)
    subparsers.add_parser("demo-filter", help="run a deterministic filter demo without serial hardware")
    subparsers.add_parser("gui", help="start the z-serial GUI shell")
    subparsers.add_parser("gui-smoke", help="run an offscreen z-serial GUI smoke test when PySide6 is available")
    subparsers.add_parser("list", help="list serial ports when pyserial is available")
    subparsers.add_parser("virtual-smoke", help="verify service send/read through a Linux PTY virtual serial pair")

    sample_profile = subparsers.add_parser("sample-profile", help="write a JSON sample workspace profile")
    sample_profile.add_argument("path", help="output JSON profile path")

    send_demo = subparsers.add_parser("send-demo", help="render and send a demo button through memory transport")
    send_demo.add_argument("button", choices=("version", "boot"), help="demo button name")

    args = parser.parse_args(argv)
    if args.command == "demo-filter":
        for line in run_filter_demo():
            print(line)
        return 0
    if args.command == "gui":
        from .gui.z_serial_app import main as gui_main

        try:
            return gui_main([])
        except RuntimeError as exc:
            print(str(exc), file=sys.stderr)
            return 1
    if args.command == "gui-smoke":
        from .gui.z_serial_app import run_offscreen_smoke

        try:
            for line in run_offscreen_smoke():
                print(line)
            return 0
        except RuntimeError as exc:
            print(str(exc), file=sys.stderr)
            return 1
    if args.command == "list":
        return _print_serial_ports()
    if args.command == "virtual-smoke":
        for line in run_virtual_smoke():
            print(line)
        return 0
    if args.command == "sample-profile":
        window = SerialWindowProfile(window_id="u5", title="U5 Debug", port="/dev/ttyUSB0")
        save_workspace_profile(args.path, DEFAULT_WORKSPACE, windows=(window,))
        print(args.path)
        return 0
    if args.command == "send-demo":
        print(run_send_demo(args.button).hex())
        return 0
    return 2


if __name__ == "__main__":
    raise SystemExit(main())
