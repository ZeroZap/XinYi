#!/usr/bin/env python3
"""Find the receive path's real overflow threshold instead of estimating it.

The ring size and the drain period are a budget: at 115200 a byte costs 86.8 us, so
256 bytes is 22 ms of line time. That arithmetic says a task draining every 20 ms
should survive a saturated line and a task draining every 30 ms should not. Arithmetic
is not evidence, so this sweeps the period and counts the overflows that actually
happen.

The probe reads xy_probe_rx_task_period_ms out of RAM, and the counters are plain
globals, so a whole sweep costs one flash: write the period, zero the counters, let the
host write at full speed for a fixed window, read what happened.

Usage:
    python3 boards/opench_ch32v307/sweep_rx_overflow.py --openocd <ocd> --config <cfg>
        --nm <nm> --elf <probe.elf> --uart /dev/ttyACM0
"""

from __future__ import annotations

import argparse
import os
import subprocess
import sys
import termios
import threading
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import validate_osal_probe as probe  # noqa: E402

COUNTERS = (
    "xy_probe_rx_irq_bytes",
    "xy_probe_rx_frames_ok",
    "xy_probe_rx_frames_bad",
    "xy_probe_rx_overflow",
)
PERIODS = (2, 5, 10, 20, 30, 50)
WINDOW_S = 1.5


def write_period(openocd: str, config: str, address: int, period: int, elf: str) -> None:
    script = f"init; halt; mww 0x{address:08x} {period}; resume; shutdown"
    subprocess.run([openocd, "-f", config, "-c", script], check=True,
                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)


def zero_counters(openocd: str, config: str, addresses: dict[str, int]) -> None:
    writes = " ".join(f"mww 0x{addresses[name]:08x} 0;" for name in COUNTERS)
    script = f"init; halt; {writes} resume; shutdown"
    subprocess.run([openocd, "-f", config, "-c", script], check=True,
                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)


def saturated_writer(port: str, stop: threading.Event) -> threading.Thread:
    """Write frames as fast as the port accepts them - no pacing at all."""

    def run() -> None:
        fd = os.open(port, os.O_WRONLY | os.O_NOCTTY)
        attrs = termios.tcgetattr(fd)
        attrs[0] = 0
        attrs[1] = 0
        attrs[2] = termios.CS8 | termios.CREAD | termios.CLOCAL
        attrs[3] = 0
        attrs[4] = attrs[5] = termios.B115200
        termios.tcsetattr(fd, termios.TCSANOW, attrs)
        index = 0
        try:
            while not stop.is_set():
                payload = b"".join(
                    b"IRQ" + (index + i).to_bytes(4, "little") for i in range(64)
                )
                os.write(fd, payload)
                index += 64
        finally:
            os.close(fd)

    thread = threading.Thread(target=run, daemon=True)
    thread.start()
    return thread


def verdict(results: list[tuple[int, dict[str, int]]]) -> tuple[int, list[str]]:
    """Turn the table into a pass or a fail.

    A sweep that never overflows has not found the threshold, and one where everything
    overflows says the harness is the bottleneck - neither is evidence of where the
    boundary is, so both fail rather than reporting a number.
    """
    clean = [period for period, words in results if words["xy_probe_rx_overflow"] == 0]
    dirty = [period for period, words in results if words["xy_probe_rx_overflow"]]
    if dirty and clean:
        return 0, [
            "",
            f"measured threshold: overflow at {min(dirty)} ms, clean at {max(clean)} ms",
            "model said: 256 bytes / 11.52 kB/s = 22.2 ms of line time",
        ]
    if not dirty:
        return 1, [
            "",
            "every period survived: the sweep never reached the threshold, so it proved "
            "nothing about where it is",
        ]
    return 1, ["", "nothing survived: the harness is the bottleneck, not the ring"]


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--openocd", required=True)
    parser.add_argument("--config", required=True)
    parser.add_argument("--nm", required=True)
    parser.add_argument("--elf", required=True)
    parser.add_argument("--uart", default="/dev/ttyACM0")
    parser.add_argument("--window", type=float, default=WINDOW_S)
    args = parser.parse_args()

    symbols = probe.symbol_addresses(args.nm, args.elf)
    wanted = {"xy_probe_rx_task_period_ms": symbols["xy_probe_rx_task_period_ms"]}
    wanted.update({name: symbols[name] for name in COUNTERS if name in symbols})
    missing = [name for name in ("xy_probe_rx_task_period_ms",) + COUNTERS
               if name not in wanted]
    if missing:
        print(f"sweep_rx_overflow failed: missing symbols {missing}")
        return 1

    print(f"{'period_ms':>9} {'needs_B':>8} {'bytes':>8} {'frames_ok':>9} {'frames_bad':>10} {'overflow':>8}")
    results = []
    for period in PERIODS:
        write_period(args.openocd, args.config, wanted["xy_probe_rx_task_period_ms"],
                     period, args.elf)
        zero_counters(args.openocd, args.config, wanted)
        stop = threading.Event()
        thread = saturated_writer(args.uart, stop)
        time.sleep(args.window)
        stop.set()
        thread.join(timeout=2)
        time.sleep(0.1)
        words = probe.read_words(args.openocd, args.config,
                                 {name: wanted[name] for name in COUNTERS}, resume=False)
        results.append((period, words))
        # The model: bytes that arrive while the task sleeps have to fit in the ring.
        needs = 11520 * period / 1000.0
        print(f"{period:>9} {needs:>8.0f} {words['xy_probe_rx_irq_bytes']:>8} "
              f"{words['xy_probe_rx_frames_ok']:>9} {words['xy_probe_rx_frames_bad']:>10} "
              f"{words['xy_probe_rx_overflow']:>8}")

    code, lines = verdict(results)
    for line in lines:
        print(line)
    return code


if __name__ == "__main__":
    raise SystemExit(main())
