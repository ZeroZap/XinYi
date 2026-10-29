#!/usr/bin/env python3
"""Validate the openCH FreeRTOS probe image through the debugger, not the UART.

The WCH-Link VCP on this board drops and reorders bytes: even with a reader
attached before the reset, markers come back as "NCH_OSAL_PROBE_BOOT_..." with
chopped prefixes, so a marker gate cannot prove what the image did. The probe
image therefore publishes its progress in RAM, and this validator halts the
target and reads those words. Anything asserted here survives the transport.

Checks:
  * xy_probe_stage reached 8, the last step of the probe sequence;
  * all three xy_os_delay(100) calls returned (xy_probe_loops == 3) with XY_OS_OK;
  * xy_os_event_flags_wait timed out with the 0x80000000 sentinel;
  * xy_os_msgqueue_get timed out with XY_OS_ERROR_TIMEOUT;
  * xTickCount advances between two samples, so the tick really runs;
  * a real I2C transfer under the kernel read MPU6050 WHO_AM_I = 0x68 without a
    single failure - the peripheral that actually has to wait;
  * every xy_hal_uart_send reported XY_HAL_OK. That is the measurement of the
    Open Issue in xy_hal_tick_internal.h from the other side: HAL polling waits
    still succeed under FreeRTOS while TXE is immediately ready, so the broken
    timebase has no effect yet - the day a send has to wait, this counter catches
    it instead of the markers quietly disappearing.
"""

import argparse
import re
import subprocess
import sys

# The probe settles at stage 16 and stays up counting receive interrupts, so the two
# snapshots can be compared. Earlier stages: stage 10 runs the UART burst that measures the
# HAL millisecond helper against wall clock, stage 12 makes a receive give up on a
# silent line, stage 14 asks a bus for an address nobody answers on, and 15 means
# every measurement is published.
STAGE_DONE = 16
# 256 bytes at 115200 8N1 is 22.2 ms on the wire. The window is wide enough for
# buffering jitter but narrow enough that a saw-tooth timebase cannot land in it.
BURST_MS_MIN = 10
BURST_MS_MAX = 200
# A one-byte receive on a silent line must spend the timeout it was given and
# report XY_HAL_ERROR_TIMEOUT (-3, stored as uint32) rather than returning early,
# hanging, or claiming success.
RECV_MS_MIN = 5
RECV_MS_MAX = 200
RECV_TIMEOUT_CODE = 0xFFFFFFFD
# An address nobody answers on has to come back as "not found", and the transfer
# right after it has to succeed - a bus left with AF latched is the field failure.
I2C_NOT_FOUND_CODE = 0xFFFFFFFA
I2C_RECOVER_VALUE = 0x68
WHOAMI_EXPECTED = 0x68
LOOPS_EXPECTED = 3
EVENT_FLAGS_ERROR = 0x80000000
MSGQUEUE_TIMEOUT = 0xFFFFFFFE

SYMBOLS = (
    "xy_probe_stage",
    "xy_probe_loops",
    "xy_probe_delay_ret",
    "xy_probe_wait_ret",
    "xy_probe_get_ret",
    "xy_probe_uart_sends",
    "xy_probe_uart_fails",
    "xy_probe_i2c_ops",
    "xy_probe_i2c_fails",
    "xy_probe_whoami",
    "xy_probe_uart_burst_ms",
    "xy_probe_uart_recv_ms",
    "xy_probe_uart_recv_code",
    "xy_probe_i2c_nak_code",
    "xy_probe_i2c_recover_code",
    "xy_probe_i2c_recover_value",
    "xy_probe_rx_irq_bytes",
    "xTickCount",
)


def symbol_addresses(nm: str, elf: str) -> dict[str, int]:
    out = subprocess.run([nm, elf], capture_output=True, text=True, check=True).stdout
    wanted = {}
    for line in out.splitlines():
        parts = line.split()
        if len(parts) == 3 and parts[2] in SYMBOLS:
            wanted[parts[2]] = int(parts[0], 16)
    missing = [name for name in SYMBOLS if name not in wanted]
    if missing:
        raise SystemExit(f"symbols missing from {elf}: {missing}")
    return wanted


def read_words(openocd: str, config: str, addresses: dict[str, int], resume: bool) -> dict[str, int]:
    commands = []
    for name, address in addresses.items():
        commands += ["-c", f"mdw 0x{address:08x} 1"]
    if resume:
        commands += ["-c", "resume"]
    commands += ["-c", "shutdown"]
    completed = subprocess.run(
        [openocd, "-f", config, "-c", "adapter speed 1000", "-c", "init", "-c", "halt", *commands],
        capture_output=True,
        text=True,
    )
    out = completed.stdout + completed.stderr
    values = {}
    for name, address in addresses.items():
        match = re.search(rf"0x{address:08x}:\s*([0-9a-fA-F]{{8}})", out)
        if match:
            values[name] = int(match.group(1), 16)
    if len(values) != len(addresses):
        raise SystemExit(f"could not read every probe word; openocd said:\n{out}")
    return values


def _start_writer(port: str):
    """Keep feeding bytes to the board so the receive interrupt has something to do."""
    import os
    import threading

    stop = threading.Event()

    def run() -> None:
        fd = os.open(port, os.O_WRONLY | os.O_NOCTTY)
        try:
            while not stop.is_set():
                os.write(fd, b"IRQTEST-0123456789ABCDEF")
                stop.wait(0.15)
        finally:
            os.close(fd)

    thread = threading.Thread(target=run, daemon=True)
    thread.start()
    return stop, thread


def _stop_writer(writer) -> None:
    stop, thread = writer
    stop.set()
    thread.join(timeout=2)


def judge(first: dict[str, int], second: dict[str, int], uart: str | None = "") -> list[str]:
    """Judge two RAM snapshots of one probe run; empty list means the run is good."""
    errors = []
    if first.get("xy_probe_stage") != STAGE_DONE:
        errors.append(f"stage is {first.get('xy_probe_stage')}, expected {STAGE_DONE}")
    burst_ms = first.get("xy_probe_uart_burst_ms")
    if burst_ms is None or not BURST_MS_MIN <= burst_ms <= BURST_MS_MAX:
        errors.append(
            f"xy_probe_uart_burst_ms is {burst_ms}, expected {BURST_MS_MIN}..{BURST_MS_MAX} "
            "(256 bytes at 115200 take about 22 ms)"
        )
    recv_ms = first.get("xy_probe_uart_recv_ms")
    if recv_ms is None or not RECV_MS_MIN <= recv_ms <= RECV_MS_MAX:
        errors.append(
            f"xy_probe_uart_recv_ms is {recv_ms}, expected {RECV_MS_MIN}..{RECV_MS_MAX} "
            "(a 10 ms receive on a silent line has to spend its timeout)"
        )
    if first.get("xy_probe_uart_recv_code") != RECV_TIMEOUT_CODE:
        errors.append(
            f"xy_probe_uart_recv_code is {first.get('xy_probe_uart_recv_code')}, "
            f"expected {RECV_TIMEOUT_CODE:#x} (XY_HAL_ERROR_TIMEOUT)"
        )
    rx_first, rx_second = first.get("xy_probe_rx_irq_bytes"), second.get("xy_probe_rx_irq_bytes")
    if not uart:
        pass  # nothing was sent, so there is nothing to judge
    elif not rx_first:
        errors.append(
            "no byte arrived through the UART receive interrupt: "
            f"xy_probe_rx_irq_bytes is {rx_first}"
        )
    elif rx_second is None or rx_second <= rx_first:
        errors.append(
            f"the receive interrupt stopped: xy_probe_rx_irq_bytes {rx_first} -> {rx_second} "
            "(a stale value cannot pass for a live interrupt)"
        )
    if first.get("xy_probe_i2c_nak_code") != I2C_NOT_FOUND_CODE:
        errors.append(
            f"an unanswered I2C address returned {first.get('xy_probe_i2c_nak_code')}, "
            f"expected {I2C_NOT_FOUND_CODE:#x} (XY_HAL_ERROR_NOT_FOUND)"
        )
    if first.get("xy_probe_i2c_recover_code") != 0:
        errors.append(
            "the transfer after an unanswered address failed with "
            f"{first.get('xy_probe_i2c_recover_code')}: the bus stayed stuck"
        )
    if first.get("xy_probe_i2c_recover_value") != I2C_RECOVER_VALUE:
        errors.append(
            f"the recovering read saw {first.get('xy_probe_i2c_recover_value')}, "
            f"expected {I2C_RECOVER_VALUE:#x}"
        )
    if first.get("xy_probe_loops") != LOOPS_EXPECTED:
        errors.append(f"loops is {first.get('xy_probe_loops')}, expected {LOOPS_EXPECTED}")
    if first.get("xy_probe_delay_ret") != 0:
        errors.append(f"delay returned {first.get('xy_probe_delay_ret')}, expected 0")
    if first.get("xy_probe_wait_ret") != EVENT_FLAGS_ERROR:
        errors.append(
            f"event-flag wait returned {first.get('xy_probe_wait_ret')}, "
            f"expected {EVENT_FLAGS_ERROR:#x}"
        )
    if first.get("xy_probe_get_ret") != MSGQUEUE_TIMEOUT:
        errors.append(
            f"msgqueue get returned {first.get('xy_probe_get_ret')}, "
            f"expected {MSGQUEUE_TIMEOUT:#x}"
        )
    ops, i2c_fails = first.get("xy_probe_i2c_ops"), first.get("xy_probe_i2c_fails")
    if ops is None or ops < 4:
        errors.append(f"only {ops} I2C transfers recorded, expected at least 4")
    if i2c_fails:
        errors.append(
            f"{i2c_fails} of {ops} I2C transfers failed under the kernel: the HAL polling "
            "timebase is invalid under FreeRTOS (see xy_hal_tick_internal.h)"
        )
    if first.get("xy_probe_whoami") != WHOAMI_EXPECTED:
        errors.append(
            f"MPU6050 WHO_AM_I read {first.get('xy_probe_whoami')}, expected {WHOAMI_EXPECTED:#x}"
        )

    sends, fails = first.get("xy_probe_uart_sends"), first.get("xy_probe_uart_fails")
    if sends is None or sends < 4:
        errors.append(f"only {sends} xy_hal_uart_send calls recorded, expected at least 4")
    if fails:
        errors.append(
            f"{fails} of {sends} xy_hal_uart_send calls failed: the HAL polling timebase is "
            "invalid under FreeRTOS (see the Open Issue in xy_hal_tick_internal.h)"
        )
    ticks_first, ticks_second = first.get("xTickCount"), second.get("xTickCount")
    if ticks_first is None or ticks_second is None or ticks_second <= ticks_first:
        errors.append(f"xTickCount did not advance: {ticks_first} -> {ticks_second}")
    return errors


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--openocd", required=True)
    parser.add_argument("--config", required=True)
    parser.add_argument("--elf", required=True)
    parser.add_argument("--nm", required=True)
    parser.add_argument("--settle-ms", type=int, default=4000)
    parser.add_argument(
        "--uart",
        help="serial port to write test bytes to; without it the receive-interrupt "
        "judgement cannot be made and is skipped",
    )
    args = parser.parse_args()

    if args.settle_ms < 0:
        raise SystemExit("settle-ms must be non-negative")

    addresses = symbol_addresses(args.nm, args.elf)

    subprocess.run(
        [
            args.openocd, "-f", args.config, "-c", "adapter speed 1000",
            "-c", "init", "-c", "reset halt", "-c", "resume", "-c", "shutdown",
        ],
        capture_output=True,
        text=True,
    )
    subprocess.run(["sleep", str(args.settle_ms / 1000.0)])

    writer = None
    if args.uart:
        writer = _start_writer(args.uart)
    first = read_words(args.openocd, args.config, addresses, resume=True)
    subprocess.run(["sleep", "2"])
    second = read_words(args.openocd, args.config, addresses, resume=False)
    if writer is not None:
        _stop_writer(writer)

    errors = judge(first, second, args.uart)
    if errors:
        print("osal_stall_probe failed:")
        for error in errors:
            print(f"- {error}")
        return 1

    # Read back from the snapshot for the summary: judge() owns the checks, so the
    # locals it builds do not exist out here.
    sends = first.get("xy_probe_uart_sends", 0)
    fails = first.get("xy_probe_uart_fails", 0)
    ops = first.get("xy_probe_i2c_ops", 0)
    i2c_fails = first.get("xy_probe_i2c_fails", 0)
    print(
        "osal_stall_probe_ok "
        f"stage={first['xy_probe_stage']} loops={first['xy_probe_loops']} "
        f"wait={first['xy_probe_wait_ret']:#x} get={first['xy_probe_get_ret']:#x} "
        f"uart={sends - fails}/{sends} i2c={ops - i2c_fails}/{ops} "
        f"nak={first.get('xy_probe_i2c_nak_code', 0):#x} "
        f"recover={first.get('xy_probe_i2c_recover_value', 0):#x} "
        f"recv_ms={first.get('xy_probe_uart_recv_ms', 0)} "
        f"ticks={first.get('xTickCount', 0):#x}->{second.get('xTickCount', 0):#x}"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
