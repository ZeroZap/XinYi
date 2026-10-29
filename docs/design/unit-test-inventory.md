# Unit Test Inventory

> Generated from the current `tests/unit` tree as the baseline for the Unity + CTest + FFF refactor.

## Summary

- Total C files under `tests/unit`: 200
- Unity-style files: 197
- Raw `assert()` files: 0
- Mixed Unity + raw `assert()` files: 0
- Plain/support helper C files without obvious Unity/assert markers: 3
- Registered CTest entries in `tests/unit/CMakeLists.txt`: 304 (measured: `ctest -N` reports 304;
  the C targets are registered through `xy_add_unit_test(...)`, so `add_test` calls in this file are
  only the explicit Python/board entries)
- Unwired source `.c` files under `tests/unit`: 0
- Python-only CTests are counted in the CTest total but not in the C-file counts above; the newest
  is `pandora_runtime_timeline` (`tests/unit/kernel/test_runtime_timeline.py`, 11 cases) which
  fixes the semantics of `boards/pandora_stm32l475/validate_runtime_timeline.py`: host-arrival
  timestamps are turned into per-cycle wall time, the largest marker-to-marker gaps, and the excess
  wall time inside windows whose only firmware work is a known `xy_os_delay()`, and proves the
  capture path against a real PTY pair; `pandora_fota_candidate_programmer_capture_validator`
  (`tests/unit/fota/test_pandora_fota_candidate_programmer_capture.py`, 19 cases) which judges the
  W25Q128 candidate-programmer marker stream (`READY` → `PROGRAMMER_COMMIT` →
  `CANDIDATE_SOURCE_COMMIT` → `PROGRAMMED_VERIFIED`), binds both commit identities, and rejects a
  truncated final frame, error markers, unknown `PANDORA_FOTA_CANDIDATE_*` markers and foreign
  output inside a frame; `pandora_icm20608_capture_validator`
  (`tests/unit/sensor/test_pandora_icm20608_capture_validator.py`) judges `ICM20608_*` captures;
  `pandora_aht10_capture_validator` (`tests/unit/sensor/test_pandora_aht10_capture.py`,
  9 cases) which captures with its own `capture` subcommand (UART opened and flushed before the
  target reset) and judges `AHT10 SAMPLE` windows for ordering, the exact firmware commit and the
  AHT10 public physical range; `pandora_fota_bootloader_capture_validator`
  (`tests/unit/fota/test_pandora_fota_bootloader_capture.py`, 47 cases) judges `PANDORA_BOOT_*`
  captures and decodes the boot journal.
- Inventory scope excludes build-generated files such as `tests/unit/build/**`.
- First-party-looking test files outside `tests/unit` are triaged separately so vendor tests and
  production self-test implementations are not mistaken for missing Unity migrations.

## Outside-`tests/unit` Triage

| Path | Classification | Action |
| --- | --- | --- |
| `components/crypto/curve25519-cortexm0-20150813/test/*.c` | Upstream/vendor Curve25519 Cortex-M0 test helpers | Keep in place; excluded from the project Unity/CTest unit inventory. |
| `components/sensor/sensor_self_test.c` | Production Sensor self-test implementation | Keep in component source; covered through `tests/unit/sensor/test_sensor_framework.c`. |
| `projects/Bank/ontroller-charger-test.c` | Imported project-local patch transcript, not an active CTest source | Leave out of unit conversion; treat as project-specific legacy material unless the Bank project is intentionally rehabilitated. |
| `tools/tests/*.py` | Host-side Python tool tests (z-serial + HIL harnesses), run by `make test-tools` | Keep separate from the C Unity/CTest inventory; the C counts above stay authoritative for firmware code. |

## Component Breakdown

| Component | Total | Unity | Raw assert | Mixed | Plain/helper | Fake-heavy candidates |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `actuator` | 1 | 1 | 0 | 0 | 0 | 1 |
| `analog_devices` | 1 | 1 | 0 | 0 | 0 | 1 |
| `clib` | 1 | 1 | 0 | 0 | 0 | 0 |
| `crypto` | 13 | 12 | 0 | 0 | 1 | 2 |
| `device` | 6 | 6 | 0 | 0 | 0 | 4 |
| `display` | 5 | 5 | 0 | 0 | 0 | 2 |
| `dm` | 6 | 6 | 0 | 0 | 0 | 2 |
| `fota` | 8 | 8 | 0 | 0 | 0 | 3 |
| `framework` | 1 | 1 | 0 | 0 | 0 | 1 |
| `fuel_gauge` | 6 | 6 | 0 | 0 | 0 | 6 |
| `gui` | 7 | 7 | 0 | 0 | 0 | 1 |
| `hal_component` | 6 | 6 | 0 | 0 | 0 | 4 |
| `ipc` | 2 | 2 | 0 | 0 | 0 | 1 |
| `kernel` | 4 | 4 | 0 | 0 | 0 | 1 |
| `mux` | 5 | 5 | 0 | 0 | 0 | 5 |
| `net` | 17 | 17 | 0 | 0 | 0 | 5 |
| `pid` | 3 | 2 | 0 | 0 | 1 | 0 |
| `pm` | 3 | 3 | 0 | 0 | 0 | 1 |
| `sensor` | 48 | 48 | 0 | 0 | 0 | 48 |
| `storage` | 1 | 1 | 0 | 0 | 0 | 1 |
| `support` | 1 | 1 | 0 | 0 | 0 | 0 |
| `sys` | 1 | 1 | 0 | 0 | 0 | 0 |
| `trace` | 1 | 1 | 0 | 0 | 0 | 0 |

> Note: the summary above is measured from the tree; the per-component breakdown is a legacy
> snapshot that has not been regenerated in lockstep (several rows, e.g. `sensor`, `gui`, `net`,
> undercount the current files). Only the `fota` row was resynchronized in the slice that added
> `test_pandora_fota_flash`, the `fota` row again for `test_pandora_fota_boot_flow`, and the
> `hal_component` row in the slices that added
> `test_pandora_i2c_owners`, `test_pandora_platform_startup`, `test_pandora_device_stack`,
> `test_pandora_hw_i2c_stack` and `test_pandora_sensor_owners`.
> The summary counts above were re-measured from the tree in the `test_pandora_sensor_owners`
> slice. A full breakdown regeneration remains a separate docs-sync slice.

## Migration Priority

Note: `tests/unit/CMakeLists.txt` also registers component example compile-smoke targets that
reuse component example sources rather than adding extra standalone `tests/unit/*.c` files; currently
`pid_example_basic`, `pid_example_incremental`, `pid_example_auto_tune`,
`pid_example_temperature`, `pid_example_charging`, `actuator_example_relay_servo_pwm`,
`crypto_smoke_example`, `crypto_root_target_smoke`, `fota_smoke_example`,
`lte_hal_uart_smoke_example`, and `fuel_gauge_smbus_hardware_smoke_example`
guard API drift in those examples/skeletons.

1. Keep all new tests on Unity assertions.
2. Keep the raw `assert()` and unwired-source inventories at zero for tracked `tests/unit` sources.
3. Use FFF first on fake-heavy targets where call count, argument capture, or return sequencing matters.
4. Keep simple local stubs for tiny deterministic dependencies.
5. Re-triage first-party-looking files outside `tests/unit` before treating them as migration work.

## File Classification

| File | Style | Fake signal count |
| --- | --- | ---: |
| `tests/unit/actuator/test_actuator_framework.c` | `unity` | 18 |
| `tests/unit/analog_devices/test_analog_devices.c` | `unity` | 11 |
| `tests/unit/clib/test_clib_core.c` | `unity` | 0 |
| `tests/unit/crypto/test_25519.c` | `unity` | 0 |
| `tests/unit/crypto/test_25519_m0.c` | `unity` | 6 |
| `tests/unit/crypto/test_cipher_hmac.c` | `unity` | 0 |
| `tests/unit/crypto/test_blake2.c` | `unity` | 0 | Focused BLAKE2s host vectors, incremental/keyed behavior, invalid-parameter guards, and output-preservation contract; API-drift guard only, not security/provenance validation. |
| `tests/unit/crypto/test_crc.c` | `unity` | 2 |
| `tests/unit/crypto/test_crypto_smoke_example.c` | `unity` | 0 | Host-safe Crypto public Base64/Hex/SHA-256/simple-RNG smoke; API-drift guard only, not security audit/compliance/hardware acceleration validation. |
| `tests/unit/crypto/crypto_root_target_smoke_probe/main.c` | plain CTest probe | 0 | Standalone public consumer linked against real root `xy_tiny_crypto`; proves aggregate `src/` Base64/Hex/SHA-256 API smoke only, not duplicate-source reconciliation or security/hardware validation. |
| `tests/unit/crypto/test_csprng.c` | `unity` | 0 |
| `tests/unit/crypto/test_encode.c` | `unity` | 0 |
| `tests/unit/crypto/test_hash.c` | `unity` | 0 |
| `tests/unit/crypto/test_lwc.c` | `unity` | 0 |
| `tests/unit/crypto/test_random.c` | `unity` | 0 |
| `tests/unit/crypto/test_sm2.c` | `unity` | 0 |
| `tests/unit/device/test_auto_register.c` | `unity` | 0 |
| `tests/unit/device/test_device_async_helper.c` | `unity` | 12 |
| `tests/unit/device/test_device_framework.c` | `unity` | 12 | Registry/PM lifecycle coverage, including the `xy_device_open()`/`xy_device_close()` ↔ registry reference accounting contract (open holds a PM-visible reference; close retires only the last reference and fails closed otherwise). |
| `tests/unit/device/test_device_bus_contract.c` | `unity` | 3 | Canonical bus-helper return contract (SPI transfer = `XY_DEVICE_OK`, send/recv = byte count, UART send/recv = `XY_DEVICE_OK`) plus the real `xy_w25qxx_read_id()` transfer sequence; host-only, not flash or SPI electrical evidence. |
| `tests/unit/device/test_device_pm.c` | `unity` | 1 |
| `tests/unit/device/test_spi_device.c` | `unity` | 40 |
| `tests/unit/display/test_display_lcd.c` | `unity` | 0 |
| `tests/unit/display/test_display_oled_ws2812.c` | `unity` | 0 |
| `tests/unit/display/test_display_rgb_matrix.c` | `unity` | 4 |
| `tests/unit/display/test_display_serial_rgb_headers.c` | `unity` | 0 |
| `tests/unit/display/test_led_driver.c` | `unity` | 10 |
| `tests/unit/dm/test_dm_base64.c` | `unity` | 0 |
| `tests/unit/dm/test_dm_corejson.c` | `unity` | 0 |
| `tests/unit/dm/test_dm_factory.c` | `unity` | 1 |
| `tests/unit/dm/test_dm_fee.c` | `unity` | 12 |
| `tests/unit/dm/test_dm_fs.c` | `unity` | 0 | FS lifecycle, path dispatch, I/O, seek and close-error contracts |
| `tests/unit/dm/test_dm_json.c` | `unity` | 0 | active `xy_json` parse, mutation, malformed-input and guard contracts |
| `tests/unit/dm/test_dm_nvm_core.c` | `unity` | 0 |
| `tests/unit/dm/test_dm_tlv.c` | `unity` | 0 |
| `tests/unit/fota/test_fota_core.c` | `unity` | 15 |
| `tests/unit/fota/test_pandora_fota_boot_flow.c` | `unity` | 21 | Pandora resident-bootloader decision sequence (`pandora_fota_boot_flow.c`) with every board operation faked: pins the order candidate-validate -> reviewed-restage -> install-once -> confirm-request/ack or record-attempt -> jump, including the confirm path never recording an attempt and the attempt path never confirming, the confirm slot being scoped to one install cycle (a request or acknowledgement present when `install_once` writes the image is cleared before the slot is read, so a stale request can never confirm a candidate that has not executed, and a clear failure halts with `PANDORA_BOOT_CONFIRM_REQUEST_ERROR` instead of continuing), plus fail-closed short circuits (`NO_VALID_IMAGE`, install/attempt/confirm errors, RTC backup read/write failures, exhausted attempts -> `ROLLBACK_REQUIRED`, all-NULL ops, and each individually missing required callback) and the marker sequence the capture validators check. Host contract only - no Flash erase/program, RTC retention, power-loss or vector-handoff evidence. |
| `tests/unit/fota/test_pandora_fota_flash.c` | `unity` | 9 | Pandora board-owned internal-Flash adapters (`pandora_fota_flash.c` / `pandora_fota_install_flash.c`) over a fake `xy_hal_flash_*`: range/8-byte-granule/two-slot-erase guards, init-unlock-write-lock ordering, failure propagation, and the real two-slot journal core driven through the board backend. Host contract only — no flash write/erase, power-loss, endurance or bootloader handoff evidence. |
| `tests/unit/hal_component/test_pandora_i2c_owners.c` | `unity` | 6 | Pandora board-owned I2C transports (`pandora_soft_i2c.c` / `pandora_hw_i2c.c`) over a host-only fake `stm32l4xx_hal.h` and fake `xy_hal_gpio_*`: GPIO event streams decoded back into I2C frames (pin mapping, START/STOP, `dev_addr << 1`, MSB-first bytes, per-byte ACK sampling, NACK propagation, receive read bit and tail NACK), the soft bus `mem_read`/`mem_write`/`is_device_ready` subset, and hw I2C3 AF4 config with `Instance` assigned before init. Host contract only — no timing, clock-stretching, bus-recovery or real-device evidence. |
| `tests/unit/hal_component/test_pandora_platform_startup.c` | `unity` | 4 | Pandora board-owned clock/power owner (`pandora_platform_startup.c`) over the same host-only fake `stm32l4xx_hal.h`: counted PWR-clock / VOS1 / `HAL_RCC_OscConfig` / `HAL_RCC_ClockConfig` hooks prove the ascending call order, per-stage failure short-circuit, HSE-only oscillator payload, HSE-driven PLL factors, all-DIV1 bus dividers and `FLASH_LATENCY_4`; datasheet invariants derived from the board's own `HSE_VALUE=8000000` pin VCO input/output ranges and the exact 80 MHz SYSCLK. Host contract only — no HSE start-up, PLL lock, real SYSCLK, bus timing or power-transition evidence. |
| `tests/unit/hal_component/test_pandora_device_stack.c` | `unity` | 14 | Pandora I2C -> Device -> driver vertical slice: a host-only I2C *slave* model (address decode, ACK/NACK, register file for the register-addressed device, command/frame model for the command-addressed device, per-phase frame log) is driven by the real board transport `pandora_soft_i2c.c` through the real `xy_i2c_device_*` bus helpers into the canonical `xy_aht10` / `xy_icm20608` Device drivers, so identity binding (WHO_AM_I 0xAE), the documented configure-and-read-back order, 14-byte sample decoding, AHT10 busy/cache behaviour, address NACK fail-closed paths and two-device address isolation are observed end to end. Host contract only - no timing, clock-stretching, real-device or board-runtime evidence. |
| `tests/unit/hal_component/test_pandora_sensor_owners.c` | `unity` | 23 | Pandora board-owned canonical sensor owners: `pandora_ap3216c_owner.c` / `pandora_icm20608_owner.c` / `pandora_aht10_owner.c` make the canonical Device drivers the instances the three sensor validation images own, replacing the legacy `sensor_*` wrappers (`ap3216c_create_light`, `icm20608_create_accel`), the ICM image's reach into `icm20608_priv_t`, and the AHT10 protocol copy that lived inline in the board smoke image. Two host-side I2C register files (0x1E / 0x68) plus an AHT10 command model (0x38) sit on the `xy_hal_i2c_*` seam, so one fixture pins all three owners: AP3216C reset-then-mode init with both 50 ms settles, the 6-byte IR/ALS/PS raw block, atomic three-channel sampling (ALS `*350` millilux, 12-bit PS mask, 10-bit IR mask) and cache mirroring; ICM20608 identity-before-any-write (foreign identity -> `XY_DEVICE_NOT_FOUND` with zero writes), reset/settle plus the documented seven write-and-read-back registers, the 14-byte burst decode at 4 g / 500 dps (500/-500/125 mg, ±3906 mdps, 3126 centi-C); AHT10 one `0xE1 0x08 0x00` calibration write with the 10 ms settle, `0xAC 0x33 0x00` measure then one bare 6-byte frame read decoding to 5000 centi-%RH / 2500 centi-C, and fail-closed init/sample paths that preserve the owner and the caller's output on a NACKing bus or a busy status. Host contract only - no timing, real-device or board-runtime evidence. |
| `tests/unit/hal_component/test_pandora_hw_i2c_stack.c` | `unity` | 13 | Pandora hardware I2C3 -> Device -> driver vertical slice for the handle transport style: `pandora_hw_i2c.c` (vendor `Instance`/AF4/RCC handle path) feeds the real `xy_i2c_device_*` helpers into the canonical `xy_ap3216c` driver against a host-side AP3216C register model on the `xy_hal_i2c_*` seam, pinning the 7-bit address and 100 ms timeout carried per transfer, reset-then-mode init with both 50 ms settle delays, ALS (`*350`), PS (12-bit mask plus the 0x40 out-of-range flag) and IR (10-bit mask) decoding, register-select-then-read framing per quantity, live-owner/cache preservation on a failed re-init, fail-closed reads/deinit on a NACKing device, and power-down on deinit. Host contract only - no timing, clock-stretching, real-device or board-runtime evidence. |
| `tests/unit/fota/test_fota_smoke_example.c` | `unity` | 0 | Host-safe public FOTA flow smoke; fake Flash callbacks only, not board/bootloader validation. |
| `tests/unit/framework/test_fff_smoke.c` | `unity` | 72 |
| `tests/unit/fuel_gauge/test_fg_bq27z561.c` | `unity` | 42 |
| `tests/unit/fuel_gauge/test_fg_bq27z746.c` | `unity` | 39 |
| `tests/unit/fuel_gauge/test_fg_bq40z50.c` | `unity` | 115 |
| `tests/unit/fuel_gauge/test_fg_max17043.c` | `unity` | 29 |
| `tests/unit/fuel_gauge/test_fuel_gauge_core.c` | `unity` | 59 |
| `tests/unit/fuel_gauge/test_fuel_gauge_smbus_hardware_smoke_example.c` | `unity` | 0 | Fuel Gauge SMBus board-validation smoke skeleton; fake-I2C only, hardware evidence remains pending. |
| `tests/unit/gui/test_gui_core.c` | `unity` | 22 |
| `tests/unit/gui/test_gui_display_backend.c` | `unity` | 0 | GUI core to fake display backend bridge contract; host-only forwarding/output-preservation guard, not hardware validation. |
| `tests/unit/gui/test_gui_ssd1306_adapter.c` | `unity` | 0 | GUI to SSD1306 display-driver adapter contract; host-only mono mapping/draw-line/draw-rect/draw-char callback forwarding/flush bus-failure propagation/multi-instance/slot-exhaustion guard, not hardware validation. |
| `tests/unit/gui/test_gui_sdl_backend.c` | `unity` | 0 | GUI SDL2 backend configure/init, RGB565 framebuffer, flush-error, cleanup, and quit-event contracts through a fake SDL seam; host-only and not real SDL window/runtime evidence. |
| `tests/unit/gui/test_gui_effects.c` | `unity` | 0 | GUI fade/blink/breath/slide/rotate algorithm and guard contracts. |
| `tests/unit/gui/test_gui_font_engine.c` | `unity` | 0 | GUI runtime font engine glyph/measurement/draw/cache contracts with local tick/log stubs; host-only, not font art or hardware rendering validation. |
| `tests/unit/gui/test_gui_font_manifest.c` | `unity` | 0 | GUI bitmap font manifest/inventory contracts for legacy ASCII/Chinese assets; validates current ranges, required UI codepoints, and duplicate/placeholder inventory; paired with `gui_font_generator_*` Python CTests for generator-bootstrap manifest schema, manifest header write-path, glyph metadata/preview/write-path validation, checked-in generated-preview byte-for-byte reproducibility, and compile guard, without claiming generated font quality or hardware validation. |
| `tests/unit/gui/test_gui_font_snapshot.c` | `unity` | 0 | GUI runtime font framebuffer snapshot smoke: deterministic host-only ASCII-art/checksum metadata, unknown-glyph preservation, and clipping guard; not font-art approval or hardware rendering validation. |
| `tests/unit/gui/test_gui_fonts.c` | `unity` | 0 | GUI bitmap font lookup and measurement contracts for ASCII 8x16/16x24 and Chinese 16x16 assets. |
| `tests/unit/gui/test_gui_led_screen_effects.c` | `unity` | 0 | GUI LED-screen extended effects public header self-containment; host include/type/signature contract only, not hardware validation. |
| `tests/unit/gui/test_gui_rgb_extended_effects_compile.c` | `unity` | 0 | GUI RGB extended music/matrix/extended/3D effects compile-boundary guard with fake serial-RGB seam, test-owned frame counter, delay/color helper seams, and deterministic low-risk smoke paths; not visual or hardware validation. |
| `tests/unit/gui/test_gui_widget_theme.c` | `unity` | 0 |
| `tests/unit/gui/test_gui_widgets.c` | `unity` | 0 |
| `tests/unit/hal_component/test_hal_pc.c` | `unity` | 0 |
| `tests/unit/ipc/test_ipc_broker.c` | `unity` | 12 |
| `tests/unit/ipc/test_ipc_event_group.c` | `unity` | 0 | IPC event-group set/clear/get/wait guard and timeout contracts with fake OS ticks. |
| `tests/unit/ipc/test_ipc_mq.c` | `unity` | 0 | IPC message queue init/FIFO/full/overwrite/timeout/stat contracts |
| `tests/unit/ipc/test_ipc_observer.c` | `unity` | 0 | IPC observer/subject init, attach, notify, detach, capacity, and clear contracts. |
| `tests/unit/ipc/test_ipc_pipe.c` | `unity` | 0 |
| `tests/unit/kernel/test_autotask.c` | `unity` | 0 | Kernel misc AutoTask init/trigger/idle-time/mutator public contracts with fake OS ticks. |
| `tests/unit/kernel/test_bootreason_check.c` | `unity` | 0 |
| `tests/unit/kernel/test_osal.c` | `unity` | 2 |
| `tests/unit/mux/test_mux_core.c` | `unity` | 19 |
| `tests/unit/mux/test_mux_gpio.c` | `unity` | 35 |
| `tests/unit/mux/test_mux_i2c.c` | `unity` | 37 |
| `tests/unit/mux/test_mux_spi.c` | `unity` | 12 |
| `tests/unit/mux/test_mux_uart.c` | `unity` | 13 |
| `tests/unit/net/test_at_client_core.c` | `unity` | 23 |
| `tests/unit/net/test_at_server_core.c` | `unity` | 10 |
| `tests/unit/net/test_can.c` | `unity` | 0 |
| `tests/unit/net/test_can_public_header.c` | `unity` | 0 | CAN public include-root/self-contained API smoke |
| `tests/unit/net/test_net_feature_gated_umbrella.c` | `unity` | 0 | Net umbrella exports CAN/LTE only under explicit feature opt-in macros. |
| `tests/unit/net/test_iso7816.c` | `unity` | 0 |
| `tests/unit/net/test_iso7816_example_main.c` | `unity` | 0 |
| `tests/unit/net/test_lte.c` | `unity` | 0 |
| `tests/unit/net/test_lte_public_header.c` | `unity` | 0 | LTE public include-root/self-contained API smoke |
| `tests/unit/net/test_lte_uart_adapter.c` | `unity` | 0 | LTE callback-backed UART adapter transport seam |
| `tests/unit/net/test_lte_hal_uart_adapter.c` | `unity` | 0 | LTE default-off HAL UART adapter transport seam with host HAL fakes. |
| `tests/unit/net/test_lte_hal_uart_smoke_example.c` | `unity` | 0 | Build-guarded LTE HAL UART smoke skeleton; not a hardware-validation record. |
| `tests/unit/net/test_modbus_legacy.c` | `unity` | 3 |
| `tests/unit/net/test_mqtt_client_core.c` | `unity` | 4 | MQTT client CONNECT, publish/subscribe/unsubscribe callbacks, SUBACK failure, and keep-alive transport failure contracts |
| `tests/unit/net/test_nano_modbus.c` | `unity` | 0 |
| `tests/unit/net/test_net_core.c` | `unity` | 0 |
| `tests/unit/net/test_net_smbus_pmbus.c` | `unity` | 34 |
| `tests/unit/pid/test_pid_auto.c` | `unity` | 12 |
| `tests/unit/pid/test_pid_core.c` | `unity` | 5 |
| `tests/unit/pid/test_pid_example_log_stub.c` | `plain/helper` | 0 | Shared no-op log shim linked into PID example smoke targets; not a standalone CTest source. |
| `tests/unit/pm/test_charger_bq25620.c` | `unity` | 10 |
| `tests/unit/pm/test_pm_core.c` | `unity` | 0 |
| `tests/unit/pm/test_pm_platform_fallback.c` | `unity` | 0 |
| `tests/unit/sensor/test_sensor_framework.c` | `unity` | 19 |
| `tests/unit/sensor/test_mlx90614.c` | `unity` | 0 |
| `tests/unit/sensor/test_aht20.c` | `unity` | 0 |
| `tests/unit/sensor/test_sht40.c` | `unity` | 0 |
| `tests/unit/sensor/test_oled_ssd1306.c` | `unity` | 0 |
| `tests/unit/sensor/test_bh1750.c` | `unity` | 0 |
| `tests/unit/sensor/test_bh1750_legacy_sensor.c` | `unity` | 0 | BH1750 legacy light sensor init/read/failure contracts |
| `tests/unit/sensor/test_sht30_legacy_sensor.c` | `unity` | 0 | SHT30 legacy compatibility wrapper delegates to canonical Device owner; dual-address/error/output-preservation contracts |
| `tests/unit/sensor/test_hdc1080.c` | `unity` | 0 |
| `tests/unit/sensor/test_tsl2561.c` | `unity` | 0 |
| `tests/unit/sensor/test_sgp40.c` | `unity` | 0 |
| `tests/unit/sensor/test_lps22hb.c` | `unity` | 0 |
| `tests/unit/sensor/test_power_monitors.c` | `unity` | 0 |
| `tests/unit/sensor/test_adc_power_monitors.c` | `unity` | 0 |
| `tests/unit/sensor/test_coulomb.c` | `unity` | 0 |
| `tests/unit/sensor/test_mpu6050.c` | `unity` | 0 |
| `tests/unit/sensor/test_bmi088.c` | `unity` | 0 |
| `tests/unit/sensor/test_bmi270.c` | `unity` | 0 |
| `tests/unit/sensor/test_w25qxx.c` | `unity` | 0 |
| `tests/unit/sensor/test_vl53l1x.c` | `unity` | 0 |
| `tests/unit/sensor/test_bno055.c` | `unity` | 0 |
| `tests/unit/sensor/test_dmp.c` | `unity` | 0 |
| `tests/unit/sensor/test_lsm9ds1.c` | `unity` | 0 |
| `tests/unit/sensor/test_lsm9ds1_device.c` | `unity` | 0 | LSM9DS1 Device-model fixed-address dual-I2C lifecycle, identity, staged 9-axis sample and failure-preservation contracts |
| `tests/unit/sensor/test_proximity_sensors.c` | `unity` | 0 |
| `tests/unit/sensor/test_vcnl4040_device.c` | `unity` | 0 | VCNL4040 Device-model fixed-address, nested lifecycle, little-endian sample and failure-preservation contracts |

| `tests/unit/sensor/test_magnetic_i2c_sensors.c` | `unity` | 0 | QMC5883L + IST8310 + AK09918 + CMM905 legacy magnetic I2C contracts |
| `tests/unit/sensor/test_ak09918_device.c` | `unity` | 0 | AK09918 canonical Device identity, reset/config, sample, failure-preservation and lifecycle contracts |

| `tests/unit/sensor/test_angle_encoders.c` | `unity` | 0 | AS5600/AS5048 legacy angle encoder create/read/guard/failure contracts |
| `tests/unit/sensor/test_as5600_device.c` | `unity` | 0 | AS5600 canonical Device lifecycle, angle decoding, transport guards and failed re-init atomicity |
| `tests/unit/sensor/test_as5048b_device.c` | `unity` | 0 | AS5048B canonical Device lifecycle, angle decoding, transport guards and failed re-init atomicity |
| `tests/unit/sensor/test_light_uv_sensors.c` | `unity` | 0 | MAX44009/GUVA-S12SD legacy light/UV read/failure/boundary contracts |
| `tests/unit/sensor/test_mq_gas_sensors.c` | `unity` | 0 | MQ3/MQ7/MQ135 ADC gas conversion and guard contracts |
| `tests/unit/sensor/test_apds9960_sensor.c` | `unity` | 0 | APDS9960 RGB init/read/failure guard contracts |
| `tests/unit/sensor/test_proximity_sensors.c` | `unity` | 0 |
| `tests/unit/sensor/test_ap3216c_sensor.c` | `unity` | 0 | AP3216C light/proximity/IR init/read/failure guard contracts |
| `tests/unit/sensor/test_ccs811_sensor.c` | `unity` | 0 | CCS811 eCO2/TVOC init/read/failure guard contracts |
| `tests/unit/sensor/test_vl53l0x_sensor.c` | `unity` | 0 | VL53L0X legacy ToF identity/init/read/failure contracts |
| `tests/unit/sensor/test_vl53l0x_device.c` | `unity` | 0 | VL53L0X canonical Device identity/read/lifecycle/failure contracts |
| `tests/unit/sensor/test_analog_misc_sensors.c` | `unity` | 0 | ACS712/FSR/MG811 legacy ADC analog conversion and guard contracts |

| `tests/unit/sensor/test_env_i2c_sensors.c` | `unity` | 0 | AHT10 legacy env I2C init/read/failure contracts |
| `tests/unit/sensor/test_bmp280_sensor.c` | `unity` | 0 | BMP280 legacy `sensor_bmp280.c` pressure/temperature calibration/read/failure contracts |
| `tests/unit/sensor/test_icm20608_sensor.c` | `unity` | 0 | ICM20608 legacy IMU I2C/SPI identity/init/read/failure contracts |
| `tests/unit/sensor/test_gps.c` | `unity` | 0 | GPS legacy parser GGA/RMC/GSV/checksum/registry contracts |
| `tests/unit/sensor/test_low_power_accel_sensors.c` | `unity` | 0 | BMA400/KX023/ADXL362/LIS2DW12 legacy low-power accel init/read/config failure contracts |
| `tests/unit/sensor/test_st_silan_accel_sensors.c` | `unity` | 0 | LIS2DH12/LIS2DW12/SC7A20/Silan SC7A20 accel init/read/config failure contracts |
| `tests/unit/sensor/test_lsm6_imu_sensors.c` | `unity` | 0 | LSM6DSL/LSM6DSO/LSM6DSR legacy IMU I2C/SPI config/read/failure contracts |
| `tests/unit/sensor/test_sensors_multi.c` | `unity` | 12 |
| `tests/unit/sensor/test_i2c2_sensor_devices.c` | `unity` | 0 | AHT30/L3G4200D/BME680/SC7A22H Device initialization and sample contracts |
| `tests/unit/sensor/test_sht30_integration.c` | `unity` | 11 |

| `tests/unit/storage/test_storage_eeprom_24xx.c` | `unity` | 67 |
| `tests/unit/support/test_clib_alloc_shim.c` | `unity` | 0 |
| `tests/unit/sys/test_sys_core.c` | `unity` | 0 | SYS weak/default reset and identity/version queries fail closed without board ownership. |
| `tests/unit/sys/test_sys_timer_sm.c` | `unity` | 0 |
| `tests/unit/trace/test_trace_core.c` | `unity` | 0 |
