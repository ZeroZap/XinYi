#!/usr/bin/env python3
"""Ask the board for an I2C register over the serial line, and read the answer from RAM.

This is the chain end to end: a byte arrives through the UART receive interrupt, the
probe reassembles it into a command frame, calls xy_hal_i2c_mem_read() with the address
and register the frame carries, and publishes the result. Nothing in the middle is a
vendor call and nothing is done by hand.

    python3 boards/opench_ch32v307/i2c_over_uart.py --uart /dev/ttyACM0 --addr 0x68 --reg 0x75

The answer is read with the debugger rather than off the wire so this checks the device
path and the transmit path separately - the transmit path already has its own evidence.
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

RESULT_SYMBOLS = (
    "xy_probe_cmd_frames",
    "xy_probe_cmd_status",
    "xy_probe_cmd_addr",
    "xy_probe_cmd_reg",
    "xy_probe_cmd_value",
)

# Well-known answers on this board, so a run can be judged rather than just observed.
KNOWN = {
    (0x68, 0x75): 0x68,  # MPU6050 WHO_AM_I
    (0x38, 0x71): 0x18,  # AHT10 status: calibrated
}


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


def command_frame(addr: int, register: int, count: int) -> bytes:
    payload = bytes((ord("C"), addr, register, count))
    checksum = 0x5A
    for byte in payload:
        checksum ^= byte
    return bytes((0x02, len(payload))) + payload + bytes((checksum,))


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--openocd", required=True)
    parser.add_argument("--config", required=True)
    parser.add_argument("--nm", required=True)
    parser.add_argument("--elf", required=True)
    parser.add_argument("--uart", default="/dev/ttyACM0")
    parser.add_argument("--addr", type=lambda v: int(v, 0), required=True)
    parser.add_argument("--reg", type=lambda v: int(v, 0), required=True)
    parser.add_argument("--count", type=int, default=1)
    parser.add_argument("--expect", type=lambda v: int(v, 0),
                        help="expected register value; defaults to the known one")
    args = parser.parse_args()

    symbols = probe.symbol_addresses(args.nm, args.elf)
    missing = [name for name in RESULT_SYMBOLS if name not in symbols]
    if missing:
        print(f"i2c_over_uart failed: missing symbols {missing}")
        return 1
    addresses = {name: symbols[name] for name in RESULT_SYMBOLS}

    # The receive interrupt is only on once the probe reaches its final stage; a byte
    # written before that has nowhere to go, and "no frame arrived" would look like a
    # broken command path when it is really a race with boot.
    stage_address = symbols.get("xy_probe_stage")
    if stage_address is not None:
        deadline = time.time() + 10.0
        while time.time() < deadline:
            words = probe.read_words(args.openocd, args.config,
                                     {"xy_probe_stage": stage_address}, resume=True)
            if words["xy_probe_stage"] >= probe.STAGE_DONE:
                break
            time.sleep(0.2)

    fd = open_raw(args.uart)
    try:
        os.write(fd, command_frame(args.addr, args.reg, args.count))
    finally:
        os.close(fd)
    time.sleep(0.6)

    words = probe.read_words(args.openocd, args.config, addresses, resume=False)
    status = words["xy_probe_cmd_status"]
    value = words["xy_probe_cmd_value"]
    print(f"frames={words['xy_probe_cmd_frames']} addr=0x{words['xy_probe_cmd_addr']:02x} "
          f"reg=0x{words['xy_probe_cmd_reg']:02x} status={status} (0x{status:08x}) "
          f"value=0x{value:02x}")

    expected = args.expect
    if expected is None:
        expected = KNOWN.get((args.addr, args.reg))

    errors = []
    if words["xy_probe_cmd_frames"] == 0:
        errors.append("no command frame reached the probe")
    if status != 0:
        errors.append(f"the transfer failed with status {status} (0x{status:08x})")
    if expected is not None and value != expected:
        errors.append(f"register read back 0x{value:02x}, expected 0x{expected:02x}")

    if errors:
        print("i2c_over_uart failed:")
        for error in errors:
            print(f"- {error}")
        return 1
    print(f"i2c_over_uart_ok addr=0x{args.addr:02x} reg=0x{args.reg:02x} value=0x{value:02x}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
