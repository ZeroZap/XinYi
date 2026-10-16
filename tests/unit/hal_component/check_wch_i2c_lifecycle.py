#!/usr/bin/env python3
"""Guard WCH I2C lifecycle ownership before register transactions."""

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[3]
SOURCE = ROOT / "components/hal/wch/ch32x/src/xy_hal_i2c.c"


def body(source: str, start: str, end: str) -> str:
    return source.split(start, 1)[-1].split(end, 1)[0]


def main() -> int:
    source = SOURCE.read_text(encoding="utf-8")
    errors: list[str] = []

    if "static uint8_t i2c_initialized[2];" not in source:
        errors.append("I2C backend must track lifecycle state per supported instance")
    if "if (i2c_initialized[index] != 0U)" not in body(
        source, "xy_hal_i2c_init", "xy_hal_i2c_deinit"
    ):
        errors.append("duplicate I2C init must be rejected before clock/register side effects")
    if "i2c_initialized[index] = 1U;" not in body(
        source, "xy_hal_i2c_init", "xy_hal_i2c_deinit"
    ):
        errors.append("I2C init must publish ownership only after peripheral enable")
    deinit = body(source, "xy_hal_i2c_deinit", "xy_hal_i2c_master_transmit")
    if "if (i2c_initialized[index] == 0U)" not in deinit:
        errors.append("I2C deinit must reject an uninitialized instance")
    if "i2c_initialized[index] = 0U;" not in deinit:
        errors.append("I2C deinit must clear ownership after peripheral teardown")

    operations = [
        ("xy_hal_i2c_master_transmit", "xy_hal_i2c_master_receive"),
        ("xy_hal_i2c_master_receive", "xy_hal_i2c_mem_write"),
        ("xy_hal_i2c_mem_write", "xy_hal_i2c_mem_read"),
        ("xy_hal_i2c_mem_read", "xy_hal_i2c_master_transmit_dma"),
        ("xy_hal_i2c_master_transmit_dma", "xy_hal_i2c_master_receive_dma"),
        ("xy_hal_i2c_master_receive_dma", "xy_hal_i2c_register_callback"),
        ("xy_hal_i2c_register_callback", "xy_hal_i2c_is_device_ready"),
        ("xy_hal_i2c_is_device_ready", "xy_hal_i2c_error"),
        ("xy_hal_i2c_error", "xy_hal_i2c_set_error_cb"),
    ]
    for start, end in operations:
        operation = body(source, start, end)
        if "i2c_initialized[index] == 0U" not in operation:
            errors.append(f"{start} must reject an uninitialized I2C instance")

    for start, end in operations[4:6]:
        operation = body(source, start, end)
        if "!i2c_address_valid(address)" not in operation or "len == 0U" not in operation:
            errors.append(f"{start} must reject invalid transfer arguments before unsupported")

    if errors:
        print("wch_i2c_lifecycle failed:")
        for error in errors:
            print(f"- {error}")
        return 1

    print("wch_i2c_lifecycle_ok ownership=tracked duplicate-init=fail-closed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
