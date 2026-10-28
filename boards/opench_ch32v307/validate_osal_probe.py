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

STAGE_DONE = 10
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


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--openocd", required=True)
    parser.add_argument("--config", required=True)
    parser.add_argument("--elf", required=True)
    parser.add_argument("--nm", required=True)
    parser.add_argument("--settle-ms", type=int, default=4000)
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

    first = read_words(args.openocd, args.config, addresses, resume=True)
    subprocess.run(["sleep", "2"])
    second = read_words(args.openocd, args.config, addresses, resume=False)

    errors = []
    if first.get("xy_probe_stage") != STAGE_DONE:
        errors.append(f"stage is {first.get('xy_probe_stage')}, expected {STAGE_DONE}")
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

    if errors:
        print("osal_stall_probe failed:")
        for error in errors:
            print(f"- {error}")
        return 1

    print(
        "osal_stall_probe_ok "
        f"stage={first['xy_probe_stage']} loops={first['xy_probe_loops']} "
        f"wait={first['xy_probe_wait_ret']:#x} get={first['xy_probe_get_ret']:#x} "
        f"uart={sends - fails}/{sends} i2c={ops - i2c_fails}/{ops} "
        f"ticks={ticks_first:#x}->{ticks_second:#x}"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
