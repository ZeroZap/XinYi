#!/usr/bin/env python3
"""Guard WCH SPI lifecycle ownership before register transactions."""

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[3]
SOURCE = ROOT / "components/hal/wch/ch32x/src/xy_hal_spi.c"


def body(source: str, start: str, end: str) -> str:
    return source.split(start, 1)[-1].split(end, 1)[0]


def main() -> int:
    source = SOURCE.read_text(encoding="utf-8")
    errors: list[str] = []

    if "static uint8_t spi_initialized[3];" not in source:
        errors.append("SPI backend must track lifecycle state per supported instance")
    init = body(source, "xy_hal_spi_init", "xy_hal_spi_deinit")
    if "spi_initialized[index] != 0U" not in init:
        errors.append("duplicate SPI init must be rejected before clock/register side effects")
    if "spi_initialized[index] = 1U;" not in init:
        errors.append("SPI init must publish ownership only after peripheral enable")

    deinit = body(source, "xy_hal_spi_deinit", "xy_hal_spi_transmit_receive")
    if "spi_initialized[index] == 0U" not in deinit:
        errors.append("SPI deinit must reject an uninitialized instance")
    if "spi_initialized[index] = 0U;" not in deinit:
        errors.append("SPI deinit must clear ownership after peripheral teardown")

    operations = [
        ("xy_hal_spi_transmit_receive", "xy_hal_spi_transmit"),
        ("xy_hal_spi_transmit_dma", "xy_hal_spi_receive_dma"),
        ("xy_hal_spi_receive_dma", "xy_hal_spi_transmit_receive_dma"),
        ("xy_hal_spi_transmit_receive_dma", "xy_hal_spi_stop"),
        ("xy_hal_spi_stop", "xy_hal_spi_register_callback"),
        ("xy_hal_spi_register_callback", "xy_hal_spi_set_cs"),
        ("xy_hal_spi_set_cs", "xy_hal_spi_error"),
        ("xy_hal_spi_error", "xy_hal_spi_set_error_cb"),
    ]
    for start, end in operations:
        operation = body(source, start, end)
        if "spi_initialized[index] == 0U" not in operation:
            errors.append(f"{start} must reject an uninitialized SPI instance")

    dma_operations = operations[1:4]
    for start, end in dma_operations:
        operation = body(source, start, end)
        if "len == 0U" not in operation:
            errors.append(f"{start} must reject zero-length transfer before unsupported")

    callback = body(source, "xy_hal_spi_register_callback", "xy_hal_spi_set_cs")
    if "callback == NULL" not in callback:
        errors.append("SPI callback registration must reject a NULL callback")

    if errors:
        print("wch_spi_lifecycle failed:")
        for error in errors:
            print(f"- {error}")
        return 1

    print("wch_spi_lifecycle_ok ownership=tracked duplicate-init=fail-closed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
