#!/usr/bin/env python3
"""Build, program, read back, and check the openCH CH32V307 UART image.

WCH-Link exposes CH32V30x code flash at logical address 0.  The image is
therefore programmed at address 0, not at a Cortex-M-style 0x08000000.
The default is a dry run; pass ``--yes`` before any write is attempted.
"""

from __future__ import annotations

import argparse
import hashlib
import shutil
import subprocess
import sys
import time
from pathlib import Path

# OpenOCD comes from PATH when it is there, so this tool is not pinned to the machine it was written on (the
# vendor default below is a Windows install of MounRiver Studio). The vendor paths stay as the fallback
# because that is what those machines rely on.
_VENDOR_MRS_ROOT = Path("D:/MounRiver/MounRiver_Studio2")
_VENDOR_OPENOCD = Path(
    "D:/MounRiver/MounRiver_Studio2/resources/app/resources/win32/"
    "components/WCH/OpenOCD/OpenOCD/bin/openocd.exe"
)
DEFAULT_OPENOCD = Path(shutil.which("openocd")) if shutil.which("openocd") else _VENDOR_OPENOCD
DEFAULT_MRS_ROOT = _VENDOR_MRS_ROOT
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
    parser.add_argument(
        "--image",
        type=Path,
        help=(
            "image to program: .bin, .hex or .elf. Every openCH target sets SUFFIX \".elf\" and "
            "only some add an objcopy step, so most of the 68 targets ship an ELF and no .bin; "
            "OpenOCD programs an ELF directly and ignores the load address for it"
        ),
    )
    parser.add_argument("--port", help="WCH-Link virtual COM port for runtime check")
    parser.add_argument("--samples", type=int, default=10)
    parser.add_argument("--interval-ms", type=int, default=100)
    parser.add_argument("--post-reset-delay-ms", type=int, default=1000)
    parser.add_argument("--openocd", type=Path, default=DEFAULT_OPENOCD)
    parser.add_argument("--mrs-root", type=Path, default=DEFAULT_MRS_ROOT)
    parser.add_argument("--config", type=Path)
    parser.add_argument("--log", type=Path, default=Path("opench_uart_runtime.log"))
    parser.add_argument(
        "--firmware-commit",
        help="exact source commit baked into the image; defaults to the repo HEAD",
    )
    parser.add_argument(
        "--expect",
        action="append",
        metavar="MARKER",
        help="expected runtime marker for the runtime check; repeatable and ordered",
    )
    parser.add_argument(
        "--adapter-speed",
        type=int,
        default=1000,
        help=(
            "SDI clock in kHz applied after the config is read (default 1000). The shipped "
            "wch-riscv.cfg sets 6000, which fails DM examination on this WCH-Link-CH549 "
            "(dmstatus=0x80000000) and aborts the OpenOCD fork mid-assert"
        ),
    )
    parser.add_argument("--yes", action="store_true", help="actually program; default is dry-run")
    args = parser.parse_args()

    if args.samples < 1 or args.interval_ms < 0 or args.post_reset_delay_ms < 0:
        parser.error("samples must be positive and delays must be non-negative")
    if args.adapter_speed <= 0:
        parser.error("adapter speed must be positive")
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
        f"adapter speed {args.adapter_speed}",
        "-c",
        f"program {openocd_tcl_path(image)} 0x{FLASH_ADDRESS:08X} verify reset exit",
    ]
    run(command, root)

    if args.port:
        if args.post_reset_delay_ms:
            time.sleep(args.post_reset_delay_ms / 1000.0)
        log_path = args.log if args.log.is_absolute() else root / args.log
        # The runtime check must speak the board capture helper's own interface:
        # it takes a positional port plus an explicit firmware commit, and it
        # writes both the payload log and a metadata sidecar itself.
        commit = args.firmware_commit or subprocess.run(
            ["git", "rev-parse", "HEAD"],
            cwd=root,
            check=True,
            capture_output=True,
            text=True,
        ).stdout.strip()
        metadata_path = log_path.with_suffix(".meta.json")
        capture = [
            sys.executable,
            str(root / "boards/opench_ch32v307/capture_uart.py"),
            args.port,
            "--firmware-commit",
            commit,
            "--output",
            str(log_path),
            "--metadata",
            str(metadata_path),
            "--duration-ms",
            str(max(1, args.samples * max(args.interval_ms, 1))),
        ]
        for marker in args.expect or []:
            capture.extend(["--expect", marker])
        print("$", subprocess.list2cmdline(capture))
        subprocess.run(capture, cwd=root, check=True)
        print(f"Runtime log: {log_path}")
        print(f"Runtime metadata: {metadata_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
