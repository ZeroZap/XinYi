# Unit Test Inventory

> Generated from the current `tests/unit` tree as the baseline for the Unity + CTest + FFF refactor.

## Summary

- Total C unit-test files: 76
- Unity-style files: 76
- Raw `assert()` files: 0
- Mixed Unity + raw `assert()` files: 0
- Plain/compile-smoke files without obvious Unity/assert markers: 0
- Registered CTest entries in `tests/unit/CMakeLists.txt`: 78
- Unwired source `.c` files under `tests/unit`: 0
- Inventory scope excludes build-generated files such as `tests/unit/build/**`.
- First-party-looking test files outside `tests/unit` are triaged separately so vendor tests and
  production self-test implementations are not mistaken for missing Unity migrations.

## Outside-`tests/unit` Triage

| Path | Classification | Action |
| --- | --- | --- |
| `components/crypto/curve25519-cortexm0-20150813/test/*.c` | Upstream/vendor Curve25519 Cortex-M0 test helpers | Keep in place; excluded from the project Unity/CTest unit inventory. |
| `components/sensor/sensor_self_test.c` | Production Sensor self-test implementation | Keep in component source; covered through `tests/unit/sensor/test_sensor_framework.c`. |
| `projects/Bank/ontroller-charger-test.c` | Imported project-local patch transcript, not an active CTest source | Leave out of unit conversion; treat as project-specific legacy material unless the Bank project is intentionally rehabilitated. |

## Component Breakdown

| Component | Total | Unity | Raw assert | Mixed | Plain | Fake-heavy candidates |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `actuator` | 1 | 1 | 0 | 0 | 0 | 1 |
| `analog_devices` | 1 | 1 | 0 | 0 | 0 | 1 |
| `clib` | 1 | 1 | 0 | 0 | 0 | 0 |
| `crypto` | 10 | 10 | 0 | 0 | 0 | 2 |
| `device` | 4 | 4 | 0 | 0 | 0 | 3 |
| `display` | 5 | 5 | 0 | 0 | 0 | 2 |
| `dm` | 6 | 6 | 0 | 0 | 0 | 2 |
| `fota` | 1 | 1 | 0 | 0 | 0 | 1 |
| `framework` | 1 | 1 | 0 | 0 | 0 | 1 |
| `fuel_gauge` | 5 | 5 | 0 | 0 | 0 | 5 |
| `gui` | 3 | 3 | 0 | 0 | 0 | 1 |
| `hal_component` | 1 | 1 | 0 | 0 | 0 | 0 |
| `ipc` | 2 | 2 | 0 | 0 | 0 | 1 |
| `kernel` | 2 | 2 | 0 | 0 | 0 | 1 |
| `mux` | 5 | 5 | 0 | 0 | 0 | 5 |
| `net` | 11 | 11 | 0 | 0 | 0 | 5 |
| `pid` | 1 | 1 | 0 | 0 | 0 | 0 |
| `pm` | 3 | 3 | 0 | 0 | 0 | 1 |
| `sensor` | 9 | 9 | 0 | 0 | 0 | 9 |
| `storage` | 1 | 1 | 0 | 0 | 0 | 1 |
| `support` | 1 | 1 | 0 | 0 | 0 | 0 |
| `sys` | 1 | 1 | 0 | 0 | 0 | 0 |
| `trace` | 1 | 1 | 0 | 0 | 0 | 0 |

## Migration Priority

1. Keep all new tests on Unity assertions.
2. Keep the raw `assert()` and unwired-source inventories at zero for tracked `tests/unit` sources.
3. Use FFF first on fake-heavy targets where call count, argument capture, or return sequencing matters.
4. Keep simple local stubs for tiny deterministic dependencies.
5. Re-triage first-party-looking files outside `tests/unit` before treating them as migration work.

## File Classification

| File | Style | Fake signal count |
| --- | --- | ---: |
| `tests/unit/actuator/test_actuator_framework.c` | `unity` | 15 |
| `tests/unit/analog_devices/test_analog_devices.c` | `unity` | 11 |
| `tests/unit/clib/test_clib_core.c` | `unity` | 0 |
| `tests/unit/crypto/test_25519.c` | `unity` | 0 |
| `tests/unit/crypto/test_25519_m0.c` | `unity` | 6 |
| `tests/unit/crypto/test_cipher_hmac.c` | `unity` | 0 |
| `tests/unit/crypto/test_crc.c` | `unity` | 2 |
| `tests/unit/crypto/test_csprng.c` | `unity` | 0 |
| `tests/unit/crypto/test_encode.c` | `unity` | 0 |
| `tests/unit/crypto/test_hash.c` | `unity` | 0 |
| `tests/unit/crypto/test_lwc.c` | `unity` | 0 |
| `tests/unit/crypto/test_random.c` | `unity` | 0 |
| `tests/unit/crypto/test_sm2.c` | `unity` | 0 |
| `tests/unit/device/test_auto_register.c` | `unity` | 0 |
| `tests/unit/device/test_device_async_helper.c` | `unity` | 12 |
| `tests/unit/device/test_device_framework.c` | `unity` | 7 |
| `tests/unit/device/test_spi_device.c` | `unity` | 40 |
| `tests/unit/display/test_display_lcd.c` | `unity` | 0 |
| `tests/unit/display/test_display_oled_ws2812.c` | `unity` | 0 |
| `tests/unit/display/test_display_rgb_matrix.c` | `unity` | 4 |
| `tests/unit/display/test_display_serial_rgb_headers.c` | `unity` | 0 |
| `tests/unit/display/test_led_driver.c` | `unity` | 12 |
| `tests/unit/dm/test_dm_base64.c` | `unity` | 0 |
| `tests/unit/dm/test_dm_corejson.c` | `unity` | 0 |
| `tests/unit/dm/test_dm_factory.c` | `unity` | 1 |
| `tests/unit/dm/test_dm_fee.c` | `unity` | 12 |
| `tests/unit/dm/test_dm_nvm_core.c` | `unity` | 0 |
| `tests/unit/dm/test_dm_tlv.c` | `unity` | 0 |
| `tests/unit/fota/test_fota_core.c` | `unity` | 15 |
| `tests/unit/framework/test_fff_smoke.c` | `unity` | 72 |
| `tests/unit/fuel_gauge/test_fg_bq27z561.c` | `unity` | 40 |
| `tests/unit/fuel_gauge/test_fg_bq27z746.c` | `unity` | 36 |
| `tests/unit/fuel_gauge/test_fg_bq40z50.c` | `unity` | 104 |
| `tests/unit/fuel_gauge/test_fg_max17043.c` | `unity` | 27 |
| `tests/unit/fuel_gauge/test_fuel_gauge_core.c` | `unity` | 62 |
| `tests/unit/gui/test_gui_core.c` | `unity` | 22 |
| `tests/unit/gui/test_gui_widget_theme.c` | `unity` | 0 |
| `tests/unit/gui/test_gui_widgets.c` | `unity` | 0 |
| `tests/unit/hal_component/test_hal_pc.c` | `unity` | 0 |
| `tests/unit/ipc/test_ipc_broker.c` | `unity` | 12 |
| `tests/unit/ipc/test_ipc_pipe.c` | `unity` | 0 |
| `tests/unit/kernel/test_bootreason_check.c` | `unity` | 0 |
| `tests/unit/kernel/test_osal.c` | `unity` | 2 |
| `tests/unit/mux/test_mux_core.c` | `unity` | 18 |
| `tests/unit/mux/test_mux_gpio.c` | `unity` | 35 |
| `tests/unit/mux/test_mux_i2c.c` | `unity` | 37 |
| `tests/unit/mux/test_mux_spi.c` | `unity` | 12 |
| `tests/unit/mux/test_mux_uart.c` | `unity` | 12 |
| `tests/unit/net/test_at_client_core.c` | `unity` | 23 |
| `tests/unit/net/test_at_server_core.c` | `unity` | 10 |
| `tests/unit/net/test_can.c` | `unity` | 0 |
| `tests/unit/net/test_iso7816.c` | `unity` | 0 |
| `tests/unit/net/test_iso7816_example_main.c` | `unity` | 0 |
| `tests/unit/net/test_lte.c` | `unity` | 0 |
| `tests/unit/net/test_modbus_legacy.c` | `unity` | 3 |
| `tests/unit/net/test_mqtt_client_core.c` | `unity` | 4 |
| `tests/unit/net/test_nano_modbus.c` | `unity` | 0 |
| `tests/unit/net/test_net_core.c` | `unity` | 0 |
| `tests/unit/net/test_net_smbus_pmbus.c` | `unity` | 34 |
| `tests/unit/pid/test_pid_core.c` | `unity` | 0 |
| `tests/unit/pm/test_charger_bq25620.c` | `unity` | 10 |
| `tests/unit/pm/test_pm_core.c` | `unity` | 0 |
| `tests/unit/pm/test_pm_platform_fallback.c` | `unity` | 0 |
| `tests/unit/sensor/test_sensor_framework.c` | `unity` | 19 |
| `tests/unit/sensor/test_mlx90614.c` | `unity` | 0 |
| `tests/unit/sensor/test_aht20.c` | `unity` | 0 |
| `tests/unit/sensor/test_sht40.c` | `unity` | 0 |
| `tests/unit/sensor/test_bh1750.c` | `unity` | 0 |
| `tests/unit/sensor/test_hdc1080.c` | `unity` | 0 |
| `tests/unit/sensor/test_tsl2561.c` | `unity` | 0 |
| `tests/unit/sensor/test_sensors_multi.c` | `unity` | 12 |
| `tests/unit/sensor/test_sht30_integration.c` | `unity` | 11 |
| `tests/unit/storage/test_storage_eeprom_24xx.c` | `unity` | 67 |
| `tests/unit/support/test_clib_alloc_shim.c` | `unity` | 0 |
| `tests/unit/sys/test_sys_timer_sm.c` | `unity` | 0 |
| `tests/unit/trace/test_trace_core.c` | `unity` | 0 |
