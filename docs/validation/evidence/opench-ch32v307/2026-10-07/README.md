# openCH CH32V307 UART1 hardware validation — 2026-10-07

- Board: openCH CH32V307, target ID `0x263e8dea`, 288 KiB Flash.
- Probe: onboard WCH-Link/CH549, RV mode 2.12, USB `1a86:8010`.
- Source commit: `6fec165745f025077f9704f0add7a9925b82c052`.
- SDK gitlink: `08a0ecb80d11429764d0b050c5de9930beb701f0`.
- Toolchain: MRS RISC-V Embedded GCC15 / GNU 15.2.0.
- Image: `opench_ch32v307_uart_smoke.elf`, 4108 Flash bytes; BIN SHA-256 `ff6d87d510cb0cd70576b210134f9d5890219ebc830e6ee2c67f1aa827787974`.
- Programming: MRS OpenOCD reported `Programming Finished`, `Verified OK`, then reset.
- UART: stable by-id `/dev/serial/by-id/usb-wch.cn_WCH-Link_BC5C0A1DC679-if01`, 115200-8-N-1.
- Capture: 2440 raw bytes; machine gate accepted an ordered complete cycle containing board banner, exact firmware commit, PA9/PA10-to-CH549 wiring identity, and alive marker.
- Raw capture and JSON metadata: `uart-6fec1657.bin`, `uart-6fec1657.json`.

Classification: bounded openCH UART1/GPIO/HAL B1. This proves the fixed image can be verified into the attached CH32V307 and repeatedly transmit the identity-bound marker chain through onboard CH549 UART. It does not prove UART RX, interrupts/DMA, baud accuracy, other UART instances, GPIO electrical characteristics, recovery, performance, or endurance.
