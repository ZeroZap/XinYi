# openCH CH32V307 peripheral HAL: eleven smoke images close the board loop

Board: openCH CH32V307, WCH-Link CH549F. Every image was built from the tracked `MCU/wch` SDK,
programmed through the WCH OpenOCD fork (`program <elf> 0x00000000 verify reset`, adapter speed
1000 — all reported `Programming Finished` / `Verified OK` / `Resetting Target`), and its UART
markers captured on the CH549F virtual COM port (`/dev/ttyACM0`, by-id
`usb-wch.cn_WCH-Link_BC5C0A1DC679-if01`) with the reader opened before reset and the port drained
first. Sources: `boards/opench_ch32v307/<name>_main.c`.

## Results

| image | observed marker | verdict |
|---|---|---|
| `tim2_counter` | `OPENCH_TIM2_COUNTER_RUNNING_OK` (repeated) | `B1_REVIEW_CANDIDATE` |
| `dma1_systick8` | `…_BOOT_20261014_A` → `OPENCH_DMA1_SYSTICK8_MEMCPY_128X_OK_A` | `B1_REVIEW_CANDIDATE` |
| `rtc_lsi` | `…_UART_READY` → `…_LSI_READY` → `OPENCH_RTC_BACKUP_OK` → `OPENCH_RTC_LSI_COUNTER_BACKUP_OK` | `B1_REVIEW_CANDIDATE` |
| `system_systick_source` | `OPENCH_SYS_SYSTICK_HCLK8_TIME_OK_A` | `B1_REVIEW_CANDIDATE` |
| `adc1_pa0` | `OPENCH_ADC1_CH0_PA0_READ_OK` (repeated) | `B1_REVIEW_CANDIDATE` |
| `dac1_8bit_alignment` | `OPENCH_DAC1_8BIT_RIGHT_DAC_ADC_32X_OK_A` + `OPENCH_DAC1_8BIT_LEFT_REJECTED_OK` | `B1_REVIEW_CANDIDATE` |
| `can1_loopback` | `…_GPIO_READY` → `…_PERIPHERAL_READY` → `…_FILTER_READY` → `…_STARTED` → `OPENCH_CAN1_PB8_PB9_LOOPBACK_OK` | `B1_REVIEW_CANDIDATE` |
| `pwm_tim3_pa6` | `OPENCH_TIM3_CH1_PA6_PWM_OK` (repeated) | `B1_REVIEW_CANDIDATE` |
| `i2s2_tx` | `…_BOOT_ID` → `…_INIT_OK` → `…_CALLBACK_OK` → `…_START_OK` → `…_READY` → `OPENCH_I2S2_TX_OK_A` | `B1_REVIEW_CANDIDATE` |
| `uart67_dma` | `OPENCH_UART6_DMA2_CH6_TX_OK` + `OPENCH_UART7_DMA2_CH8_TX_OK` (repeated) | `B1_REVIEW_CANDIDATE` |
| `iwdg` | `OPENCH_IWDG_FEED_OK` (repeated) | `B1_REVIEW_CANDIDATE` |

## What each image proves

- **tim2_counter** — a general timer configured through `xy_hal_timer_*` actually advances: the
  counter read before and after a bounded delay differs, so the peripheral is clocked and counting.
- **dma1_systick8** — 128 DMA1 memory-to-memory copies through `xy_hal_dma_*` each compare equal
  after `poll_complete` returns and the transfer counter reaches 0.
- **rtc_lsi** — RTC on LSI initializes, a backup register write/read round-trips `0x51A7C0DE`,
  and the timestamp advances across a delay.
- **system_systick_source** — the SysTick time source reports a sane wall-clock for an HCLK/8
  measurement window (`…_HCLK8_TIME_OK_A`).
- **adc1_pa0** — ADC1 channel 0 on PA0 samples through `xy_hal_adc_*` and returns a reading.
- **dac1_8bit_alignment** — DAC1 8-bit right-aligned values round-trip through the onboard
  DAC→ADC path 32×, and 8-bit left alignment is rejected rather than silently coerced.
- **can1_loopback** — CAN1 on PB8/PB9 passes init, filter setup and an internal loopback
  send/receive through `xy_hal_can_*`.
- **pwm_tim3_pa6** — TIM3 CH1 on PA6 is configured and started through `xy_hal_pwm_*`.
- **i2s2_tx** — I2S2 initializes, arms its callback, starts and reports TX through `xy_hal_i2s_*`.
- **uart67_dma** — UART6 and UART7 each complete a DMA-backed TX through `xy_hal_uart_*` + `xy_hal_dma_*`.
- **iwdg** — the independent watchdog is fed through `xy_hal_iwdg_*` without a reset.

## Boundary

These are controller/register-level runtime proofs — each peripheral is configured through the HAL,
started, and observed to reach its own success marker on the real board. They do **not** prove:

- ADC accuracy/linearity against a characterized source, or DAC analog output level (DAC is only
  proven back through the on-chip ADC, not an external instrument);
- PWM frequency/duty electrical correctness (needs a scope/logic analyzer);
- CAN physical-bus arbitration/recovery (loopback only, no transceiver);
- I2S audio quality, or any peer on the other side of UART6/UART7 (DMA TX completion is not peer
  reception);
- watchdog reset-cause behavior under a deliberately un-fed window (the image feeds it, so no reset
  is observed by design).

Evidence is machine-captured UART marker sequences; each image's raw capture is in `uart-<name>.txt`.
