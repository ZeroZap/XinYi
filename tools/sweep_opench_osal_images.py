#!/usr/bin/env python3
"""Sweep the openCH OSAL images with debugger evidence instead of the UART.

The WCH-Link virtual COM port keeps a ~150-260 byte ring and drops bytes out of
order, so markers captured over it cannot decide anything: a 214 byte window
holds two or three markers and on one run even replayed the previous image's
text. This script therefore asks the debugger instead:

  * flash and verify the ELF,
  * let it run, then halt and read `xTickCount` twice with a resume between,
  * resolve `pc` against the ELF to see which function the core actually sits in,
  * read `uxCurrentNumberOfTasks`.

Advancing ticks mean the scheduler is alive; an idle-task pc after the work has
finished means the image is not spinning in a failure trap. That is a liveness
verdict, not a PASS verdict - saying an image passed needs in-image evidence,
which only an instrumented counter can provide over this link.
"""

from __future__ import annotations

import argparse
import pathlib
import os
import re
import subprocess
import sys
import time

WCH = pathlib.Path(
    "/usr/share/MRS2/MRS-linux-x64/resources/app/resources/linux/components/WCH"
)
OPENOCD = WCH / "OpenOCD/OpenOCD/bin/openocd"
CONFIG = WCH / "OpenOCD/OpenOCD/bin/wch-riscv.cfg"
TOOLCHAIN = WCH / "Toolchain/RISC-V Embedded GCC15/bin/riscv32-wch-elf"

BOARD = pathlib.Path("build/opench-osal-verify/boards/opench_ch32v307")


def run_openocd(commands: list[str]) -> str:
    argv = [str(OPENOCD), "-f", str(CONFIG), "-c", "adapter speed 1000"]
    for command in commands:
        argv += ["-c", command]
    argv += ["-c", "shutdown"]
    completed = subprocess.run(argv, capture_output=True, text=True, timeout=180)
    return completed.stdout + completed.stderr


def symbol_address(elf: pathlib.Path, name: str) -> int | None:
    completed = subprocess.run(
        [f"{TOOLCHAIN}-nm", str(elf)], capture_output=True, text=True, timeout=120
    )
    for line in completed.stdout.splitlines():
        parts = line.split()
        if len(parts) == 3 and parts[2] == name:
            return int(parts[0], 16)
    return None


def pc_symbol(elf: pathlib.Path, pc: int) -> str:
    completed = subprocess.run(
        [f"{TOOLCHAIN}-addr2line", "-f", "-e", str(elf), hex(pc)],
        capture_output=True,
        text=True,
        timeout=120,
    )
    return completed.stdout.splitlines()[0] if completed.stdout.strip() else "?"


def read_word(address: int) -> int | None:
    output = run_openocd(["init", "halt", f"mdw {hex(address)} 1"])
    match = re.search(rf"{address:#010x}:\s+([0-9a-fA-F]{{8}})", output)
    return int(match.group(1), 16) if match else None


def read_marker_string(address: int, limit: int = 64) -> str:
    """Pull the marker text the image pointed at out of flash."""
    output = run_openocd(["init", "halt", f"mdb {hex(address)} {limit}"])
    text = bytearray()
    for line in output.splitlines():
        if not re.match(r"^0x[0-9a-fA-F]+:", line):
            continue
        body = line.split(":", 1)[1]
        text += bytes(int(pair, 16) for pair in re.findall(r"\b[0-9a-fA-F]{2}\b", body))
    return text.split(b"\x00", 1)[0].decode("ascii", "replace").strip()


def read_pc() -> int | None:
    output = run_openocd(["init", "halt", "reg pc"])
    match = re.search(r"pc \(/32\):\s+(0x[0-9a-fA-F]+)", output)
    return int(match.group(1), 16) if match else None


def drain_uart(port: str) -> None:
    """Throw away whatever the previous image left in the virtual COM buffer."""
    try:
        handle = open(port, "rb", buffering=0)
    except OSError:
        return
    try:
        os.set_blocking(handle.fileno(), False)
        handle.read(65536)
    finally:
        handle.close()


def capture_uart(port: str, seconds: float) -> str:
    """Read the virtual COM port for a while, tolerating its tiny ring buffer."""
    try:
        handle = open(port, "rb", buffering=0)
    except OSError:
        return ""
    text = bytearray()
    deadline = time.time() + seconds
    try:
        os.set_blocking(handle.fileno(), False)
        while time.time() < deadline:
            chunk = handle.read(4096)
            if chunk:
                text += chunk
            else:
                time.sleep(0.02)
    finally:
        handle.close()
    return text.decode("ascii", "replace")


def sweep(name: str, uart: str | None = None) -> dict:
    elf = BOARD / f"opench_ch32v307_{name}.elf"
    if not elf.is_file():
        return {"image": name, "error": f"missing {elf}"}

    ticks_address = symbol_address(elf, "xTickCount")
    tasks_address = symbol_address(elf, "uxCurrentNumberOfTasks")
    if ticks_address is None:
        return {"image": name, "error": "no xTickCount symbol"}

    if uart:
        drain_uart(uart)
    output = run_openocd(
        [
            "init",
            f"program {elf} 0x00000000 verify",
            "reset halt",
            "resume",
        ]
    )
    if "** Programming Finished **" not in output:
        return {"image": name, "error": f"program failed: {output.strip()[-200:]}"}

    time.sleep(2)
    first = read_word(ticks_address)
    pc = read_pc()
    tasks = read_word(tasks_address) if tasks_address else None
    run_openocd(["init", "resume"])
    time.sleep(1)
    second = read_word(ticks_address)

    steps_address = symbol_address(elf, "xy_image_steps")
    failed_address = symbol_address(elf, "xy_image_failed")
    last_address = symbol_address(elf, "xy_image_last")

    markers: list[str] = []
    if uart:
        captured = capture_uart(uart, 3.0)
        markers = sorted(set(re.findall(r"OPENCH_OSAL_[A-Z0-9_]*", captured)))

    steps = read_word(steps_address) if steps_address else None
    failed = read_word(failed_address) if failed_address else None
    last_pointer = read_word(last_address) if last_address else None
    last_marker = read_marker_string(last_pointer) if last_pointer else ""

    return {
        "image": name,
        "steps": steps,
        "failed": failed,
        "last_marker": last_marker,
        "markers": markers,
        "ticks": (first, second),
        "advanced": (first is not None and second is not None and second > first),
        "tasks": tasks,
        "pc": pc,
        "pc_symbol": pc_symbol(elf, pc) if pc is not None else "?",
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("images", nargs="+")
    parser.add_argument("--uart", help="serial port to sample for markers as well")
    arguments = parser.parse_args()

    failures = 0
    for name in arguments.images:
        result = sweep(name, arguments.uart)
        if "error" in result:
            print(f"{name}: ERROR {result['error']}")
            failures += 1
            continue
        first, second = result["ticks"]
        steps = result["steps"]
        failed = result["failed"]
        if failed:
            verdict = "FAILED"
        elif result["advanced"] and steps:
            verdict = "ok"
        elif not result["advanced"]:
            verdict = "NOT ADVANCING"
        else:
            verdict = "NO MARKERS"
        print(
            f"{result['image']}: {verdict} steps={steps} failed={failed} "
            f"ticks={first}->{second} pc={result['pc_symbol']} "
            f"last={result['last_marker']}"
        )
        if result['markers']:
            print(f"  uart (unreliable): {' '.join(result['markers'])}")
        if verdict != "ok":
            failures += 1

    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
