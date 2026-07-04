# Unit Test Inventory

> Generated from the current `tests/unit` tree as the baseline for the Unity + CTest + FFF refactor.

## Summary

- Total C unit-test files: 78
- Unity-style files: 28
- Raw `assert()` files: 42
- Mixed Unity + raw `assert()` files: 0
- Plain/compile-smoke files without obvious Unity/assert markers: 9

## Component Breakdown

| Component | Total | Unity | Raw assert | Mixed | Plain | Fake-heavy candidates |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `actuator` | 1 | 0 | 1 | 0 | 0 | 1 |
| `analog_devices` | 2 | 1 | 1 | 0 | 0 | 2 |
| `clib` | 2 | 1 | 1 | 0 | 0 | 0 |
| `crypto` | 7 | 1 | 6 | 0 | 0 | 1 |
| `device` | 7 | 3 | 0 | 0 | 4 | 2 |
| `display` | 5 | 1 | 4 | 0 | 0 | 1 |
| `dm` | 6 | 2 | 4 | 0 | 0 | 0 |
| `fota` | 2 | 1 | 1 | 0 | 0 | 1 |
| `fuel_gauge` | 5 | 4 | 1 | 0 | 0 | 4 |
| `gui` | 4 | 1 | 3 | 0 | 0 | 2 |
| `hal_component` | 5 | 5 | 0 | 0 | 0 | 0 |
| `ipc` | 3 | 1 | 2 | 0 | 0 | 1 |
| `kernel` | 1 | 0 | 0 | 0 | 1 | 0 |
| `net` | 13 | 1 | 10 | 0 | 1 | 8 |
| `pid` | 2 | 1 | 1 | 0 | 0 | 0 |
| `pm` | 4 | 1 | 3 | 0 | 0 | 1 |
| `sensor` | 4 | 2 | 0 | 0 | 2 | 1 |
| `storage` | 1 | 0 | 1 | 0 | 0 | 0 |
| `support` | 1 | 0 | 0 | 0 | 1 | 0 |
| `sys` | 1 | 0 | 1 | 0 | 0 | 1 |
| `trace` | 2 | 1 | 1 | 0 | 0 | 2 |

## Migration Priority

1. Keep all new tests on Unity assertions.
2. Convert raw `assert()` tests one component group at a time.
3. Use FFF first on fake-heavy targets where call count, argument capture, or return sequencing matters.
4. Keep simple local stubs for tiny deterministic dependencies.

## File Classification

| File | Style | Fake signal count |
| --- | --- | ---: |
| `tests/unit/actuator/test_actuator_framework.c` | `raw_assert` | 37 |
| `tests/unit/analog_devices/test_addc.c` | `unity` | 22 |
| `tests/unit/analog_devices/test_analog_devices.c` | `raw_assert` | 53 |
| `tests/unit/clib/test_clib_core.c` | `raw_assert` | 4 |
| `tests/unit/clib/test_xy_clib.c` | `unity` | 6 |
| `tests/unit/crypto/test_cipher_hmac.c` | `raw_assert` | 0 |
| `tests/unit/crypto/test_crc.c` | `raw_assert` | 2 |
| `tests/unit/crypto/test_crypto.c` | `unity` | 0 |
| `tests/unit/crypto/test_csprng.c` | `raw_assert` | 28 |
| `tests/unit/crypto/test_encode.c` | `raw_assert` | 0 |
| `tests/unit/crypto/test_hash.c` | `raw_assert` | 0 |
| `tests/unit/crypto/test_random.c` | `raw_assert` | 0 |
| `tests/unit/device/test_auto_register.c` | `plain` | 7 |
| `tests/unit/device/test_device.c` | `unity` | 0 |
| `tests/unit/device/test_device_async_helper.c` | `plain` | 56 |
| `tests/unit/device/test_device_bmp280.c` | `unity` | 0 |
| `tests/unit/device/test_device_framework.c` | `plain` | 6 |
| `tests/unit/device/test_device_mpu6050.c` | `unity` | 0 |
| `tests/unit/device/test_spi_device.c` | `plain` | 40 |
| `tests/unit/display/test_display_lcd.c` | `raw_assert` | 10 |
| `tests/unit/display/test_display_oled_ws2812.c` | `raw_assert` | 7 |
| `tests/unit/display/test_display_rgb_matrix.c` | `raw_assert` | 7 |
| `tests/unit/display/test_display_serial_rgb_headers.c` | `raw_assert` | 1 |
| `tests/unit/display/test_led_driver.c` | `unity` | 37 |
| `tests/unit/dm/test_dm.c` | `unity` | 2 |
| `tests/unit/dm/test_dm_base64.c` | `raw_assert` | 0 |
| `tests/unit/dm/test_dm_factory.c` | `raw_assert` | 0 |
| `tests/unit/dm/test_dm_nvm.c` | `unity` | 7 |
| `tests/unit/dm/test_dm_nvm_core.c` | `raw_assert` | 3 |
| `tests/unit/dm/test_dm_tlv.c` | `raw_assert` | 1 |
| `tests/unit/fota/test_fota.c` | `unity` | 5 |
| `tests/unit/fota/test_fota_core.c` | `raw_assert` | 63 |
| `tests/unit/fuel_gauge/test_fg_bq27z561.c` | `unity` | 22 |
| `tests/unit/fuel_gauge/test_fg_bq27z746.c` | `unity` | 21 |
| `tests/unit/fuel_gauge/test_fg_bq40z50.c` | `unity` | 55 |
| `tests/unit/fuel_gauge/test_fg_max17043.c` | `unity` | 15 |
| `tests/unit/fuel_gauge/test_fuel_gauge_core.c` | `raw_assert` | 39 |
| `tests/unit/gui/test_gui.c` | `unity` | 22 |
| `tests/unit/gui/test_gui_core.c` | `raw_assert` | 96 |
| `tests/unit/gui/test_gui_widget_theme.c` | `raw_assert` | 4 |
| `tests/unit/gui/test_gui_widgets.c` | `raw_assert` | 15 |
| `tests/unit/hal_component/test_hal.c` | `unity` | 0 |
| `tests/unit/hal_component/test_hal_gpio.c` | `unity` | 13 |
| `tests/unit/hal_component/test_hal_i2c.c` | `unity` | 4 |
| `tests/unit/hal_component/test_hal_pc.c` | `unity` | 12 |
| `tests/unit/hal_component/test_hal_uart.c` | `unity` | 11 |
| `tests/unit/ipc/test_ipc.c` | `unity` | 9 |
| `tests/unit/ipc/test_ipc_broker.c` | `raw_assert` | 23 |
| `tests/unit/ipc/test_ipc_pipe.c` | `raw_assert` | 0 |
| `tests/unit/kernel/test_osal.c` | `plain` | 17 |
| `tests/unit/net/test_at_client_core.c` | `raw_assert` | 52 |
| `tests/unit/net/test_at_server_core.c` | `raw_assert` | 43 |
| `tests/unit/net/test_can.c` | `raw_assert` | 49 |
| `tests/unit/net/test_iso7816.c` | `raw_assert` | 56 |
| `tests/unit/net/test_iso7816_example_main.c` | `plain` | 6 |
| `tests/unit/net/test_lte.c` | `raw_assert` | 16 |
| `tests/unit/net/test_modbus_legacy.c` | `raw_assert` | 76 |
| `tests/unit/net/test_mqtt_client_core.c` | `raw_assert` | 13 |
| `tests/unit/net/test_nano_modbus.c` | `raw_assert` | 55 |
| `tests/unit/net/test_net.c` | `unity` | 12 |
| `tests/unit/net/test_net_core.c` | `raw_assert` | 1 |
| `tests/unit/net/test_net_mqtt.c` | `unity` | 0 |
| `tests/unit/net/test_net_smbus_pmbus.c` | `raw_assert` | 47 |
| `tests/unit/pid/test_pid.c` | `unity` | 0 |
| `tests/unit/pid/test_pid_core.c` | `raw_assert` | 14 |
| `tests/unit/pm/test_charger_bq25620.c` | `raw_assert` | 56 |
| `tests/unit/pm/test_pm.c` | `unity` | 18 |
| `tests/unit/pm/test_pm_core.c` | `raw_assert` | 7 |
| `tests/unit/pm/test_pm_platform_fallback.c` | `raw_assert` | 0 |
| `tests/unit/sensor/test_sensor.c` | `unity` | 9 |
| `tests/unit/sensor/test_sensor_framework.c` | `unity` | 94 |
| `tests/unit/sensor/test_sensors_multi.c` | `plain` | 12 |
| `tests/unit/sensor/test_sht30_integration.c` | `plain` | 12 |
| `tests/unit/storage/test_storage_eeprom_24xx.c` | `raw_assert` | 9 |
| `tests/unit/support/test_clib_alloc_shim.c` | `plain` | 0 |
| `tests/unit/sys/test_sys_timer_sm.c` | `raw_assert` | 74 |
| `tests/unit/trace/test_trace.c` | `unity` | 49 |
| `tests/unit/trace/test_trace_core.c` | `raw_assert` | 59 |
