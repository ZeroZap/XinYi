# XinYi Unified QEMU Deployment Guide

This guide describes the unified QEMU environment used by XinYi for STM32 and CH32V virtual tests.

The goal is to keep STM32 and CH32V tests on the same QEMU base version while still using the correct QEMU system binary for each architecture.

## Overview

XinYi uses one local QEMU 10.2.2 build with two softmmu targets:

```text
arm-softmmu,riscv32-softmmu
```

Installed binaries:

```text
/home/eugene/Tools/qemu-unified-10.2.2/bin/qemu-system-arm
/home/eugene/Tools/qemu-unified-10.2.2/bin/qemu-system-riscv32
```

Usage split:

| Platform | Binary | Machine | Test target |
|----------|--------|---------|-------------|
| STM32F4 | `qemu-system-arm` | `olimex-stm32-h405` | `make test-qemu` |
| CH32V307 | `qemu-system-riscv32` | `ch32v307` | `make test-qemu-ch32v` |

## Repository Layout

The local QEMU maintenance repository is outside the XinYi source tree:

```text
/home/eugene/zerozap/qemu-unified
```

It is based on:

```text
https://github.com/lintel/qemu-ch32v
```

Local additions in that repository:

```text
build-unified-qemu.sh
README_XINYI_UNIFIED.md
```

Local modification:

```text
build-wch-qemu.sh
```

The local `build-wch-qemu.sh` accepts:

```bash
WCH_QEMU_TARGET_LIST=arm-softmmu,riscv32-softmmu
```

This keeps the CH32/QingKe patches from `qemu-ch32v` while also building the official ARM QEMU target used by STM32 tests.

## Dependencies

Install build dependencies on Debian/Ubuntu:

```bash
sudo apt update
sudo apt install -y --no-install-recommends \
  build-essential git python3 ninja-build flex bison pkg-config \
  libglib2.0-dev libpixman-1-dev zlib1g-dev libslirp-dev
```

Toolchains used by XinYi tests are separate from QEMU:

| Platform | Toolchain |
|----------|-----------|
| STM32F4 | `/home/eugene/Tools/arm-gnu-toolchain/bin/arm-none-eabi-gcc` |
| CH32V | `/usr/share/MRS2/MRS-linux-x64/resources/app/resources/linux/components/WCH/Toolchain/RISC-V Embedded GCC15/bin/riscv32-wch-elf-gcc` |

## Build Unified QEMU

From the QEMU maintenance repository:

```bash
cd /home/eugene/zerozap/qemu-unified
./build-unified-qemu.sh --no-install-deps
```

Default install prefix:

```text
/home/eugene/Tools/qemu-unified-10.2.2
```

Override the install prefix:

```bash
cd /home/eugene/zerozap/qemu-unified
QEMU_UNIFIED_PREFIX=/home/eugene/Tools/qemu-unified-10.2.2 \
  ./build-unified-qemu.sh --no-install-deps
```

## Verify Installation

Check both QEMU binaries use the same version:

```bash
/home/eugene/Tools/qemu-unified-10.2.2/bin/qemu-system-arm --version
/home/eugene/Tools/qemu-unified-10.2.2/bin/qemu-system-riscv32 --version
```

Expected version:

```text
QEMU emulator version 10.2.2
```

Check STM32 machines:

```bash
/home/eugene/Tools/qemu-unified-10.2.2/bin/qemu-system-arm -machine help
```

STM32-related machines verified locally:

| Machine | Board | MCU/series | Core |
|---------|-------|------------|------|
| `olimex-stm32-h405` | Olimex STM32-H405 | STM32F405RG / STM32F4 | Cortex-M4 |
| `stm32vldiscovery` | ST STM32VLDISCOVERY | STM32F100RB / STM32F1 Value Line | Cortex-M3 |
| `b-l475e-iot01a` | B-L475E-IOT01A Discovery Kit | STM32L475 / STM32L4 | Cortex-M4 |

Check CH32 machines:

```bash
/home/eugene/Tools/qemu-unified-10.2.2/bin/qemu-system-riscv32 -machine help
/home/eugene/Tools/qemu-unified-10.2.2/bin/qemu-system-riscv32 -cpu help
```

CH32 machines verified locally:

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

QingKe CPU models verified locally:

```text
wch-qingke-v2a
wch-qingke-v2c
wch-qingke-v3a
wch-qingke-v3b
wch-qingke-v3c
wch-qingke-v3f
wch-qingke-v3v
wch-qingke-v4a
wch-qingke-v4b
wch-qingke-v4c
wch-qingke-v4f
wch-qingke-v4j
wch-qingke-v5f
```

## Run XinYi Tests

Run STM32F4 QEMU tests:

```bash
make test-qemu
```

Expected result from the current suite:

```text
hal_test:        PASS=11 FAIL=0
alg_test:        PASS=17 FAIL=0
components_test: PASS=18 FAIL=0
Total: PASS=46 FAIL=0
```

Run CH32V QEMU tests:

```bash
make test-qemu-ch32v
```

Expected result:

```text
smoke_test: PASS=1 FAIL=0
Total: PASS=1 FAIL=0
```

Focused commands:

```bash
make -C tests/qemu_stm32f4 test
make -C tests/qemu_ch32v test
```

## Default Paths In XinYi

STM32 QEMU default selection:

```make
QEMU_UNIFIED_ARM ?= /home/eugene/Tools/qemu-unified-10.2.2/bin/qemu-system-arm
QEMU ?= $(if $(wildcard $(QEMU_UNIFIED_ARM)),$(QEMU_UNIFIED_ARM),qemu-system-arm)
```

Defined in:

```text
tests/qemu_stm32f4/Makefile
```

CH32V QEMU default selection:

```make
QEMU_UNIFIED_RISCV32 ?= /home/eugene/Tools/qemu-unified-10.2.2/bin/qemu-system-riscv32
QEMU ?= $(if $(wildcard $(QEMU_UNIFIED_RISCV32)),$(QEMU_UNIFIED_RISCV32),qemu-system-riscv32)
```

This keeps local XinYi machines on unified QEMU 10.2.2 while allowing other environments to use `PATH`-provided QEMU binaries.

Defined in:

```text
tests/qemu_ch32v/Makefile
```

Override at runtime if needed:

```bash
make -C tests/qemu_stm32f4 test QEMU=/path/to/qemu-system-arm
make -C tests/qemu_ch32v test QEMU=/path/to/qemu-system-riscv32
```

## Manual Run Examples

STM32F4:

```bash
/home/eugene/Tools/qemu-unified-10.2.2/bin/qemu-system-arm \
  -M olimex-stm32-h405 \
  -nographic \
  -kernel tests/qemu_stm32f4/hal_test/hal_test.elf \
  -semihosting
```

CH32V307:

```bash
/home/eugene/Tools/qemu-unified-10.2.2/bin/qemu-system-riscv32 \
  -M ch32v307 \
  -display none \
  -serial stdio \
  -kernel tests/qemu_ch32v/smoke_test/smoke_test.elf
```

## Maintenance Workflow

Update or rebuild the local QEMU maintenance repository:

```bash
cd /home/eugene/zerozap/qemu-unified
git status --short
git fetch --all --prune
```

Rebuild after local changes:

```bash
./build-unified-qemu.sh --no-install-deps
```

Then verify from XinYi:

```bash
cd /home/eugene/zerozap/XinYi
make test-qemu
make test-qemu-ch32v
```

## Troubleshooting

If QEMU source download fails, reuse a known-good cached archive when available:

```text
/tmp/opencode/qemu-ch32v/downloads/archives/qemu-10.2.2.tar.xz
```

Copy it into the unified repository cache:

```bash
cp -f \
  /tmp/opencode/qemu-ch32v/downloads/archives/qemu-10.2.2.tar.xz \
  /home/eugene/zerozap/qemu-unified/downloads/archives/qemu-10.2.2.tar.xz
```

If `sudo` cannot read a password from the current automation session, install dependencies in a normal terminal first, then run:

```bash
cd /home/eugene/zerozap/qemu-unified
./build-unified-qemu.sh --no-install-deps
```

If QEMU machines are missing, check that the unified build was configured with both targets:

```bash
cd /home/eugene/zerozap/qemu-unified
grep WCH_QEMU_TARGET_LIST build-wch-qemu.sh build-unified-qemu.sh
```

Expected target list:

```text
arm-softmmu,riscv32-softmmu
```

## Notes

- STM32 and CH32V do not use the same executable. They share the same QEMU source/version/install prefix, but STM32 uses `qemu-system-arm` and CH32V uses `qemu-system-riscv32`.
- The STM32 machines are from official QEMU 10.2.2 ARM support.
- The CH32 machines come from the `qemu-ch32v` overlay and patches.
- The old system QEMU 8.2.2 remains available as a fallback but is no longer the XinYi default.
