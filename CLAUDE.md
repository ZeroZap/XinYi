# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build System

The project uses CMake with a Makefile wrapper. The ARM GCC toolchain must be at `/home/eugene/Tools/arm-gnu-toolchain/bin/`.

```bash
# PC simulation (fastest iteration)
mkdir -p build_xinyi && cd build_xinyi
cmake .. -DCMAKE_BUILD_TYPE=Release && make -j$(nproc)

# STM32F4 with QEMU test support
cd build_stm32f4_test
cmake .. -DHAL_PLATFORM=STM32F4 -DBUILD_TESTING=ON && make -j$(nproc)

# STM32U5 with FOTA
cd build_stm32u5_fota
cmake .. -DHAL_PLATFORM=STM32U5 -DXY_FOTA_ENABLED=ON && make -j$(nproc)

# Via Makefile wrapper
make all                  # Default build
make BUILD_TYPE=debug     # Debug build (-O0 -g3)
make BUILD_TESTS=1        # Include unit tests
make clean
```

Platform is selected via `-DHAL_PLATFORM=` (values: `PC`, `STM32F4`, `STM32U5`, `WCH`, `HC32`). Kconfig (`Kconfig` at root) controls which components are included.

## Running Tests

Unit tests use the Unity framework (`third_party/unity/`). QEMU-based integration tests exist for STM32F4:

```bash
# Run QEMU HAL tests
cd tests/qemu_stm32f4/hal_test && make && qemu-system-arm ...

# Run PC-simulation component tests
cd build_xinyi && make test
```

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
