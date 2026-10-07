# openCH CH32V307 HAL runtime validation — 2026-10-07

- Board: openCH CH32V307VCT6 with onboard CH549 WCH-Link/UART.
- Source commit before evidence commit: `777dac93b2a8ea074de51716763fee0f106d0df0`.
- SDK gitlink: `08a0ecb80d11429764d0b050c5de9930beb701f0`.
- Programmer: MRS OpenOCD, WCH-Link RV 2.12, target `0x263e8dea`, 288 KiB Flash.
- UART: `/dev/serial/by-id/usb-wch.cn_WCH-Link_BC5C0A1DC679-if01`, 115200 8-N-1.
- Every image was independently programmed, verified, reset, and followed by a five-second UART capture.
- Machine-readable hashes and marker counts are in `summary.json`; raw UART and OpenOCD logs are retained per target.

## Results

| HAL slice | Runtime result | Evidence boundary |
|---|---|---|
| SPI3/W25Q128 | PASS: JEDEC `EF4018` repeated 14 times | Proves onboard SPI3 NOR identity read |
| I2C2 | PASS: peripheral ready and scan completed | Scan reported at least one ACK; does not identify a specific external device |
| UART6/UART7 | PASS: both peripheral-ready markers repeated | Proves initialization/runtime path, not ESP/BLE peer response |
| UART6/UART7 DMA TX | PASS: both DMA completion markers repeated 14 times | Proves DMA request/completion path, not peer reception |
| ADC1/PA0 | PASS: bounded 12-bit read marker repeated 13 times | Proves conversion path; PA0 has no characterized onboard analog source |
| TIM3 CH1/PA6 PWM | PASS: configuration/runtime marker repeated 13 times | Does not prove physical frequency/duty without an instrument |
| IWDG | PASS: feed marker repeated 12 times | Proves configured watchdog remains alive while fed; reset behavior not tested |
| CAN1 internal loopback | PASS: started marker and loopback marker repeated 14 times | Proves controller internal loopback; no external transceiver/bus evidence |
| TIM2 | PASS: changing free-running counter marker repeated 13 times | Proves counter runtime path |

During runtime closure, two defects were exposed and fixed before the final pass:

1. I2C2 scan originally emitted no UART data; staged markers localized the path and the rebuilt image completed the scan.
2. CAN1 initially failed initialization because the smoke timing tuple was invalid for the live PCLK1. The HAL now derives PCLK1 through `RCC_GetClocksFreq()`, and the smoke uses an exactly divisible 125 kbit/s timing tuple (`BS1=5`, `BS2=2`).

These are bounded B1 runtime results. They do not replace electrical measurements, peer/module protocol validation, negative recovery tests, or endurance evidence.
