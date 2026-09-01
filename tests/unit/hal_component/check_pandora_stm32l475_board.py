#!/usr/bin/env python3
"""Fail-closed contract for the Pandora STM32L475VE smoke target."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
BOARD = ROOT / "boards" / "pandora_stm32l475"


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
        BOARD / "main.c",
        "GPIO_PIN_7",
        "GPIO_PIN_9",
        "GPIO_PIN_10",
        "USART1",
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
    print("Pandora STM32L475VE board contract: PASS")


if __name__ == "__main__":
    main()
