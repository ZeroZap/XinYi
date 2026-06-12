# QEMU Deployment Guide

This document records the QEMU environments used by XinYi for STM32 and CH32 virtual tests.

For the full XinYi unified QEMU build and deployment workflow, see `docs/XINYI_UNIFIED_QEMU_DEPLOY_GUIDE.md`.

## Current Status

| Target | QEMU binary | Version | Machine | Test entry |
|--------|-------------|---------|---------|------------|
| STM32F4 | `/home/eugene/Tools/qemu-unified-10.2.2/bin/qemu-system-arm` | `10.2.2` | `olimex-stm32-h405` | `make test-qemu` |
| CH32V307 | `/home/eugene/Tools/qemu-unified-10.2.2/bin/qemu-system-riscv32` | `10.2.2` | `ch32v307` | `make test-qemu-ch32v` |

The local unified QEMU maintenance repository is:

```text
/home/eugene/zerozap/qemu-unified
```

It is based on `https://github.com/lintel/qemu-ch32v` and builds both `arm-softmmu` and `riscv32-softmmu` from QEMU 10.2.2.

## STM32 QEMU

The STM32 tests prefer the unified local QEMU 10.2.2 `qemu-system-arm` binary and fall back to `qemu-system-arm` from `PATH` if the unified binary is not installed.

The previous verified system package was:

```text
qemu-system-arm 8.2.2 (Debian 1:8.2.2+ds-0ubuntu1.16)
```

It remains usable as a fallback, but the project default is now the unified local QEMU.

Fallback system install:

```bash
sudo apt update
sudo apt install -y qemu-system-arm gdb-multiarch
```

Verify:

```bash
qemu-system-arm --version
qemu-system-arm -machine help
```

Unified QEMU verify:

```bash
/home/eugene/Tools/qemu-unified-10.2.2/bin/qemu-system-arm --version
/home/eugene/Tools/qemu-unified-10.2.2/bin/qemu-system-arm -machine help
```

STM32-related machines verified in unified QEMU 10.2.2:

| QEMU machine | Board | MCU/series | Core |
|--------------|-------|------------|------|
| `olimex-stm32-h405` | Olimex STM32-H405 | STM32F405RG / STM32F4 | Cortex-M4 |
| `stm32vldiscovery` | ST STM32VLDISCOVERY | STM32F100RB / STM32F1 Value Line | Cortex-M3 |
| `b-l475e-iot01a` | B-L475E-IOT01A Discovery Kit | STM32L475 / STM32L4 | Cortex-M4 |

Run XinYi STM32F4 QEMU tests:

```bash
make test-qemu
```

Focused command:

```bash
make -C tests/qemu_stm32f4 test
```

Default STM32F4 QEMU command shape:

```bash
/home/eugene/Tools/qemu-unified-10.2.2/bin/qemu-system-arm \
  -M olimex-stm32-h405 \
  -nographic \
  -kernel firmware.elf \
  -semihosting
```

## CH32 QEMU

The CH32 tests prefer the unified local QEMU 10.2.2 `qemu-system-riscv32` binary. The CH32 support comes from `lintel/qemu-ch32v`, based on QEMU 10.2.2 with WCH CH32/QingKe models.

Unified install path used by this workspace:

```text
/home/eugene/Tools/qemu-unified-10.2.2
```

Binaries:

```text
/home/eugene/Tools/qemu-unified-10.2.2/bin/qemu-system-arm
/home/eugene/Tools/qemu-unified-10.2.2/bin/qemu-system-riscv32
```

Build or rebuild unified QEMU from the local maintenance repository:

```bash
cd /home/eugene/zerozap/qemu-unified
./build-unified-qemu.sh --no-install-deps
```

Install dependencies first if needed:

```bash
sudo apt update
sudo apt install -y --no-install-recommends \
  build-essential git python3 ninja-build flex bison pkg-config \
  libglib2.0-dev libpixman-1-dev zlib1g-dev libslirp-dev
```

Verify:

```bash
/home/eugene/Tools/qemu-unified-10.2.2/bin/qemu-system-riscv32 --version
/home/eugene/Tools/qemu-unified-10.2.2/bin/qemu-system-riscv32 -machine help
/home/eugene/Tools/qemu-unified-10.2.2/bin/qemu-system-riscv32 -cpu help
```

Supported CH32 machines verified locally:

```text
ch32h417
ch32v003
ch32v103
ch32v203
ch32v203rb
ch32v303
ch32v305
ch32v307
ch32v317
ch32v407
```

Run XinYi CH32V QEMU tests:

```bash
make test-qemu-ch32v
```

Focused command:

```bash
make -C tests/qemu_ch32v test
```

Default CH32V QEMU command shape:

```bash
/home/eugene/Tools/qemu-unified-10.2.2/bin/qemu-system-riscv32 \
  -M ch32v307 \
  -display none \
  -serial stdio \
  -kernel firmware.elf
```

## Toolchains

STM32F4 QEMU tests default to:

```text
/home/eugene/Tools/arm-gnu-toolchain/bin/arm-none-eabi-gcc
```

CH32V QEMU tests default to:

```text
/usr/share/MRS2/MRS-linux-x64/resources/app/resources/linux/components/WCH/Toolchain/RISC-V Embedded GCC15/bin/riscv32-wch-elf-gcc
```

Override CH32 paths when needed:

```bash
make -C tests/qemu_ch32v test \
  QEMU=/path/to/qemu-system-riscv32 \
  WCH_TOOLCHAIN_BIN="/path/to/wch/toolchain/bin"
```

## Notes

- Official QEMU does not provide broad STM32 family coverage. It only exposes board-level models such as `olimex-stm32-h405` and `stm32vldiscovery`.
- `qemu-ch32v` is separate from official `qemu-system-arm`; it builds `qemu-system-riscv32` for CH32/QingKe RISC-V MCUs.
- Existing STM32 QEMU tests remain independent from CH32 QEMU tests.
- XinYi uses a unified local QEMU 10.2.2 install so STM32 and CH32 tests share the same QEMU version while still using different system binaries.
