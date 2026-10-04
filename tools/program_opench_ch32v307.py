#!/usr/bin/env python3
"""Build, program, read back, and check the openCH CH32V307 UART image.

WCH-Link exposes CH32V30x code flash at logical address 0.  The image is
therefore programmed at address 0, not at a Cortex-M-style 0x08000000.
The default is a dry run; pass ``--yes`` before any write is attempted.
"""

from __future__ import annotations

import argparse
import hashlib
import subprocess
import sys
import time
from pathlib import Path

DEFAULT_OPENOCD = Path(
    "D:/MounRiver/MounRiver_Studio2/resources/app/resources/win32/"
    "components/WCH/OpenOCD/OpenOCD/bin/openocd.exe"
)
DEFAULT_MRS_ROOT = Path("D:/MounRiver/MounRiver_Studio2")
FLASH_ADDRESS = 0x00000000


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def openocd_tcl_path(path: Path) -> str:
    """Use forward slashes because backslashes are Tcl escapes on Windows."""
    return path.resolve().as_posix()


def run(command: list[str], cwd: Path) -> None:
    print("$", subprocess.list2cmdline(command))
    subprocess.run(command, cwd=cwd, check=True)


def main() -> int:
    root = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--image", type=Path)
    parser.add_argument("--port", help="WCH-Link virtual COM port for runtime check")
    parser.add_argument("--samples", type=int, default=10)
    parser.add_argument("--interval-ms", type=int, default=100)
    parser.add_argument("--post-reset-delay-ms", type=int, default=1000)
    parser.add_argument("--openocd", type=Path, default=DEFAULT_OPENOCD)
    parser.add_argument("--mrs-root", type=Path, default=DEFAULT_MRS_ROOT)
    parser.add_argument("--config", type=Path)
    parser.add_argument("--log", type=Path, default=Path("opench_uart_runtime.log"))
    parser.add_argument("--yes", action="store_true", help="actually program; default is dry-run")
    args = parser.parse_args()

    if args.samples < 1 or args.interval_ms < 0 or args.post_reset_delay_ms < 0:
        parser.error("samples must be positive and delays must be non-negative")
    image = args.image or (
        root / "build/opench-uart/boards/opench_ch32v307/opench_ch32v307_uart_smoke.bin"
    )
    image = image if image.is_absolute() else root / image
    config = args.config or (args.openocd.parent / "wch-riscv.cfg")
    config = config if config.is_absolute() else root / config

    if not image.is_file():
        parser.error(f"image not found: {image}")
    if not args.yes:
        if not args.openocd.is_file():
            print(f"NOTE: Windows OpenOCD path not present on this host: {args.openocd}")
        if not config.is_file():
            print(f"NOTE: Windows WCH config path not present on this host: {config}")
    else:
        if not args.openocd.is_file():
            parser.error(f"OpenOCD not found: {args.openocd}")
        if not config.is_file():
            parser.error(f"WCH config not found: {config}")

    print(f"Image:   {image}")
    print(f"SHA256:  {sha256(image)}")
    print(f"Size:    {image.stat().st_size} bytes")
    print(f"Address: 0x{FLASH_ADDRESS:08X} (WCH logical flash address)")
    print(f"Config:  {config}")
    if not args.yes:
        print("DRY RUN: no Flash write performed; pass --yes to program")
        return 0

    command = [
        str(args.openocd),
        "-f",
        str(config),
        "-c",
        f"program {openocd_tcl_path(image)} 0x{FLASH_ADDRESS:08X} verify reset exit",
    ]
    run(command, root)

    if args.port:
        if args.post_reset_delay_ms:
            time.sleep(args.post_reset_delay_ms / 1000.0)
        log_path = args.log if args.log.is_absolute() else root / args.log
        capture = [
            sys.executable,
            str(root / "boards/opench_ch32v307/capture_uart.py"),
            args.port,
            "--samples",
            str(args.samples),
            "--interval-ms",
            str(args.interval_ms),
        ]
        with log_path.open("w", encoding="utf-8", newline="\n") as stream:
            print("$", subprocess.list2cmdline(capture))
            subprocess.run(capture, cwd=root, stdout=stream, stderr=subprocess.STDOUT, check=True)
        print(f"Runtime log: {log_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
