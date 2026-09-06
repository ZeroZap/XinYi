#!/usr/bin/env python3
"""Fail-closed source contract for the Pandora onboard ST7789 path."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
BOARD = ROOT / "boards" / "pandora_stm32l475"


def require(path: Path, *needles: str) -> None:
    text = path.read_text(encoding="utf-8")
    for needle in needles:
        assert needle in text, f"{path.relative_to(ROOT)} missing {needle!r}"


def main() -> None:
    require(
        BOARD / "CMakeLists.txt",
        "pandora_stm32l475_st7789",
        "st7789_main.c",
        "xy_lcd_st7789.c",
        "stm32l4xx_hal_spi.c",
    )
    require(
        BOARD / "st7789_main.c",
        "SPI3",
        "xy_hal_gpio_init(GPIOB, 3U",
        "xy_hal_gpio_init(GPIOB, 5U",
        "GPIO_AF6_SPI3",
        "GPIOB, 4U",
        "GPIOD, 7U",
        "GPIOB, 6U",
        "GPIOB, 7U",
        "PANDORA_ST7789_COLOR RED",
        "PANDORA_ST7789_COLOR GREEN",
        "PANDORA_ST7789_COLOR BLUE",
        "PANDORA_ST7789_COLOR WHITE",
        "PANDORA_ST7789_COLOR BLACK",
        "PANDORA_ST7789_PATTERN_DONE",
        "PANDORA_ST7789_FINAL_SAFE",
        "xy_lcd_st7789_fill_checked",
        "config.rgb_order = true;",
    )
    print("pandora_st7789_board_contract=passed")


if __name__ == "__main__":
    main()