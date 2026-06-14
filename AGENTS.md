# AGENTS.md

## Commands
- Default local build is PC simulation: `make` configures `build/pc` with `HAL_PLATFORM=PC`, `BUILD_TYPE=Release`, `BUILD_TESTS=OFF`, then builds it.
- Useful Makefile variants: `make BUILD_TYPE=debug`, `make HAL_PLATFORM=STM32U5`, `make HAL_PLATFORM=STM32F4 BUILD_TESTS=ON`, `make FOTA=ON`, `make configure`, `make distclean`.
- Direct CMake equivalent: `cmake -B build/pc -S . -DHAL_PLATFORM=PC -DCMAKE_BUILD_TYPE=Release && cmake --build build/pc -j$(nproc)`.
- Run PC unit tests only with `make test-unit`; this configures and runs `build/tests/unit` with `ctest --output-on-failure`.
- Run QEMU STM32F4 tests only with `make test-qemu`; it delegates to `tests/qemu_stm32f4/Makefile`.
- `make test` runs both PC unit tests and QEMU tests, so it requires QEMU plus the ARM toolchain.
- Focused QEMU checks: from `tests/qemu_stm32f4`, use `make list`, `make hal_test`, `make run-hal_test`, or `make test`.
- Root `BUILD_TESTING=ON` builds the AT unit tests under `tests/unit/net/` via `tests/CMakeLists.txt`; the independent `tests/unit` suite is what `make test-unit` runs.

## Toolchains And Platforms
- CMake platform values are exactly `PC`, `STM32F4`, `STM32U5`, `WCH`, and `HC32`; pass them as `-DHAL_PLATFORM=<value>` or `make HAL_PLATFORM=<value>`.
- Non-PC builds set cross-compilers before `project()`; STM32/HC32 default to `/home/eugene/Tools/arm-gnu-toolchain/bin/arm-none-eabi-*`.
- WCH builds default to `/usr/share/MRS2/MRS-linux-x64/resources/app/resources/linux/components/WCH/Toolchain/RISC-V Embedded GCC15/bin/riscv32-wch-elf-*`.
- Platform chip defaults can be overridden: `STM32F4_CHIP=STM32F407xx`, `STM32U5_CHIP=STM32U575xx`, `WCH_CHIP=CH32V30x`, `HC32_CHIP=HC32L021`.
- The repo uses submodules for MCU SDKs and USB/FlashDB dependencies; missing vendor headers usually means submodules are not initialized, not that includes should be rewritten.

## Architecture Rules
- Keep the layering: applications/projects/examples call components; components use OSAL and HAL; HAL implementations wrap vendor SDKs under `MCU/`.
- Do not edit generated/vendor SDK trees under `MCU/` or `third_party/` unless the task explicitly targets them.
- Drivers belong under `components/drivers/<category>/<driver_name>/` and should register with the `components/device` model instead of inventing standalone lifecycles.
- New reusable component code should have a `CMakeLists.txt`; root CMake auto-discovers top-level `components/*` except nested `components/kernel/{osal,misc}` and `components/clib/xy_clib`, which are added manually.
- Optional features/components should be represented in Kconfig as well as CMake when they are meant to be configurable.
- Use `hal_*` APIs from drivers/components instead of vendor SDK calls; use `osal_*` for RTOS abstraction and `xy_log` instead of `printf` in embedded code.
- Include `components/clib/xy_clib` facilities before standard-library assumptions in embedded-facing code; it defines project types, errors, and assertions.

## Style And Checks
- C is C99 in CMake, while `.clang-format` uses LLVM style with 4-space indentation, K&R braces, 100-column limit, left-aligned pointers, and sorted includes.
- Format touched C/H files with `clang-format -i <file>`; CI-style formatting excludes `third_party` and build directories.
- Naming conventions that matter here: `xy_` for framework utilities/components, `hal_` for HAL APIs, and `osal_` for OS abstraction APIs.
- `.clang-tidy` is present but tuned loosely for `src/`/`include/`; for repo-wide embedded C changes, build verification is usually more reliable than assuming tidy coverage.

## Documentation Notes
- `CLAUDE.md` is the prior compact instruction source and is mostly consistent, but the root `Makefile` is the source of truth for current build/test targets.
- Some README and older CI examples use stale options such as `-DPLATFORM=...`; prefer root `CMakeLists.txt` and `Makefile`, which use `HAL_PLATFORM`.
