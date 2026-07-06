# Unit Test Inventory

> Generated from the current `tests/unit` tree as the baseline for the Unity + CTest + FFF refactor.

## Summary

- Total C unit-test files: 69
- Unity-style files: 68
- Raw `assert()` files: 0
- Mixed Unity + raw `assert()` files: 0
- Plain/compile-smoke files without obvious Unity/assert markers: 1

## Component Breakdown

| Component | Total | Unity | Raw assert | Mixed | Plain | Fake-heavy candidates |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `actuator` | 1 | 1 | 0 | 0 | 0 | 1 |
| `analog_devices` | 1 | 1 | 0 | 0 | 0 | 1 |
| `clib` | 1 | 1 | 0 | 0 | 0 | 0 |
| `crypto` | 9 | 9 | 0 | 0 | 0 | 0 |
| `device` | 4 | 4 | 0 | 0 | 0 | 3 |
| `display` | 5 | 5 | 0 | 0 | 0 | 4 |
| `dm` | 6 | 6 | 0 | 0 | 0 | 1 |
| `fota` | 1 | 1 | 0 | 0 | 0 | 1 |
| `framework` | 1 | 1 | 0 | 0 | 0 | 1 |
| `fuel_gauge` | 5 | 5 | 0 | 0 | 0 | 5 |
| `gui` | 3 | 3 | 0 | 0 | 0 | 2 |
| `hal_component` | 1 | 1 | 0 | 0 | 0 | 1 |
| `ipc` | 2 | 2 | 0 | 0 | 0 | 1 |
| `kernel` | 2 | 2 | 0 | 0 | 0 | 1 |
| `mux` | 5 | 5 | 0 | 0 | 0 | 5 |
| `net` | 11 | 11 | 0 | 0 | 0 | 7 |
| `pid` | 1 | 1 | 0 | 0 | 0 | 0 |
| `pm` | 3 | 3 | 0 | 0 | 0 | 1 |
| `sensor` | 3 | 3 | 0 | 0 | 0 | 3 |
| `storage` | 1 | 1 | 0 | 0 | 0 | 1 |
| `support` | 1 | 0 | 0 | 0 | 1 | 0 |
| `sys` | 1 | 1 | 0 | 0 | 0 | 1 |
| `trace` | 1 | 1 | 0 | 0 | 0 | 0 |

## Migration Priority

1. Keep all new tests on Unity assertions.
2. Convert raw `assert()` tests one component group at a time.
3. Use FFF first on fake-heavy targets where call count, argument capture, or return sequencing matters.
4. Keep simple local stubs for tiny deterministic dependencies.

## File Classification

| File | Style | Fake signal count |
| --- | --- | ---: |
| `tests/unit/actuator/test_actuator_framework.c` | `unity` | 15 |
| `tests/unit/analog_devices/test_analog_devices.c` | `unity` | 7 |
| `tests/unit/clib/test_clib_core.c` | `unity` | 0 |
| `tests/unit/crypto/test_cipher_hmac.c` | `unity` | 0 |
| `tests/unit/crypto/test_crc.c` | `unity` | 0 |
| `tests/unit/crypto/test_csprng.c` | `unity` | 0 |
| `tests/unit/crypto/test_encode.c` | `unity` | 0 |
| `tests/unit/crypto/test_hash.c` | `unity` | 0 |
| `tests/unit/crypto/test_25519.c` | `unity` | 0 |
| `tests/unit/crypto/test_25519_m0.c` | `unity` | 0 |
| `tests/unit/crypto/test_random.c` | `unity` | 0 |
| `tests/unit/crypto/test_sm2.c` | `unity` | 0 |
| `tests/unit/device/test_auto_register.c` | `unity` | 0 |
| `tests/unit/device/test_device_async_helper.c` | `unity` | 32 |
| `tests/unit/device/test_device_framework.c` | `unity` | 4 |
| `tests/unit/device/test_spi_device.c` | `unity` | 20 |
| `tests/unit/display/test_display_lcd.c` | `unity` | 0 |
| `tests/unit/display/test_display_oled_ws2812.c` | `unity` | 3 |
| `tests/unit/display/test_display_rgb_matrix.c` | `unity` | 3 |
| `tests/unit/display/test_display_serial_rgb_headers.c` | `unity` | 1 |
| `tests/unit/display/test_led_driver.c` | `unity` | 11 |
| `tests/unit/dm/test_dm_base64.c` | `unity` | 0 |
| `tests/unit/dm/test_dm_corejson.c` | `unity` | 0 |
| `tests/unit/dm/test_dm_factory.c` | `unity` | 0 |
| `tests/unit/dm/test_dm_fee.c` | `unity` | 6 |
| `tests/unit/dm/test_dm_nvm_core.c` | `unity` | 0 |
| `tests/unit/dm/test_dm_tlv.c` | `unity` | 0 |
| `tests/unit/fota/test_fota_core.c` | `unity` | 17 |
| `tests/unit/framework/test_fff_smoke.c` | `unity` | 38 |
| `tests/unit/fuel_gauge/test_fg_bq27z561.c` | `unity` | 20 |
| `tests/unit/fuel_gauge/test_fg_bq27z746.c` | `unity` | 18 |
| `tests/unit/fuel_gauge/test_fg_bq40z50.c` | `unity` | 52 |
| `tests/unit/fuel_gauge/test_fg_max17043.c` | `unity` | 13 |
| `tests/unit/fuel_gauge/test_fuel_gauge_core.c` | `unity` | 31 |
| `tests/unit/gui/test_gui_core.c` | `unity` | 24 |
| `tests/unit/gui/test_gui_widget_theme.c` | `unity` | 0 |
| `tests/unit/gui/test_gui_widgets.c` | `unity` | 4 |
| `tests/unit/hal_component/test_hal_pc.c` | `unity` | 2 |
| `tests/unit/ipc/test_ipc_broker.c` | `unity` | 17 |
| `tests/unit/ipc/test_ipc_pipe.c` | `unity` | 0 |
| `tests/unit/kernel/test_bootreason_check.c` | `unity` | 0 |
| `tests/unit/kernel/test_osal.c` | `unity` | 5 |
| `tests/unit/mux/test_mux_core.c` | `unity` | 18 |
| `tests/unit/mux/test_mux_gpio.c` | `unity` | 35 |
| `tests/unit/mux/test_mux_i2c.c` | `unity` | 37 |
| `tests/unit/mux/test_mux_spi.c` | `unity` | 12 |
| `tests/unit/mux/test_mux_uart.c` | `unity` | 12 |
| `tests/unit/net/test_at_client_core.c` | `unity` | 25 |
| `tests/unit/net/test_at_server_core.c` | `unity` | 10 |
| `tests/unit/net/test_can.c` | `unity` | 17 |
| `tests/unit/net/test_iso7816.c` | `unity` | 0 |
| `tests/unit/net/test_iso7816_example_main.c` | `unity` | 0 |
| `tests/unit/net/test_lte.c` | `unity` | 7 |
| `tests/unit/net/test_modbus_legacy.c` | `unity` | 2 |
| `tests/unit/net/test_mqtt_client_core.c` | `unity` | 4 |
| `tests/unit/net/test_nano_modbus.c` | `unity` | 0 |
| `tests/unit/net/test_net_core.c` | `unity` | 0 |
| `tests/unit/net/test_net_smbus_pmbus.c` | `unity` | 18 |
| `tests/unit/pid/test_pid_core.c` | `unity` | 0 |
| `tests/unit/pm/test_charger_bq25620.c` | `unity` | 5 |
| `tests/unit/pm/test_pm_core.c` | `unity` | 0 |
| `tests/unit/pm/test_pm_platform_fallback.c` | `unity` | 0 |
| `tests/unit/sensor/test_sensor_framework.c` | `unity` | 28 |
| `tests/unit/sensor/test_sensors_multi.c` | `unity` | 6 |
| `tests/unit/sensor/test_sht30_integration.c` | `unity` | 6 |
| `tests/unit/storage/test_storage_eeprom_24xx.c` | `unity` | 58 |
| `tests/unit/support/test_clib_alloc_shim.c` | `plain` | 0 |
| `tests/unit/sys/test_sys_timer_sm.c` | `unity` | 18 |
| `tests/unit/trace/test_trace_core.c` | `unity` | 0 |
