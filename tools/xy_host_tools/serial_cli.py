from __future__ import annotations

import argparse
from typing import Sequence

from .serial_actions import render_button_payload
from .serial_config import ActionButton, FilterRule, SerialWindowProfile, SerialWorkspaceProfile
from .serial_filter import apply_filters
from .serial_profile import save_workspace_profile
from .serial_transport import MemorySerialTransport, list_serial_ports


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
    rules = DEFAULT_WORKSPACE.effective_filters_for(window)
    rendered: list[str] = []

    for line in lines or _demo_lines():
        result = apply_filters(line, rules)
        if not result.visible:
            continue
        rendered.append(
            f"fg={result.foreground} bg={result.background} rules={','.join(result.matched_rules) or '-'} | {line}"
        )
    return rendered


def run_send_demo(button_name: str) -> bytes:
    window = SerialWindowProfile(window_id="demo", title="Demo", port="virtual")
    buttons = {button.name: button for button in DEFAULT_WORKSPACE.effective_buttons_for(window)}
    if button_name not in buttons:
        raise ValueError(f"unknown demo button: {button_name}")

    transport = MemorySerialTransport()
    transport.open()
    payload = render_button_payload(buttons[button_name], context={"port": window.port, "window_id": window.window_id})
    transport.write(payload)
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


def main(argv: Sequence[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="XinYi host serial tool prototype")
    subparsers = parser.add_subparsers(dest="command", required=True)
    subparsers.add_parser("demo-filter", help="run a deterministic filter demo without serial hardware")
    subparsers.add_parser("list", help="list serial ports when pyserial is available")

    sample_profile = subparsers.add_parser("sample-profile", help="write a JSON sample workspace profile")
    sample_profile.add_argument("path", help="output JSON profile path")

    send_demo = subparsers.add_parser("send-demo", help="render and send a demo button through memory transport")
    send_demo.add_argument("button", choices=("version", "boot"), help="demo button name")

    args = parser.parse_args(argv)
    if args.command == "demo-filter":
        for line in run_filter_demo():
            print(line)
        return 0
    if args.command == "list":
        return _print_serial_ports()
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
