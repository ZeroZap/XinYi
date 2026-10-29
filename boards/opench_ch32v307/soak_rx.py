#!/usr/bin/env python3
"""Soak the receive path: a long run with no debugger traffic in the middle.

Every judgement so far has been a two second window. That cannot see a slow leak, a
counter that wraps, or an overflow that only happens once the scheduler has been busy for
a while.

The protocol matters more than the duration: the counters are zeroed before the run and
read once at the end, because reading memory stops the core and the hardware loses
whatever arrives while it is stopped. A soak that is interrupted every few seconds would
be measuring the debugger.

    python3 boards/opench_ch32v307/soak_rx.py --openocd <ocd> --config <cfg> \
        --nm <nm> --elf <probe.elf> --seconds 300
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
    "xy_probe_rx_ore",
    "xy_probe_rx_last_index",
    "xy_probe_rx_expected",
)


def open_raw(port: str) -> int:
    fd = os.open(port, os.O_WRONLY | os.O_NOCTTY)
    attrs = termios.tcgetattr(fd)
    attrs[0] = 0
    attrs[1] = 0
    attrs[2] = termios.CS8 | termios.CREAD | termios.CLOCAL
    attrs[3] = 0
    attrs[4] = attrs[5] = termios.B115200
    termios.tcsetattr(fd, termios.TCSANOW, attrs)
    return fd


def verdict(sent_frames: int, words: dict[str, int]) -> list[str]:
    """Return what is wrong with a soak result; empty means every byte was accounted for.

    Broken out of main() so it can be tested. A judgement that only ever runs on hardware
    is a judgement nobody has ever seen fail, and "0 across the board" reads exactly the
    same whether the counters were checked or the check was silently skipped.
    """
    errors = []
    if words["xy_probe_rx_frames_bad"]:
        errors.append(f"{words['xy_probe_rx_frames_bad']} frames broke continuity")
    if words["xy_probe_rx_overflow"]:
        errors.append(f"the ring overflowed {words['xy_probe_rx_overflow']} times")
    if words["xy_probe_rx_ore"]:
        errors.append(f"the receiver overran {words['xy_probe_rx_ore']} times")
    expected_bytes = sent_frames * 8
    if words["xy_probe_rx_irq_bytes"] != expected_bytes:
        errors.append(
            f"{expected_bytes} bytes sent but {words['xy_probe_rx_irq_bytes']} arrived"
        )
    if words["xy_probe_rx_frames_ok"] != sent_frames:
        errors.append(
            f"{sent_frames} frames sent but {words['xy_probe_rx_frames_ok']} were intact"
        )
    return errors


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--openocd", required=True)
    parser.add_argument("--config", required=True)
    parser.add_argument("--nm", required=True)
    parser.add_argument("--elf", required=True)
    parser.add_argument("--uart", default="/dev/ttyACM0")
    parser.add_argument("--seconds", type=float, default=300.0)
    parser.add_argument("--frames-per-burst", type=int, default=8)
    parser.add_argument("--burst-ms", type=float, default=10.0)
    parser.add_argument("--settle", type=float, default=3.0,
                        help="seconds to let the ring drain before the final read")
    parser.add_argument("--saturated", action="store_true",
                        help="no pacing: write as fast as the port accepts, which is the "
                             "line rate the ring budget was computed against")
    args = parser.parse_args()

    symbols = probe.symbol_addresses(args.nm, args.elf)
    missing = [name for name in COUNTERS if name not in symbols]
    if missing:
        print(f"soak_rx failed: missing symbols {missing}")
        return 1
    addresses = {name: symbols[name] for name in COUNTERS}

    # Zero everything, including the index expectation, so the first frame of the soak is
    # the first frame the probe has ever seen.
    zero = []
    for name, address in addresses.items():
        value = "0xffffffff" if name == "xy_probe_rx_expected" else "0"
        zero += ["-c", f"mww 0x{address:08x} {value}"]
    reset = subprocess.run(
        [args.openocd, "-f", args.config, "-c", "adapter speed 1000", "-c", "init",
         "-c", "halt", *zero, "-c", "resume", "-c", "shutdown"],
        capture_output=True, text=True,
    )
    if reset.returncode != 0:
        print(f"soak_rx failed: could not zero the counters\n{reset.stdout}{reset.stderr}")
        return 1

    stop = threading.Event()
    sent = {"frames": 0}

    def run() -> None:
        fd = open_raw(args.uart)
        index = 0
        try:
            while not stop.is_set():
                payload = b"".join(
                    probe.encode_frame(b"I" + (index + i).to_bytes(4, "little"))
                    for i in range(args.frames_per_burst)
                )
                os.write(fd, payload)
                index += args.frames_per_burst
                sent["frames"] = index
                if not args.saturated:
                    stop.wait(args.burst_ms / 1000.0)
        finally:
            os.close(fd)

    thread = threading.Thread(target=run, daemon=True)
    started = time.time()
    thread.start()
    try:
        while time.time() - started < args.seconds:
            time.sleep(5.0)
            print(f"  {time.time() - started:6.0f}s sent={sent['frames']}", flush=True)
    finally:
        stop.set()
        thread.join(timeout=5)
    time.sleep(args.settle)

    words = probe.read_words(args.openocd, args.config, addresses, resume=False)
    mode = "saturated" if args.saturated else f"paced at {args.frames_per_burst} frames " \
        f"per {args.burst_ms:.0f} ms"
    rate = sent["frames"] * 8 / max(args.seconds, 1e-9)
    print(f"\nsoak: {args.seconds:.0f}s {mode}, sent {sent['frames']} frames "
          f"({sent['frames'] * 8} bytes, {rate / 1024.0:.1f} kB/s)")
    for name, value in words.items():
        print(f"  {name} = {value}")

    errors = verdict(sent["frames"], words)
    if errors:
        print("soak_rx failed:")
        for error in errors:
            print(f"- {error}")
        return 1
    print(f"soak_rx_ok {args.seconds:.0f}s {sent['frames']} frames byte-exact, no overflow, "
          f"no overruns")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
