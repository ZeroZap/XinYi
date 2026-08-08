# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build System

The project uses CMake with a Makefile wrapper. Treat the root `Makefile` and
`AGENTS.md` as the source of truth when commands drift. Non-PC builds default to
the ARM GCC toolchain under `/home/eugene/Tools/arm-gnu-toolchain/bin/` unless a
toolchain path is overridden by CMake/toolchain settings.

```bash
# PC simulation (default local build)
make

# Direct PC CMake equivalent
cmake -B build/pc -S . -DHAL_PLATFORM=PC -DCMAKE_BUILD_TYPE=Release
cmake --build build/pc -j$(nproc)

# PC debug build
make BUILD_TYPE=debug

# STM32F4 with test sources enabled for build/QEMU flows
make HAL_PLATFORM=STM32F4 BUILD_TESTS=ON

# STM32U5 with FOTA enabled
make HAL_PLATFORM=STM32U5 FOTA=ON

# Common maintenance targets
make configure
make clean
make distclean
```

Platform is selected via `-DHAL_PLATFORM=` or `make HAL_PLATFORM=...` (values:
`PC`, `STM32F4`, `STM32U5`, `STM32L4`, `WCH`, `HC32`). Kconfig (`Kconfig` at root)
controls which components are included; the Makefile passes feature overrides through
`KCONFIG_OVERRIDES`.

## Running Tests

The active PC unit suite is the independent `tests/unit` CMake project. Unit
tests link the repo-local Unity copy under `tests/unity/`.

```bash
# Run PC unit tests only (preferred fast regression gate)
make test-unit

# Run all configured test suites: PC unit tests plus QEMU STM32F4 tests
make test

# Run only QEMU STM32F4 tests
make test-qemu

# Focused QEMU checks
make -C tests/qemu_stm32f4 list
make -C tests/qemu_stm32f4 hal_test
make -C tests/qemu_stm32f4 run-hal_test
```

AT client/server host coverage is registered in `tests/unit/CMakeLists.txt` as
`at_client` / `at_server` and is covered by `make test-unit`; there is currently
no root `tests/CMakeLists.txt` suite.

## Code Style

```bash
clang-format -i <file>          # Auto-format (LLVM, 100-col, 4-space)
clang-tidy <file> -- -I./components  # Static analysis
```

The project enforces C99. Naming: `xy_` prefix for library utilities, `hal_` for HAL APIs, `osal_` for RTOS abstraction APIs.

## Architecture

Five-layer architecture (top to bottom):

```
Application / Examples / Projects
        ↓
Components  (sensor, net, crypto, gui, dm, fota, pid, pm, ...)
        ↓
OSAL  (OS abstraction: FreeRTOS | RT-Thread | CMSIS-RTX | bare-metal)
        ↓
HAL   (102 unified APIs: GPIO, UART, SPI, I2C, ADC, DAC, ...)
        ↓
SDK HAL  (MCU/ST/STM32F4, MCU/ST/STM32U5, MCU/wch/CH32, PC sim)
```

**Key directories:**
- `components/hal/` — 102 platform-independent hardware APIs. Implementations live under `MCU/` submodules.
- `components/kernel/osal/backend/` — four RTOS backends sharing a common API surface.
- `components/device/` — unified device model; all sensor/display/storage drivers register here.
- `components/clib/xy_clib/` — the project's C runtime shim (type defs, error codes, assertions). Include this before standard headers in embedded code.
- `components/crypto/` — SM2/SM3/SM4 (Chinese standards) plus AES, HMAC, CRC, Base64.
- `components/net/` — MQTT client, Modbus RTU/TCP, AT server, ISO7816.
- `components/dm/` — data management: EEPROM, NOR Flash, FlashDB (KV store).
- `components/fota/` — dual-bank OTA with bootloader handoff (STM32U5 reference: 128 KB boot + 2×256 KB app slots).
- `components/trace/` — `xy_log` logging macros; use these instead of `printf`.
- `projects/` — real firmware images; `examples/` — standalone demos.
- `tests/` — QEMU and PC-simulation test suites.
- `MCU/` — vendor SDK submodules (ST CubeF1/F4/U5, WCH CH32, TI LM3S). Do not edit generated SDK files.

## Adding a New Driver

1. Create `components/drivers/<category>/<driver_name>/`.
2. Implement against the `components/device/` device model (register via `xy_device_register()`).
3. Call only `hal_*` APIs — never vendor SDK APIs directly.
4. Add a Unity test under `tests/` or the component's own `test/` subdirectory.
5. Wire into `CMakeLists.txt` component auto-detection block and add a Kconfig option if the driver is optional.

## Platform Simulation

The `PC` platform compiles the full stack to a native Linux binary using POSIX stubs for HAL. This is the fastest way to iterate on component logic without hardware. UART is mapped to stdin/stdout; GPIO/SPI/I2C are no-ops that log calls via `xy_log`.
