#!/usr/bin/env python3
"""Cross the frame counter's wrap boundary and check that continuity is still detected.

The probe tracks the next expected index in a signed 32 bit counter and uses a negative
value to mean "nothing expected yet". Those two conventions meet at the wrap boundary,
where a legitimate index *is* negative. This script drives the real boundary rather than
reasoning about it: set the expectation just below the wrap, send frames across it, then
skip some frames and see whether the probe still notices.

    python3 boards/opench_ch32v307/wrap_rx.py --openocd <ocd> --config <cfg> \
        --nm <nm> --elf <probe.elf>
"""

from __future__ import annotations

import argparse
import os
import subprocess
import sys
import termios
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import validate_osal_probe as probe  # noqa: E402

# Start three frames below the wrap so the sequence crosses it and keeps going.
WRAP_START = 0xFFFFFFFF - 2


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


def write_frames(port: str, indices: list[int]) -> None:
    fd = open_raw(port)
    try:
        for index in indices:
            os.write(fd, probe.encode_frame(b"I" + (index & 0xFFFFFFFF).to_bytes(4, "little")))
            time.sleep(0.002)
    finally:
        os.close(fd)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--openocd", required=True)
    parser.add_argument("--config", required=True)
    parser.add_argument("--nm", required=True)
    parser.add_argument("--elf", required=True)
    parser.add_argument("--uart", default="/dev/ttyACM0")
    args = parser.parse_args()

    symbols = probe.symbol_addresses(args.nm, args.elf)
    names = (
        "xy_probe_rx_irq_bytes",
        "xy_probe_rx_frames_ok",
        "xy_probe_rx_frames_bad",
        "xy_probe_rx_last_index",
        "xy_probe_rx_last_bad_index",
        "xy_probe_rx_expected",
        "xy_probe_rx_have_expected",
    )
    present = [name for name in names if name in symbols]

    def zero() -> None:
        commands = []
        for name in ("xy_probe_rx_irq_bytes", "xy_probe_rx_frames_ok", "xy_probe_rx_frames_bad"):
            commands += ["-c", f"mww 0x{symbols[name]:08x} 0"]
        if "xy_probe_rx_have_expected" in symbols:
            # Newer firmware: the expectation is a counter plus a validity flag.
            commands += ["-c", f"mww 0x{symbols['xy_probe_rx_have_expected']:08x} 0"]
        else:
            commands += ["-c", f"mww 0x{symbols['xy_probe_rx_expected']:08x} 0xffffffff"]
        subprocess.run(
            [args.openocd, "-f", args.config, "-c", "adapter speed 1000", "-c", "init",
             "-c", "halt", *commands, "-c", "resume", "-c", "shutdown"],
            capture_output=True, text=True, check=True,
        )

    def arm(expectation: int) -> None:
        """Tell the probe which index comes next, without sending that frame."""
        commands = ["-c", f"mww 0x{symbols['xy_probe_rx_expected']:08x} 0x{expectation & 0xFFFFFFFF:08x}"]
        if "xy_probe_rx_have_expected" in symbols:
            commands += ["-c", f"mww 0x{symbols['xy_probe_rx_have_expected']:08x} 1"]
        subprocess.run(
            [args.openocd, "-f", args.config, "-c", "adapter speed 1000", "-c", "init",
             "-c", "halt", *commands, "-c", "resume", "-c", "shutdown"],
            capture_output=True, text=True, check=True,
        )

    addresses = {name: symbols[name] for name in present if name != "xy_probe_rx_have_expected"}
    zero()

    # Phase 1: a continuous run straight across the wrap. Every frame must be accepted.
    across = [WRAP_START + offset for offset in range(8)]
    write_frames(args.uart, across)
    time.sleep(0.5)
    before = probe.read_words(args.openocd, args.config, addresses, resume=False)
    print("across the wrap:")
    for name, value in before.items():
        print(f"  {name} = {value}")

    if before["xy_probe_rx_frames_ok"] != len(across) or before["xy_probe_rx_frames_bad"] != 0:
        print(f"wrap_rx failed: crossing the boundary broke continuity")
        return 1

    # Phase 2: skip three frames after the wrap. The probe must notice.
    zero()
    arm(WRAP_START)
    write_frames(args.uart, [WRAP_START, WRAP_START + 4])
    time.sleep(0.5)
    after = probe.read_words(args.openocd, args.config, addresses, resume=False)
    print("\nwith three frames skipped after the boundary:")
    for name, value in after.items():
        print(f"  {name} = {value}")

    if after["xy_probe_rx_frames_bad"] != 1:
        print("wrap_rx failed: a skipped frame after the wrap was not detected — "
              "continuity checking is dead past the boundary")
        return 1

    print("\nwrap_rx_ok continuity survives the counter wrapping")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
