#!/usr/bin/env python3
"""Fail-closed contract for the Pandora STM32L475VE smoke target."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
BOARD = ROOT / "boards" / "pandora_stm32l475"
RECORD = ROOT / "docs" / "validation" / "xinyi-pandora-stm32l475-board-smoke-record.md"


def require(path: Path, *needles: str) -> None:
    assert path.is_file(), f"missing {path.relative_to(ROOT)}"
    text = path.read_text(encoding="utf-8")
    for needle in needles:
        assert needle in text, f"{path.relative_to(ROOT)} missing {needle!r}"


def main() -> None:
    require(
        ROOT / "cmake" / "platform" / "STM32L4.cmake",
        "STM32L475xx",
        "STM32L4_BOARD",
    )
    require(
        BOARD / "CMakeLists.txt",
        "pandora_stm32l475_smoke",
        "startup_stm32l475xx.s",
        "STM32L475VETX_FLASH.ld",
        "-O binary",
    )
    require(
        BOARD / "capture_uart.py",
        "NO_DATA_TIMEOUT",
        "DEVICE_OPEN_FAILED",
        'default="/dev/ttyACM0"',
        "source_commit",
        "bytes_captured",
    )
    require(
        BOARD / "main.c",
        "GPIO_PIN_7",
        "GPIO_PIN_9",
        "GPIO_PIN_10",
        "GPIO_PIN_1",
        "GPIO_PIN_6",
        "soft_i2c_start",
        "soft_i2c_write_byte",
        "soft_i2c_read_byte",
        "aht10_init",
        "aht10_measure",
        "0xACU",
        "0x33U",
        "humidity_milli_percent",
        "temperature_milli_c",
        "0x38U << 1",
        "USART1",
        "AHT10 0x38 ACK",
        "KEY0",
        "PANDORA STM32L475VE",
    )
    require(
        BOARD / "STM32L475VETX_FLASH.ld",
        "ORIGIN = 0x08000000",
        "LENGTH = 512K",
        "ORIGIN = 0x20000000",
        "LENGTH = 96K",
    )
    require(ROOT / "CMakeLists.txt", "STM32L4_BOARD", "add_subdirectory(boards/pandora_stm32l475)")
    require(
        RECORD,
        "BLOCKED_VCP_UNSTABLE",
        "00cc9ee97256c5114c5e218717fc12253ad6e0f9",
        "0483:374b",
        "V2J24S11",
        "Flash written and verified",
        "CAPTURE_BYTES=0",
        "BOARD_RUNTIME_PENDING",
    )
    print("Pandora STM32L475VE board contract: PASS")


if __name__ == "__main__":
    main()
