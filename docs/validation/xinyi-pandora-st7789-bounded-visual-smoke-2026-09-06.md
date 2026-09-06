# Pandora STM32L475VE onboard ST7789 validation

Date: 2026-09-06

## Hardware binding

Sources inspected:

- Tracked Pandora V2.4 schematic: `docs/hardware/pandora_stm32l475/Pandora_STM32L4_Board_V2.4_SCH.pdf`.
- RT-Thread upstream Pandora BSP at commit `54e5164064dd7bf2e3f008109d173c36e45f8f69`:
  `bsp/stm32/stm32l475-atk-pandora/board/ports/lcd/drv_lcd.c` and
  `applications/arduino_pinout/pins_arduino.h`.

Confirmed 4-wire, write-only SPI binding:

| Signal | MCU | Behavior |
|---|---|---|
| SCK | PB3 / SPI3 AF6 | SPI mode 0 clock |
| MOSI/SDA | PB5 / SPI3 AF6 | MSB-first display data |
| CS | PD7 | active low |
| D/C (`LCD_WR`) | PB4 | low command, high data |
| RESET | PB6 | active low |
| backlight/power | PB7 / TIM4_CH2 | active high; BSP uses PWM for brightness |

The board image initially uses 10 MHz SPI3, drives PB7 digitally on/off, and uses the reusable
`xy_lcd` → `xy_lcd_spi` → `xy_lcd_st7789` path. No application-level ST7789 register writes bypass
the component.

## Bounded smoke

The dedicated `pandora_stm32l475_st7789` image emits an exact source commit and markers while it:

1. initializes the panel and enables the backlight;
2. holds red, green, blue, white, and black for 500 ms each;
3. renders four colored quadrants with a centered black square;
4. leaves that pattern held with the backlight on.

Transport failures turn the backlight off and stop execution. Full-screen and rectangle fill use a
256-byte bounded stack buffer rather than a 115,200-byte allocation, and checked APIs propagate SPI
failures. The ST7789 inversion constants are corrected to DCS `0x21` ON / `0x20` OFF.

## Evidence boundary

Status: `COLOR_ORDER_CORRECTION_VISUAL_PENDING`.

Clean committed image `e125038eba6723ccf654d610573b55e96bc74c4f` was built with Arm GNU
15.2.1. ELF/BIN sizes are 35,556/12,820 bytes; BIN SHA-256 is
`e41a0b0db4dc9118b95443522728af61ed6abfcfed8b5b48bfda34b671ec2fdf`. ST-Link
V2J24S11 reported `Flash written and verified`; a 12,820-byte read-back was byte-identical.
The independent WCH-Link UART capture is 304 bytes with SHA-256
`fc5ceb584514bd2c3abb476acfe2fa509fd66f53eb54816246eeb714b33d2071`; machine parsing
confirmed the exact firmware identity and all nine ordered startup/color/pattern/final-state markers,
with zero error markers.

Evidence:

- `evidence/pandora-stm32l475/2026-09-06/pandora-st7789-e125038e-readback.bin`
- `evidence/pandora-stm32l475/2026-09-06/pandora-st7789-e125038e-uart.txt`

The initial visual report recorded in commit `653f1905` was incorrect. Eugene's corrected observation
for image `e125038e` is blue → green → red → white → black, followed by four quadrants with a central
black square. The firmware sent RGB565 red → green → blue while configuring MADCTL BGR, so this is
direct evidence of a red/blue channel swap. The premature visual B1 claim is retracted; the prior
flash/read-back/UART evidence remains valid only for control-path execution.

The correction configures this Pandora panel for RGB order (MADCTL BGR clear) and adds a Host
transaction contract that distinguishes RGB from BGR configuration. A newly built and flashed image
must be visually confirmed as red → green → blue → white → black, with correct quadrant colors,
before visual B1 can be restored.

This observation does not establish colorimetry, pixel-by-pixel integrity, tearing behavior, refresh
throughput, DMA operation, touch input, brightness, orientation qualification, performance, or
endurance. Those require separate fixtures and records.
