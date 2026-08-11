cmake_minimum_required(VERSION 3.10)
project(xy_unit_tests C)
set(CMAKE_C_STANDARD 99)

# PC simulation mode
add_definitions(-DHAL_PLATFORM_PC -DXY_HAL_PC_SIM)
add_compile_options(-Wall -Wextra -Wno-unused-parameter -g -O0)

option(XY_ENABLE_UNIT_COVERAGE "Enable gcov coverage instrumentation for unit tests" OFF)
if(XY_ENABLE_UNIT_COVERAGE)
    if(NOT CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        message(FATAL_ERROR "XY_ENABLE_UNIT_COVERAGE requires GCC or Clang")
    endif()

    add_compile_options(--coverage -O0 -g)
    add_link_options(--coverage)
endif()

# Path roots
set(ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../..)
get_filename_component(ROOT ${ROOT} ABSOLUTE)
set(DEV  ${ROOT}/components/device)
set(HAL  ${ROOT}/components/hal)
set(DRV  ${ROOT}/components/drivers)
set(NET  ${ROOT}/components/net)
set(CRYPTO ${ROOT}/components/crypto)
set(SENSOR ${ROOT}/components/sensor)
set(PM ${ROOT}/components/pm)
set(TRACE ${ROOT}/components/trace)
set(CLIB ${ROOT}/components/clib/xy_clib)
set(OSAL ${ROOT}/components/kernel/osal)
set(MUX ${ROOT}/components/mux)
set(PID ${ROOT}/components/pid)
set(DM ${ROOT}/components/dm)
set(IPC ${ROOT}/components/ipc)
set(STORAGE ${ROOT}/components/drivers/storage)
set(DISPLAY ${ROOT}/components/drivers/display)
set(FOTA ${ROOT}/components/fota)
set(GUI ${ROOT}/components/gui)
set(ANALOG ${ROOT}/components/analog_devices)
set(SYS ${ROOT}/components/sys)
set(ACTUATOR ${ROOT}/components/actuator)
set(BOOTREASON ${ROOT}/components/kernel/service/bootreason_check)

set(UNIT_ACTUATOR ${CMAKE_CURRENT_SOURCE_DIR}/actuator)
set(UNIT_ANALOG ${CMAKE_CURRENT_SOURCE_DIR}/analog_devices)
set(UNIT_CLIB ${CMAKE_CURRENT_SOURCE_DIR}/clib)
set(UNIT_CRYPTO ${CMAKE_CURRENT_SOURCE_DIR}/crypto)
set(UNIT_DEVICE ${CMAKE_CURRENT_SOURCE_DIR}/device)
set(UNIT_DISPLAY ${CMAKE_CURRENT_SOURCE_DIR}/display)
set(UNIT_DM ${CMAKE_CURRENT_SOURCE_DIR}/dm)
set(UNIT_FOTA ${CMAKE_CURRENT_SOURCE_DIR}/fota)
set(UNIT_FRAMEWORK ${CMAKE_CURRENT_SOURCE_DIR}/framework)
set(UNIT_FUEL_GAUGE ${CMAKE_CURRENT_SOURCE_DIR}/fuel_gauge)
set(UNIT_GUI ${CMAKE_CURRENT_SOURCE_DIR}/gui)
set(UNIT_HAL ${CMAKE_CURRENT_SOURCE_DIR}/hal_component)
set(UNIT_IPC ${CMAKE_CURRENT_SOURCE_DIR}/ipc)
set(UNIT_KERNEL ${CMAKE_CURRENT_SOURCE_DIR}/kernel)
set(UNIT_NET ${CMAKE_CURRENT_SOURCE_DIR}/net)
set(UNIT_MUX ${CMAKE_CURRENT_SOURCE_DIR}/mux)
set(UNIT_PID ${CMAKE_CURRENT_SOURCE_DIR}/pid)
set(UNIT_PM ${CMAKE_CURRENT_SOURCE_DIR}/pm)
set(UNIT_SENSOR ${CMAKE_CURRENT_SOURCE_DIR}/sensor)
set(UNIT_STORAGE ${CMAKE_CURRENT_SOURCE_DIR}/storage)
set(UNIT_SUPPORT ${CMAKE_CURRENT_SOURCE_DIR}/support)
set(UNIT_SYS ${CMAKE_CURRENT_SOURCE_DIR}/sys)
set(UNIT_TRACE ${CMAKE_CURRENT_SOURCE_DIR}/trace)

include_directories(
    ${DEV}                          # xy_device_core.h
    ${DEV}/inc                      # xy_device.h
    ${HAL}/inc                      # xy_hal_*.h
    ${HAL}/PC                       # xy_hal_pc.h
    ${DRV}/sensor/temperature/sht30 # xy_sht30.h
    ${DRV}/sensor/motion/mpu6050    # xy_mpu6050.h
    ${DRV}/sensor/pressure/bmp280   # xy_bmp280.h
    ${DRV}/sensor/adc/ads1115       # xy_ads1115.h
    ${NET}/inc                      # xy_can.h
    ${NET}/modbus                   # legacy mb_slave.h
    ${NET}/src                      # xy_mqtt_client.h
    ${NET}/at/atc/Modules/cellular/common # at_client.h
    ${NET}/at/ats                  # xy_ats.h
    ${NET}/xy_iso7816               # xy_iso7816.h
    ${NET}/smbus                    # xy_smbus.h / xy_pmbus.h
    ${CRYPTO}/xy_crc                # xy_crc.h
    ${CRYPTO}/xy_rng                # xy_rng.h
    ${CRYPTO}/xy_chacha             # xy_chacha20_poly1305.h
    ${CRYPTO}/xy_sm3                # xy_sm3.h
    ${CRYPTO}/xy_sm4                # xy_sm4.h
    ${ROOT}/components/clib/xy_clib # xy_typedef.h
    ${CLIB}/inc                     # xy_clib aggregate header
    ${CLIB}                         # xy_clib public headers
    ${ROOT}/components/trace/xy_log/inc # xy_log.h
    ${TRACE}/xy_log/inc              # xy_log.h
    ${ROOT}/components/kernel/osal  # xy_os.h
    ${PM}/inc                       # xy_pm.h
    ${PM}/charger                   # xy_charger.h
    ${ROOT}/components/fuel_gauge/inc # xy_fuel_gauge.h
    ${PM}/fuel-gauge                # legacy xy_fuel_gauge.h path
    ${MUX}/inc                      # xy_mux*.h
    ${PID}                          # xy_pid.h
    ${PID}/inc                      # xy_pid_auto.h
    ${OSAL}/inc                     # xy_os_tick.h
    ${DM}/xy_base64                 # xy_base64.h
    ${IPC}/pipe                     # xy_pipe.h
    ${IPC}/xy_broker                # xy_broker.h
    ${STORAGE}/eeprom/24xx          # xy_eeprom_24xx.h
    ${DISPLAY}/lcd                  # xy_lcd*.h
    ${DISPLAY}/oled/ssd1306         # xy_oled_ssd1306.h
    ${DISPLAY}/led_drivers/serial_rgb # xy_ws2812.h
    ${DISPLAY}/led_drivers            # xy_led_driver.h
    ${FOTA}                         # xy_fota.h
    ${GUI}                          # xy_gui.h
    ${GUI}/inc                      # GUI widget headers
    ${ANALOG}                       # xy_adc.h
    ${ANALOG}/inc                   # xy_adc_ext.h
    ${SYS}/xy_timer                 # xy_timer.h
    ${SYS}/xy_state_machine         # xy_st.h
    ${ROOT}/components/kernel/misc  # xy_tick.h
    ${UNIT_KERNEL}                  # focused kernel test stubs
    ${BOOTREASON}/inc               # bootreason_check.h
    ${BOOTREASON}/src               # include implementation for focused test
    ${SENSOR}                       # sensor_core.h / sensor_type.h
    ${ACTUATOR}                     # xy_actuator.h
    ${ROOT}/tests/unity             # unity.h
    ${ROOT}/tests/fff               # fff.h
)

# Device framework + PC HAL sources shared by all tests
set(FRAMEWORK_SOURCES
    ${DEV}/src/xy_device.c          # lifecycle + register forwarding
    ${DEV}/src/xy_device_pm.c       # per-device PM ops
    ${DEV}/src/xy_device_bus_helpers.c # i2c/spi/uart/gpio helpers + register API
    ${DEV}/xy_device_core.c         # static-array registry + PM check
    ${HAL}/PC/xy_hal_pc.c
    ${HAL}/PC/xy_hal_gpio_pc.c
    ${HAL}/PC/xy_hal_i2c_pc.c
    ${HAL}/PC/xy_hal_spi_pc.c
    ${HAL}/PC/xy_hal_uart_pc.c
)

set(XY_UNIT_TEST_TARGETS)

function(xy_add_unit_test TARGET_NAME TEST_NAME)
    cmake_parse_arguments(XY_UNIT "UNITY" "" "" ${ARGN})

    set(XY_UNIT_SOURCES ${XY_UNIT_UNPARSED_ARGUMENTS})
    if(XY_UNIT_UNITY)
        list(APPEND XY_UNIT_SOURCES ${ROOT}/tests/unity/unity.c)
    endif()

    add_executable(${TARGET_NAME} ${XY_UNIT_SOURCES})
    add_test(NAME ${TEST_NAME} COMMAND ${TARGET_NAME})
    set_property(GLOBAL APPEND PROPERTY XY_UNIT_TEST_TARGETS ${TARGET_NAME})
endfunction()

# --- Test 1: framework end-to-end ---
xy_add_unit_test(test_device device_framework UNITY
    ${UNIT_DEVICE}/test_device_framework.c
    ${FRAMEWORK_SOURCES}
)

# --- Test 2: SHT30 driver integration ---
xy_add_unit_test(test_sht30_integration sht30_integration UNITY
    ${UNIT_SENSOR}/test_sht30_integration.c
    ${DRV}/sensor/temperature/sht30/xy_sht30.c
    ${FRAMEWORK_SOURCES}
)

# --- Test 3: heterogeneous sensor coexistence ---
xy_add_unit_test(test_sensors_multi sensors_multi UNITY
    ${UNIT_SENSOR}/test_sensors_multi.c
    ${DRV}/sensor/temperature/sht30/xy_sht30.c
    ${DRV}/sensor/motion/mpu6050/xy_mpu6050.c
    ${DRV}/sensor/pressure/bmp280/xy_bmp280.c
    ${DRV}/sensor/adc/ads1115/xy_ads1115.c
    ${FRAMEWORK_SOURCES}
)

# --- Test 4: SPI device helper + mixed-bus registry ---
xy_add_unit_test(test_spi_device spi_device UNITY
    ${UNIT_DEVICE}/test_spi_device.c
    ${FRAMEWORK_SOURCES}
)

# --- Test 6: XY_DEVICE_REGISTER auto-registration via GCC constructors ---
xy_add_unit_test(test_auto_register auto_register UNITY
    ${UNIT_DEVICE}/test_auto_register.c
    ${FRAMEWORK_SOURCES}
)

# --- Test 8: device registry example stays aligned with public APIs ---
xy_add_unit_test(test_device_registry_example device_registry_example
    ${ROOT}/examples/device_registry_example.c
    ${FRAMEWORK_SOURCES}
)

# --- Test 9: device driver template compiles and runs against public APIs ---
xy_add_unit_test(test_device_driver_template device_driver_template UNITY
    ${ROOT}/examples/device_driver_template.c
    ${FRAMEWORK_SOURCES}
)

# --- Test 10: CAN protocol component API + callback/fifo behavior ---
xy_add_unit_test(test_can can_component UNITY
    ${UNIT_NET}/test_can.c
    ${NET}/src/xy_can.c
    ${CLIB}/xy_stdio.c
)

# --- Test 10b: CAN public header include-root smoke ---
xy_add_unit_test(test_can_public_header can_public_header UNITY
    ${UNIT_NET}/test_can_public_header.c
)

target_include_directories(test_can_public_header PRIVATE
    ${CLIB}
    ${NET}/inc
    ${ROOT}/tests/unity
)

# --- Test 11: LTE component lifecycle, callback storage, and validation ---
xy_add_unit_test(test_lte lte_component UNITY
    ${UNIT_NET}/test_lte.c
    ${NET}/src/xy_lte.c
)

# --- Test 11b: LTE public header include-root smoke ---
xy_add_unit_test(test_lte_public_header lte_public_header UNITY
    ${UNIT_NET}/test_lte_public_header.c
)

target_include_directories(test_lte_public_header PRIVATE
    ${NET}/inc
    ${ROOT}/tests/unity
)

# --- Test 11c: LTE callback-backed UART adapter transport seam ---
xy_add_unit_test(test_lte_uart_adapter lte_uart_adapter UNITY
    ${UNIT_NET}/test_lte_uart_adapter.c
    ${NET}/src/xy_lte_uart_adapter.c
    ${NET}/src/xy_lte.c
)

# --- Test 11d: LTE HAL UART adapter transport seam ---
xy_add_unit_test(test_lte_hal_uart_adapter lte_hal_uart_adapter UNITY
    ${UNIT_NET}/test_lte_hal_uart_adapter.c
    ${NET}/src/xy_lte_hal_uart_adapter.c
    ${NET}/src/xy_lte.c
)

target_include_directories(test_lte_hal_uart_adapter PRIVATE
    ${HAL}/inc
)

# --- Test 11e: LTE HAL UART adapter smoke skeleton stays build-guarded ---
xy_add_unit_test(test_lte_hal_uart_smoke_example lte_hal_uart_smoke_example UNITY
    ${UNIT_NET}/test_lte_hal_uart_smoke_example.c
    ${NET}/src/xy_lte_hal_uart_adapter.c
    ${NET}/src/xy_lte.c
)

target_include_directories(test_lte_hal_uart_smoke_example PRIVATE
    ${HAL}/inc
)

# --- Test 12: nano_modbus compatibility API over mb_tiny ---
xy_add_unit_test(test_nano_modbus nano_modbus_compat UNITY
    ${UNIT_NET}/test_nano_modbus.c
    ${NET}/modbus_tiny/mb_tiny.c
)

# --- Test 12b: legacy Modbus RTU slave host guard ---
xy_add_unit_test(test_modbus_legacy modbus_legacy UNITY
    ${UNIT_NET}/test_modbus_legacy.c
    ${NET}/modbus/mb_slave.c
)

# --- Test 13: ISO7816 public include, lifecycle, and APDU behavior ---
xy_add_unit_test(test_iso7816 iso7816_component UNITY
    ${UNIT_NET}/test_iso7816.c
    ${NET}/xy_iso7816/xy_iso7816.c
)

# --- Test 14: ISO7816 usage example stays aligned with public APIs ---
xy_add_unit_test(test_iso7816_example iso7816_example UNITY
    ${NET}/xy_iso7816/xy_iso7816_example.c
    ${NET}/xy_iso7816/xy_iso7816.c
    ${UNIT_NET}/test_iso7816_example_main.c
)

# --- Test 15: Net umbrella/core lifecycle and platform helpers ---
xy_add_unit_test(test_net_core net_core UNITY
    ${UNIT_NET}/test_net_core.c
    ${NET}/src/xy_net.c
    ${NET}/src/xy_net_platform.c
    ${NET}/modbus_tiny/mb_tiny.c
    ${CLIB}/xy_stdio.c
)

target_include_directories(test_net_core PRIVATE
    ${NET}/modbus_tiny
    ${NET}/modbus_full
    ${NET}/at/atc/core
)

# --- Test 15b: Net umbrella exports CAN/LTE headers only when explicitly enabled ---
xy_add_unit_test(test_net_feature_gated_umbrella net_feature_gated_umbrella UNITY
    ${UNIT_NET}/test_net_feature_gated_umbrella.c
)
target_include_directories(test_net_feature_gated_umbrella PRIVATE
    ${NET}/modbus_tiny
    ${NET}/modbus_full
    ${NET}/at/atc/core
)
target_compile_definitions(test_net_feature_gated_umbrella PRIVATE
    XY_NET_ENABLE_CAN=1
    XY_NET_ENABLE_LTE=1
)

# --- Test 16: CRC core and common variants ---
xy_add_unit_test(test_crc crypto_crc UNITY
    ${UNIT_CRYPTO}/test_crc.c
    ${CRYPTO}/xy_crc/xy_crc.c
)

# --- Test 5: OSAL baremetal primitives (mutex/sem/event/queue/pool) ---
set(OSAL_ROOT ${ROOT}/components/kernel)

# --- Test 7: optional xy_device_async_*_ex helper API ---
xy_add_unit_test(test_device_async_helper device_async_helper UNITY
    ${UNIT_DEVICE}/test_device_async_helper.c
    ${DEV}/src/xy_device_async.c
)
target_include_directories(test_device_async_helper PRIVATE
    ${OSAL_ROOT}/osal
    ${OSAL_ROOT}/osal/inc
)

xy_add_unit_test(test_osal osal_baremetal UNITY
    ${UNIT_KERNEL}/test_osal.c
    ${OSAL_ROOT}/osal/backend/baremetal/xy_os_baremetal.c
    ${OSAL_ROOT}/osal/src/xy_os_tick.c
    ${OSAL_ROOT}/misc/xy_tick.c
    ${OSAL_ROOT}/misc/xy_timer_sw.c
)
target_include_directories(test_osal PRIVATE
    ${OSAL_ROOT}/osal
    ${OSAL_ROOT}/osal/inc
    ${OSAL_ROOT}/osal/backend/baremetal
    ${OSAL_ROOT}/misc
)
target_compile_definitions(test_osal PRIVATE
    XY_OS_BACKEND_BAREMETAL
    XY_OS_DISABLE_ASM    # use no-op IRQ stubs on x86
)

# --- Test 5b: kernel misc AutoTask scheduler public contracts ---
xy_add_unit_test(test_autotask kernel_autotask UNITY
    ${UNIT_KERNEL}/test_autotask.c
    ${OSAL_ROOT}/misc/src/xy_autotask.c
)
target_include_directories(test_autotask PRIVATE
    ${OSAL_ROOT}/misc/inc
    ${OSAL_ROOT}/osal
    ${OSAL_ROOT}/osal/inc
)

enable_testing()

# --- Test 16: Crypto CSPRNG lifecycle, buffering, and helpers ---
xy_add_unit_test(test_csprng crypto_csprng UNITY
    ${UNIT_CRYPTO}/test_csprng.c
    ${CRYPTO}/xy_rng/xy_csprng.c
    ${CLIB}/xy_stdio.c
)

# --- Test 17: Crypto simple RNG validation and output shape ---
xy_add_unit_test(test_random crypto_random UNITY
    ${UNIT_CRYPTO}/test_random.c
    ${CRYPTO}/xy_rng/xy_random.c
)

# --- Test 18: Crypto Base64 and Hex validation/roundtrip behavior ---
xy_add_unit_test(test_encode crypto_encode UNITY
    ${UNIT_CRYPTO}/test_encode.c
    ${CRYPTO}/xy_base/xy_base64.c
    ${CRYPTO}/xy_hex/xy_hex.c
)
target_include_directories(test_encode PRIVATE
    ${CRYPTO}
    ${CRYPTO}/inc
)

# --- Test 19: Crypto MD5/SHA-256 vectors and incremental hashing ---
xy_add_unit_test(test_hash crypto_hash UNITY
    ${UNIT_CRYPTO}/test_hash.c
    ${CRYPTO}/xy_md/xy_md5.c
    ${CRYPTO}/xy_hmac/xy_sha256.c
)
target_include_directories(test_hash PRIVATE
    ${CRYPTO}
    ${CRYPTO}/inc
)

# --- Test 20: Crypto AES/HMAC/SM3/SM4/ChaCha20 coverage ---
xy_add_unit_test(test_cipher_hmac crypto_cipher_hmac UNITY
    ${UNIT_CRYPTO}/test_cipher_hmac.c
    ${CRYPTO}/xy_aes/xy_aes.c
    ${CRYPTO}/xy_hmac/xy_hmac.c
    ${CRYPTO}/xy_md/xy_md5.c
    ${CRYPTO}/xy_hmac/xy_sha256.c
    ${CRYPTO}/xy_sm3/xy_sm3.c
    ${CRYPTO}/xy_sm4/xy_sm4.c
    ${CRYPTO}/xy_chacha/xy_chacha20_poly1305.c
)
target_include_directories(test_cipher_hmac PRIVATE
    ${CRYPTO}
    ${CRYPTO}/inc
    ${CRYPTO}/xy_chacha
    ${CRYPTO}/xy_sm3
    ${CRYPTO}/xy_sm4
)

# --- Test 20b: Crypto SM2 public API contracts ---
xy_add_unit_test(test_sm2 crypto_sm2 UNITY
    ${UNIT_CRYPTO}/test_sm2.c
    ${CRYPTO}/xy_sm2/xy_sm2.c
    ${CRYPTO}/xy_sm3/xy_sm3.c
    ${CRYPTO}/xy_sm4/xy_sm4.c
    ${CRYPTO}/xy_rng/xy_random.c
)
target_include_directories(test_sm2 PRIVATE
    ${CRYPTO}
    ${CRYPTO}/inc
    ${CRYPTO}/xy_sm2
    ${CRYPTO}/xy_sm3
    ${CRYPTO}/xy_sm4
)

# --- Test 20c: Crypto lightweight AEAD/hash contracts ---
xy_add_unit_test(test_lwc crypto_lwc UNITY
    ${UNIT_CRYPTO}/test_lwc.c
    ${CRYPTO}/xy_ascon/xy_ascon.c
    ${CRYPTO}/xy_tinyjambu/xy_tinyjambu.c
    ${CRYPTO}/xy_photon_beetle/xy_photon_beetle.c
)
target_include_directories(test_lwc PRIVATE
    ${CRYPTO}
    ${CRYPTO}/inc
    ${CRYPTO}/xy_ascon
    ${CRYPTO}/xy_tinyjambu
    ${CRYPTO}/xy_photon_beetle
)

# --- Support: host malloc-backed CLIB allocation shim contract ---
xy_add_unit_test(test_clib_alloc_shim clib_alloc_shim UNITY
    ${UNIT_SUPPORT}/test_clib_alloc_shim.c
)
target_compile_definitions(test_clib_alloc_shim PRIVATE TEST_CLIB_ALLOC_SHIM_MAIN)

# --- Test 20d: Crypto 25519 public API contracts ---
xy_add_unit_test(test_25519 crypto_25519 UNITY
    ${UNIT_CRYPTO}/test_25519.c
    ${CRYPTO}/xy_25519/xy_25519.c
    ${CLIB}/xy_string.c
    ${CLIB}/xy_ctype.c
    ${CLIB}/xy_stdio.c
    ${UNIT_SUPPORT}/test_clib_alloc_shim.c
)
target_include_directories(test_25519 PRIVATE
    ${CRYPTO}/xy_25519
)

# --- Test 20e: Crypto 25519 Cortex-M0 API and field smoke contracts ---
xy_add_unit_test(test_25519_m0 crypto_25519_m0 UNITY
    ${UNIT_CRYPTO}/test_25519_m0.c
    ${CRYPTO}/xy_25519/xy_25519_m0.c
    ${CRYPTO}/xy_25519/fe25519_m0.c
    ${CLIB}/xy_string.c
    ${CLIB}/xy_ctype.c
    ${CLIB}/xy_stdio.c
    ${UNIT_SUPPORT}/test_clib_alloc_shim.c
)
target_include_directories(test_25519_m0 PRIVATE
    ${CRYPTO}/xy_25519
    ${CLIB}
)

# --- Test 21: Sensor framework lifecycle, registry, FIFO, calibration, filters ---
xy_add_unit_test(test_sensor_framework sensor_framework UNITY
    ${UNIT_SENSOR}/test_sensor_framework.c
    ${SENSOR}/sensor_core.c
    ${SENSOR}/sensor_fifo.c
    ${SENSOR}/sensor_calibration.c
    ${SENSOR}/sensor_filter.c
    ${SENSOR}/sensor_threshold.c
    ${SENSOR}/sensor_self_test.c
)

xy_add_unit_test(test_mlx90614 sensor_mlx90614 UNITY
    ${UNIT_SENSOR}/test_mlx90614.c
    ${SENSOR}/src/xy_mlx90614.c
)
target_include_directories(test_mlx90614 PRIVATE
    ${SENSOR}/inc
)

xy_add_unit_test(test_aht20 sensor_aht20 UNITY
    ${UNIT_SENSOR}/test_aht20.c
    ${SENSOR}/src/xy_aht20.c
)
target_include_directories(test_aht20 PRIVATE
    ${SENSOR}/inc
)

xy_add_unit_test(test_sht40 sensor_sht40 UNITY
    ${UNIT_SENSOR}/test_sht40.c
    ${SENSOR}/src/xy_sht40.c
)
target_include_directories(test_sht40 PRIVATE
    ${SENSOR}/inc
)

xy_add_unit_test(test_sht30 sensor_sht30 UNITY
    ${UNIT_SENSOR}/test_sht30.c
    ${SENSOR}/src/xy_sht30.c
)
target_include_directories(test_sht30 BEFORE PRIVATE
    ${SENSOR}/inc
)

xy_add_unit_test(test_oled_ssd1306 sensor_oled_ssd1306 UNITY
    ${UNIT_SENSOR}/test_oled_ssd1306.c
    ${SENSOR}/src/xy_oled_ssd1306.c
)
target_include_directories(test_oled_ssd1306 BEFORE PRIVATE
    ${SENSOR}/inc
    ${OSAL}/inc
)

xy_add_unit_test(test_bq25620 sensor_bq25620 UNITY
    ${UNIT_SENSOR}/test_bq25620.c
    ${SENSOR}/src/xy_bq25620.c
)
target_include_directories(test_bq25620 BEFORE PRIVATE
    ${SENSOR}/inc
    ${HAL}/inc
    ${KERNEL}/osal/inc
)

xy_add_unit_test(test_bh1750 sensor_bh1750 UNITY
    ${UNIT_SENSOR}/test_bh1750.c
    ${SENSOR}/src/xy_bh1750.c
)
target_include_directories(test_bh1750 PRIVATE
    ${SENSOR}/inc
)

xy_add_unit_test(test_hdc1080 sensor_hdc1080 UNITY
    ${UNIT_SENSOR}/test_hdc1080.c
    ${SENSOR}/src/xy_hdc1080.c
)
target_include_directories(test_hdc1080 PRIVATE
    ${SENSOR}/inc
)

xy_add_unit_test(test_tsl2561 sensor_tsl2561 UNITY
    ${UNIT_SENSOR}/test_tsl2561.c
    ${SENSOR}/src/xy_tsl2561.c
)
target_include_directories(test_tsl2561 PRIVATE
    ${SENSOR}/inc
)

xy_add_unit_test(test_sgp40 sensor_sgp40 UNITY
    ${UNIT_SENSOR}/test_sgp40.c
    ${SENSOR}/src/xy_sgp40.c
)
target_include_directories(test_sgp40 PRIVATE
    ${SENSOR}/inc
)

xy_add_unit_test(test_lps22hb sensor_lps22hb UNITY
    ${UNIT_SENSOR}/test_lps22hb.c
    ${SENSOR}/src/xy_lps22hb.c
)
target_include_directories(test_lps22hb PRIVATE
    ${SENSOR}/inc
)
target_link_libraries(test_lps22hb PRIVATE m)

xy_add_unit_test(test_power_monitors sensor_power_monitors UNITY
    ${UNIT_SENSOR}/test_power_monitors.c
    ${SENSOR}/src/xy_max17043.c
    ${SENSOR}/src/xy_ina226.c
)
target_include_directories(test_power_monitors PRIVATE
    ${SENSOR}/inc
    ${HAL}/inc
    ${KERNEL}/osal
)

xy_add_unit_test(test_ina219 sensor_ina219 UNITY
    ${UNIT_SENSOR}/test_ina219.c
    ${SENSOR}/sensors/sensor_ina219.c
)
target_include_directories(test_ina219 BEFORE PRIVATE
    ${SENSOR}
    ${SENSOR}/sensors
)

xy_add_unit_test(test_angle_encoders sensor_angle_encoders UNITY
    ${UNIT_SENSOR}/test_angle_encoders.c
    ${SENSOR}/sensors/sensor_as5600.c
    ${SENSOR}/sensors/sensor_as5048.c
)
target_include_directories(test_angle_encoders BEFORE PRIVATE
    ${SENSOR}
    ${SENSOR}/sensors
)

xy_add_unit_test(test_light_uv_sensors sensor_light_uv_sensors UNITY
    ${UNIT_SENSOR}/test_light_uv_sensors.c
    ${SENSOR}/sensors/sensor_max44009.c
    ${SENSOR}/sensors/sensor_guvas12sd.c
)
target_include_directories(test_light_uv_sensors BEFORE PRIVATE
    ${SENSOR}
    ${SENSOR}/sensors
)
target_link_libraries(test_light_uv_sensors PRIVATE m)

xy_add_unit_test(test_bh1750_legacy_sensor sensor_bh1750_legacy UNITY
    ${UNIT_SENSOR}/test_bh1750_legacy_sensor.c
    ${SENSOR}/sensors/sensor_bh1750.c
)
target_include_directories(test_bh1750_legacy_sensor BEFORE PRIVATE
    ${SENSOR}
    ${SENSOR}/sensors
)

xy_add_unit_test(test_sht30_legacy_sensor sensor_sht30_legacy UNITY
    ${UNIT_SENSOR}/test_sht30_legacy_sensor.c
    ${SENSOR}/sensors/sensor_sht30.c
)
target_include_directories(test_sht30_legacy_sensor BEFORE PRIVATE
    ${SENSOR}
    ${SENSOR}/sensors
)

xy_add_unit_test(test_mq_gas_sensors sensor_mq_gas_sensors UNITY
    ${UNIT_SENSOR}/test_mq_gas_sensors.c
    ${SENSOR}/sensors/sensor_mq3.c
    ${SENSOR}/sensors/sensor_mq7.c
    ${SENSOR}/sensors/sensor_mq135.c
)
target_include_directories(test_mq_gas_sensors BEFORE PRIVATE
    ${SENSOR}
    ${SENSOR}/sensors
)

xy_add_unit_test(test_stub_i2c_sensors sensor_stub_i2c_sensors UNITY
    ${UNIT_SENSOR}/test_stub_i2c_sensors.c
    ${SENSOR}/sensors/sensor_sgp30.c
    ${SENSOR}/sensors/sensor_sgp40.c
    ${SENSOR}/sensors/sensor_ens160.c
    ${SENSOR}/sensors/sensor_im69d.c
    ${SENSOR}/sensors/sensor_max30102.c
)
target_include_directories(test_stub_i2c_sensors BEFORE PRIVATE
    ${SENSOR}
    ${SENSOR}/sensors
)

xy_add_unit_test(test_proximity_sensors sensor_proximity_sensors UNITY
    ${UNIT_SENSOR}/test_proximity_sensors.c
    ${SENSOR}/sensors/sensor_pa122.c
    ${SENSOR}/sensors/sensor_vcnl4040.c
)
target_include_directories(test_proximity_sensors BEFORE PRIVATE
    ${SENSOR}
    ${SENSOR}/sensors
)

xy_add_unit_test(test_ccs811_sensor sensor_ccs811_sensor UNITY
    ${UNIT_SENSOR}/test_ccs811_sensor.c
    ${SENSOR}/sensors/sensor_ccs811.c
)
target_include_directories(test_ccs811_sensor BEFORE PRIVATE
    ${SENSOR}
    ${SENSOR}/sensors
)

xy_add_unit_test(test_vl53l0x_sensor sensor_vl53l0x_sensor UNITY
    ${UNIT_SENSOR}/test_vl53l0x_sensor.c
    ${SENSOR}/sensors/sensor_vl53l0x.c
)
target_include_directories(test_vl53l0x_sensor BEFORE PRIVATE
    ${SENSOR}
    ${SENSOR}/sensors
)

xy_add_unit_test(test_ap3216c_sensor sensor_ap3216c_sensor UNITY
    ${UNIT_SENSOR}/test_ap3216c_sensor.c
    ${SENSOR}/sensors/sensor_ap3216c.c
)
target_include_directories(test_ap3216c_sensor BEFORE PRIVATE
    ${SENSOR}
    ${SENSOR}/sensors
)

xy_add_unit_test(test_apds9960_sensor sensor_apds9960_sensor UNITY
    ${UNIT_SENSOR}/test_apds9960_sensor.c
    ${SENSOR}/sensors/sensor_apds9960.c
)
target_include_directories(test_apds9960_sensor BEFORE PRIVATE
    ${SENSOR}
    ${SENSOR}/sensors
)

xy_add_unit_test(test_analog_misc_sensors sensor_analog_misc_sensors UNITY
    ${UNIT_SENSOR}/test_analog_misc_sensors.c
    ${SENSOR}/sensors/sensor_acs712.c
    ${SENSOR}/sensors/sensor_fsr.c
    ${SENSOR}/sensors/sensor_mg811.c
)
target_include_directories(test_analog_misc_sensors BEFORE PRIVATE
    ${SENSOR}
    ${SENSOR}/sensors
)

xy_add_unit_test(test_env_i2c_sensors sensor_env_i2c_sensors UNITY
    ${UNIT_SENSOR}/test_env_i2c_sensors.c
    ${SENSOR}/sensors/sensor_aht10.c
    ${SENSOR}/sensors/sensor_bmp390.c
)
target_include_directories(test_env_i2c_sensors BEFORE PRIVATE
    ${SENSOR}
    ${SENSOR}/sensors
)

xy_add_unit_test(test_bmp280_sensor sensor_bmp280 UNITY
    ${UNIT_SENSOR}/test_bmp280_sensor.c
    ${SENSOR}/sensors/sensor_bmp280.c
)
target_include_directories(test_bmp280_sensor BEFORE PRIVATE
    ${SENSOR}
    ${SENSOR}/sensors
)

xy_add_unit_test(test_icm20608_sensor sensor_icm20608 UNITY
    ${UNIT_SENSOR}/test_icm20608_sensor.c
    ${SENSOR}/sensors/sensor_icm20608.c
)
target_include_directories(test_icm20608_sensor BEFORE PRIVATE
    ${SENSOR}
    ${SENSOR}/sensors
)

xy_add_unit_test(test_low_power_accel_sensors sensor_low_power_accel_sensors UNITY
    ${UNIT_SENSOR}/test_low_power_accel_sensors.c
    ${SENSOR}/sensors/sensor_adxl362.c
    ${SENSOR}/sensors/sensor_bma400.c
    ${SENSOR}/sensors/sensor_iis2iclp.c
    ${SENSOR}/sensors/sensor_lis2dw12.c
    ${SENSOR}/sensors/sensor_kx023.c
)
target_include_directories(test_low_power_accel_sensors BEFORE PRIVATE
    ${SENSOR}
    ${SENSOR}/sensors
)

xy_add_unit_test(test_st_silan_accel_sensors sensor_st_silan_accel_sensors UNITY
    ${UNIT_SENSOR}/test_st_silan_accel_sensors.c
    ${SENSOR}/sensors/sensor_lis2dh12.c
    ${SENSOR}/sensors/sensor_lis2dw12.c
    ${SENSOR}/sensors/sensor_sc7a20.c
    ${SENSOR}/sensors/sensor_silan_sc7a20.c
)
target_include_directories(test_st_silan_accel_sensors BEFORE PRIVATE
    ${SENSOR}
    ${SENSOR}/sensors
)

xy_add_unit_test(test_lsm6_imu_sensors sensor_lsm6_imu_sensors UNITY
    ${UNIT_SENSOR}/test_lsm6_imu_sensors.c
    ${SENSOR}/sensors/sensor_lsm6dsl.c
    ${SENSOR}/sensors/sensor_lsm6dso.c
    ${SENSOR}/sensors/sensor_lsm6dsr.c
)
target_include_directories(test_lsm6_imu_sensors BEFORE PRIVATE
    ${SENSOR}
    ${SENSOR}/sensors
)

xy_add_unit_test(test_stub_accel_sensors sensor_stub_accel_sensors UNITY
    ${UNIT_SENSOR}/test_stub_accel_sensors.c
    ${SENSOR}/sensors/sensor_dmp6100.c
    ${SENSOR}/sensors/sensor_cms.c
    ${SENSOR}/sensors/sensor_gd30df.c
    ${SENSOR}/sensors/sensor_hs_ads1100.c
    ${SENSOR}/sensors/sensor_qma6100.c
)
target_include_directories(test_stub_accel_sensors BEFORE PRIVATE
    ${SENSOR}
    ${SENSOR}/sensors
)

xy_add_unit_test(test_lsm9ds1 sensor_lsm9ds1 UNITY
    ${UNIT_SENSOR}/test_lsm9ds1.c
    ${SENSOR}/sensors/sensor_lsm9ds1.c
)
target_include_directories(test_lsm9ds1 BEFORE PRIVATE
    ${SENSOR}
    ${SENSOR}/sensors
)

xy_add_unit_test(test_gps sensor_gps UNITY
    ${UNIT_SENSOR}/test_gps.c
    ${SENSOR}/sensors/sensor_gps.c
)
target_include_directories(test_gps BEFORE PRIVATE
    ${SENSOR}
    ${SENSOR}/sensors
)
target_link_libraries(test_gps PRIVATE m)

xy_add_unit_test(test_magnetic_angle_sensors sensor_magnetic_angle_sensors UNITY
    ${UNIT_SENSOR}/test_magnetic_angle_sensors.c
    ${SENSOR}/sensors/sensor_mlx90393.c
    ${SENSOR}/sensors/sensor_aeat8800.c
)
target_include_directories(test_magnetic_angle_sensors BEFORE PRIVATE
    ${SENSOR}
    ${SENSOR}/sensors
)

xy_add_unit_test(test_magnetic_i2c_sensors sensor_magnetic_i2c_sensors UNITY
    ${UNIT_SENSOR}/test_magnetic_i2c_sensors.c
    ${SENSOR}/sensors/sensor_qmc5883l.c
    ${SENSOR}/sensors/sensor_ist8310.c
    ${SENSOR}/sensors/sensor_ak09918.c
    ${SENSOR}/sensors/sensor_cmm905.c
)
target_include_directories(test_magnetic_i2c_sensors BEFORE PRIVATE
    ${SENSOR}
    ${SENSOR}/sensors
)

xy_add_unit_test(test_adc_power_monitors sensor_adc_power_monitors UNITY
    ${UNIT_SENSOR}/test_adc_power_monitors.c
    ${SENSOR}/src/xy_ltc2945.c
    ${SENSOR}/src/xy_ads1115.c
)
target_include_directories(test_adc_power_monitors BEFORE PRIVATE
    ${SENSOR}/inc
    ${HAL}/inc
    ${KERNEL}/osal
)

xy_add_unit_test(test_coulomb sensor_coulomb UNITY
    ${UNIT_SENSOR}/test_coulomb.c
    ${SENSOR}/src/xy_coulomb.c
)
target_include_directories(test_coulomb BEFORE PRIVATE
    ${SENSOR}/inc
    ${HAL}/inc
    ${KERNEL}/osal
)
target_link_libraries(test_coulomb PRIVATE m)

xy_add_unit_test(test_mpu6050 sensor_mpu6050 UNITY
    ${UNIT_SENSOR}/test_mpu6050.c
    ${SENSOR}/src/xy_mpu6050.c
)
target_include_directories(test_mpu6050 BEFORE PRIVATE
    ${SENSOR}/inc
    ${HAL}/inc
    ${KERNEL}/osal
)

xy_add_unit_test(test_bmi088 sensor_bmi088 UNITY
    ${UNIT_SENSOR}/test_bmi088.c
    ${SENSOR}/src/xy_bmi088.c
)
target_include_directories(test_bmi088 BEFORE PRIVATE
    ${SENSOR}/inc
)

xy_add_unit_test(test_bmi270 sensor_bmi270 UNITY
    ${UNIT_SENSOR}/test_bmi270.c
    ${SENSOR}/src/xy_bmi270.c
)
target_include_directories(test_bmi270 BEFORE PRIVATE
    ${SENSOR}/inc
)
target_link_libraries(test_bmi270 PRIVATE m)

xy_add_unit_test(test_w25qxx sensor_w25qxx UNITY
    ${UNIT_SENSOR}/test_w25qxx.c
    ${SENSOR}/src/xy_w25qxx.c
)
target_include_directories(test_w25qxx BEFORE PRIVATE
    ${SENSOR}/inc
    ${DEVICE}/inc
    ${HAL}/inc
    ${KERNEL}/osal
)

xy_add_unit_test(test_vl53l1x sensor_vl53l1x UNITY
    ${UNIT_SENSOR}/test_vl53l1x.c
    ${SENSOR}/src/xy_vl53l1x.c
)
target_include_directories(test_vl53l1x BEFORE PRIVATE
    ${SENSOR}/inc
)
target_link_libraries(test_vl53l1x PRIVATE m)

xy_add_unit_test(test_bno055 sensor_bno055 UNITY
    ${UNIT_SENSOR}/test_bno055.c
    ${SENSOR}/src/xy_bno055.c
)
target_include_directories(test_bno055 BEFORE PRIVATE
    ${SENSOR}/inc
)
target_link_libraries(test_bno055 PRIVATE m)

xy_add_unit_test(test_dmp sensor_dmp UNITY
    ${UNIT_SENSOR}/test_dmp.c
    ${SENSOR}/src/xy_dmp.c
)
target_include_directories(test_dmp BEFORE PRIVATE
    ${SENSOR}/inc
    ${HAL}/inc
    ${KERNEL}/osal/inc
)
target_link_libraries(test_dmp PRIVATE m)

# --- Test 22: HAL PC simulation contracts for GPIO/UART/I2C/SPI/SYS ---
xy_add_unit_test(test_hal_pc hal_pc UNITY
    ${UNIT_HAL}/test_hal_pc.c
    ${HAL}/PC/xy_hal_pc.c
    ${HAL}/PC/xy_hal_gpio_pc.c
    ${HAL}/PC/xy_hal_i2c_pc.c
    ${HAL}/PC/xy_hal_spi_pc.c
    ${HAL}/PC/xy_hal_uart_pc.c
)
target_include_directories(test_hal_pc PRIVATE
    ${HAL}/inc
    ${HAL}/PC
)

# --- Test 23: PM lifecycle, charger, fuel gauge, and PC platform contracts ---
xy_add_unit_test(test_pm pm_component UNITY
    ${UNIT_PM}/test_pm_core.c
    ${PM}/src/xy_pm_system.c
    ${PM}/src/xy_pm_adc.c
    ${PM}/src/xy_pm_platform.c
    ${PM}/src/xy_charger.c
    ${PM}/src/xy_fuel_gauge.c
    ${CLIB}/xy_stdio.c
    ${CLIB}/xy_string.c
    ${CLIB}/xy_ctype.c
    ${UNIT_SUPPORT}/test_clib_alloc_shim.c
)
target_compile_definitions(test_pm PRIVATE PLATFORM_PC XY_PM_PLATFORM_OWNS_CHARGER_HW XY_PM_ENABLE_TEST_HOOKS)

xy_add_unit_test(test_pm_platform_fallback pm_platform_fallback UNITY
    ${UNIT_PM}/test_pm_platform_fallback.c
    ${PM}/src/xy_pm_platform.c
)
target_compile_definitions(test_pm_platform_fallback PRIVATE XY_PM_ENABLE_TEST_HOOKS)

# --- Test 24: Trace logging output and dynamic-level contracts ---
xy_add_unit_test(test_trace trace_component UNITY
    ${UNIT_TRACE}/test_trace_core.c
    ${TRACE}/xy_log/src/xy_log.c
    ${CLIB}/xy_stdio.c
    ${CLIB}/xy_string.c
    ${CLIB}/xy_ctype.c
    ${UNIT_SUPPORT}/test_clib_alloc_shim.c
)
target_compile_definitions(test_trace PRIVATE XY_LOG_LEVEL=XY_LOG_LEVEL_DEBUG)

# --- Test 25: CLIB common/string/stdio/ring-buffer contracts ---
xy_add_unit_test(test_clib clib_component UNITY
    ${UNIT_CLIB}/test_clib_core.c
    ${CLIB}/xy_common.c
    ${CLIB}/xy_string.c
    ${CLIB}/xy_stdio.c
    ${CLIB}/xy_ctype.c
    ${CLIB}/xy_filter.c
    ${CLIB}/xy_sort.c
    ${CLIB}/xy_rb.c
    ${UNIT_SUPPORT}/test_clib_alloc_shim.c
)

# --- Test 26: MUX manager, registration, and TLV packet contracts ---
xy_add_unit_test(test_mux_core mux_core UNITY
    ${UNIT_MUX}/test_mux_core.c
    ${MUX}/src/xy_mux.c
    ${MUX}/src/xy_mux_gpio.c
    ${MUX}/src/xy_mux_i2c.c
    ${MUX}/src/xy_mux_spi.c
    ${MUX}/src/xy_mux_uart.c
    ${TRACE}/xy_log/src/xy_log.c
    ${CLIB}/xy_stdio.c
    ${CLIB}/xy_string.c
    ${CLIB}/xy_ctype.c
    ${UNIT_SUPPORT}/test_clib_alloc_shim.c
)

# --- Test 27: MUX I2C channel helper contracts ---
xy_add_unit_test(test_mux_i2c mux_i2c UNITY
    ${UNIT_MUX}/test_mux_i2c.c
    ${MUX}/src/xy_mux.c
    ${MUX}/src/xy_mux_i2c.c
    ${TRACE}/xy_log/src/xy_log.c
    ${CLIB}/xy_stdio.c
    ${CLIB}/xy_string.c
    ${CLIB}/xy_ctype.c
    ${UNIT_SUPPORT}/test_clib_alloc_shim.c
)

# --- Test 28: MUX GPIO channel helper contracts ---
xy_add_unit_test(test_mux_gpio mux_gpio UNITY
    ${UNIT_MUX}/test_mux_gpio.c
    ${MUX}/src/xy_mux.c
    ${MUX}/src/xy_mux_gpio.c
    ${TRACE}/xy_log/src/xy_log.c
    ${CLIB}/xy_stdio.c
    ${CLIB}/xy_string.c
    ${CLIB}/xy_ctype.c
    ${UNIT_SUPPORT}/test_clib_alloc_shim.c
)

# --- Test 29: PID core lifecycle, tuning, limits, and compute contracts ---
xy_add_unit_test(test_pid_core pid_core UNITY
    ${UNIT_PID}/test_pid_core.c
    ${PID}/xy_pid.c
    ${TRACE}/xy_log/src/xy_log.c
    ${CLIB}/xy_stdio.c
)
target_link_libraries(test_pid_core m)

# --- Test 29b: PID auto tuner lifecycle, sampling, and apply contracts ---
xy_add_unit_test(test_pid_auto pid_auto UNITY
    ${UNIT_PID}/test_pid_auto.c
    ${PID}/xy_pid.c
    ${PID}/src/xy_pid_auto.c
    ${TRACE}/xy_log/src/xy_log.c
    ${CLIB}/xy_stdio.c
)
target_link_libraries(test_pid_auto m)

# --- Test 29c: PID examples stay aligned with public APIs ---
xy_add_unit_test(test_pid_example_basic pid_example_basic
    ${PID}/examples/example_basic.c
    ${PID}/xy_pid.c
    ${TRACE}/xy_log/src/xy_log.c
    ${CLIB}/xy_stdio.c
    ${UNIT_PID}/test_pid_example_log_stub.c
)
target_link_libraries(test_pid_example_basic m)

xy_add_unit_test(test_pid_example_incremental pid_example_incremental
    ${PID}/examples/example_incremental.c
    ${PID}/xy_pid.c
    ${TRACE}/xy_log/src/xy_log.c
    ${CLIB}/xy_stdio.c
    ${UNIT_PID}/test_pid_example_log_stub.c
)
target_link_libraries(test_pid_example_incremental m)

xy_add_unit_test(test_pid_example_auto_tune pid_example_auto_tune
    ${PID}/examples/example_auto_tune.c
    ${PID}/xy_pid.c
    ${PID}/src/xy_pid_auto.c
    ${TRACE}/xy_log/src/xy_log.c
    ${CLIB}/xy_stdio.c
    ${UNIT_PID}/test_pid_example_log_stub.c
)
target_link_libraries(test_pid_example_auto_tune m)

xy_add_unit_test(test_pid_example_temperature pid_example_temperature
    ${PID}/examples/example_temperature.c
    ${PID}/xy_pid.c
    ${TRACE}/xy_log/src/xy_log.c
    ${CLIB}/xy_stdio.c
    ${UNIT_PID}/test_pid_example_log_stub.c
)
target_link_libraries(test_pid_example_temperature m)

xy_add_unit_test(test_pid_example_charging pid_example_charging
    ${PID}/examples/example_charging.c
    ${PID}/xy_pid.c
    ${TRACE}/xy_log/src/xy_log.c
    ${CLIB}/xy_stdio.c
    ${UNIT_PID}/test_pid_example_log_stub.c
)
target_link_libraries(test_pid_example_charging m)

# --- Test 30: MUX SPI channel helper contracts ---
xy_add_unit_test(test_mux_spi mux_spi UNITY
    ${UNIT_MUX}/test_mux_spi.c
    ${MUX}/src/xy_mux.c
    ${MUX}/src/xy_mux_spi.c
    ${TRACE}/xy_log/src/xy_log.c
    ${CLIB}/xy_stdio.c
    ${CLIB}/xy_string.c
    ${CLIB}/xy_ctype.c
    ${UNIT_SUPPORT}/test_clib_alloc_shim.c
)

# --- Test 31: MUX UART channel helper contracts ---
xy_add_unit_test(test_mux_uart mux_uart UNITY
    ${UNIT_MUX}/test_mux_uart.c
    ${MUX}/src/xy_mux.c
    ${MUX}/src/xy_mux_uart.c
    ${TRACE}/xy_log/src/xy_log.c
    ${CLIB}/xy_stdio.c
    ${CLIB}/xy_string.c
    ${CLIB}/xy_ctype.c
    ${UNIT_SUPPORT}/test_clib_alloc_shim.c
)

# --- Test 31b: MUX build-guarded public API example ---
xy_add_unit_test(test_mux_example_basic mux_example_basic
    ${MUX}/examples/example_mux_basic.c
    ${MUX}/src/xy_mux.c
    ${MUX}/src/xy_mux_gpio.c
    ${MUX}/src/xy_mux_i2c.c
    ${MUX}/src/xy_mux_spi.c
    ${MUX}/src/xy_mux_uart.c
    ${TRACE}/xy_log/src/xy_log.c
    ${CLIB}/xy_stdio.c
    ${CLIB}/xy_string.c
    ${CLIB}/xy_ctype.c
    ${UNIT_SUPPORT}/test_clib_alloc_shim.c
    ${UNIT_PID}/test_pid_example_log_stub.c
)

# --- Test 28: DM base64 public vectors and binary roundtrip ---
xy_add_unit_test(test_dm_base64 dm_base64 UNITY
    ${UNIT_DM}/test_dm_base64.c
    ${DM}/xy_base64/xy_base64.c
)
target_include_directories(test_dm_base64 PRIVATE ${ROOT}/components)

# --- Test 28b: DM TLV encode/decode contracts ---
xy_add_unit_test(test_dm_tlv dm_tlv UNITY
    ${UNIT_DM}/test_dm_tlv.c
    ${DM}/xy_tlv/xy_tlv.c
)
target_include_directories(test_dm_tlv PRIVATE ${DM}/xy_tlv)

# --- Test 29: DM NVM key-value storage contracts ---
xy_add_unit_test(test_dm_nvm dm_nvm UNITY
    ${UNIT_DM}/test_dm_nvm_core.c
    ${DM}/xy_nvm/src/xy_nvm.c
    ${TRACE}/xy_log/src/xy_log.c
    ${CLIB}/xy_stdio.c
    ${CLIB}/xy_string.c
    ${CLIB}/xy_ctype.c
    ${UNIT_SUPPORT}/test_clib_alloc_shim.c
)
target_include_directories(test_dm_nvm PRIVATE ${DM}/xy_nvm/inc)

# --- Test 29b: DM factory public contracts ---
xy_add_unit_test(test_dm_factory dm_factory UNITY
    ${UNIT_DM}/test_dm_factory.c
    ${DM}/factory/xy_factory.c
)
target_include_directories(test_dm_factory PRIVATE ${DM}/factory)

# --- Test 29c: DM FEE flash EEPROM emulation contracts ---
xy_add_unit_test(test_dm_fee dm_fee UNITY
    ${UNIT_DM}/test_dm_fee.c
    ${DM}/fee/src/xy_fee.c
)
target_include_directories(test_dm_fee PRIVATE ${DM}/fee/inc)

# --- Test 29d: DM coreJSON parser/search contracts ---
xy_add_unit_test(test_dm_corejson dm_corejson UNITY
    ${UNIT_DM}/test_dm_corejson.c
    ${DM}/coreJSON/core_json.c
)
target_include_directories(test_dm_corejson PRIVATE ${DM}/coreJSON)

# --- Test 30: IPC pipe ring-buffer contracts ---
xy_add_unit_test(test_ipc_pipe ipc_pipe UNITY
    ${UNIT_IPC}/test_ipc_pipe.c
    ${IPC}/pipe/xy_pipe.c
)

# --- Test 31: IPC broker direct-message, pub/sub, and request contracts ---
xy_add_unit_test(test_ipc_broker ipc_broker UNITY
    ${UNIT_IPC}/test_ipc_broker.c
    ${IPC}/xy_broker/xy_broker.c
)

# --- Test 31b: IPC message-queue FIFO/overflow/timeout contracts ---
xy_add_unit_test(test_ipc_mq ipc_mq UNITY
    ${UNIT_IPC}/test_ipc_mq.c
    ${IPC}/src/xy_mq.c
    ${TRACE}/xy_log/src/xy_log.c
    ${CLIB}/xy_stdio.c
)
target_include_directories(test_ipc_mq PRIVATE ${IPC}/inc ${TRACE}/xy_log/inc)

# --- Test 31c: IPC observer/subject public contracts ---
xy_add_unit_test(test_ipc_observer ipc_observer UNITY
    ${UNIT_IPC}/test_ipc_observer.c
    ${IPC}/observer/xy_observer.c
)
target_include_directories(test_ipc_observer PRIVATE ${IPC}/observer)

# --- Test 31d: IPC event-group OSAL wrapper contracts ---
xy_add_unit_test(test_ipc_event_group ipc_event_group UNITY
    ${UNIT_IPC}/test_ipc_event_group.c
    ${IPC}/src/xy_event_group.c
    ${OSAL}/backend/baremetal/xy_os_baremetal.c
)
target_include_directories(test_ipc_event_group PRIVATE ${IPC}/inc ${OSAL} ${OSAL}/inc)

# --- Test 32: Storage EEPROM 24xx fake-I2C host contracts ---
xy_add_unit_test(test_storage_eeprom_24xx storage_eeprom_24xx UNITY
    ${UNIT_STORAGE}/test_storage_eeprom_24xx.c
    ${STORAGE}/eeprom/24xx/xy_eeprom_24xx.c
)

# --- Test 33: Display LCD SPI/I8080/ST7789 host contracts ---
xy_add_unit_test(test_display_lcd display_lcd UNITY
    ${UNIT_DISPLAY}/test_display_lcd.c
    ${DISPLAY}/lcd/xy_lcd.c
    ${DISPLAY}/lcd/xy_lcd_spi.c
    ${DISPLAY}/lcd/xy_lcd_i8080.c
    ${DISPLAY}/lcd/xy_lcd_st7789.c
)

# --- Test 34: Display OLED SSD1306 / WS2812 host contracts ---
xy_add_unit_test(test_display_oled_ws2812 display_oled_ws2812 UNITY
    ${UNIT_DISPLAY}/test_display_oled_ws2812.c
    ${DISPLAY}/oled/ssd1306/xy_oled_ssd1306.c
    ${DISPLAY}/led_drivers/serial_rgb/xy_ws2812.c
)

# --- Test 35: Display RGB matrix layout/effect contracts ---
xy_add_unit_test(test_display_rgb_matrix display_rgb_matrix UNITY
    ${UNIT_DISPLAY}/test_display_rgb_matrix.c
    ${DISPLAY}/led_drivers/serial_rgb/xy_rgb_matrix.c
    ${DISPLAY}/led_drivers/serial_rgb/xy_ws2812.c
    ${DEV}/xy_device_core.c
)

# --- Test 36: Display serial RGB public helper headers remain self-contained ---
xy_add_unit_test(test_display_serial_rgb_headers display_serial_rgb_headers UNITY
    ${UNIT_DISPLAY}/test_display_serial_rgb_headers.c
)

# --- Test 37: Display LED driver GUI adapter contracts ---
xy_add_unit_test(test_display_led_driver display_led_driver UNITY
    ${UNIT_DISPLAY}/test_led_driver.c
    ${DISPLAY}/led_drivers/xy_led_driver.c
)

# --- Test 38: MQTT client helpers, topic filters, and validation ---
xy_add_unit_test(test_mqtt_client mqtt_client UNITY
    ${UNIT_NET}/test_mqtt_client_core.c
    ${NET}/src/xy_mqtt_client.c
)

# --- Test 31: AT client registration, responses, and IO helpers ---
xy_add_unit_test(test_at_client at_client UNITY
    ${UNIT_NET}/test_at_client_core.c
    ${NET}/at/atc/Modules/cellular/common/at_client.c
)

# --- Test 32: AT server command dispatch, responses, and parsing ---
xy_add_unit_test(test_at_server at_server UNITY
    ${UNIT_NET}/test_at_server_core.c
    ${NET}/at/ats/xy_ats.c
    ${NET}/at/ats/xy_ats_hash.c
)
target_include_directories(test_at_server PRIVATE ${TRACE}/xy_log/inc)

# --- Test 33: FOTA lifecycle, CRC, download, and flash-op contracts ---
xy_add_unit_test(test_fota_core fota_core UNITY
    ${UNIT_FOTA}/test_fota_core.c
    ${FOTA}/xy_fota.c
)

# --- Test 33b: FOTA public update flow smoke example stays build-guarded ---
xy_add_unit_test(test_fota_smoke_example fota_smoke_example UNITY
    ${UNIT_FOTA}/test_fota_smoke_example.c
    ${FOTA}/xy_fota.c
)

# --- Test 34: GUI core drawing and object contracts ---
xy_add_unit_test(test_gui_core gui_core UNITY
    ${UNIT_GUI}/test_gui_core.c
    ${GUI}/xy_gui.c
)

# --- Test 35: GUI widget base and theme contracts ---
xy_add_unit_test(test_gui_widget_theme gui_widget_theme UNITY
    ${UNIT_GUI}/test_gui_widget_theme.c
    ${GUI}/src/xy_gui_widget.c
    ${GUI}/src/xy_gui_theme.c
)

# --- Test 36: GUI event and remaining widget contracts ---
xy_add_unit_test(test_gui_widgets gui_widgets UNITY
    ${UNIT_GUI}/test_gui_widgets.c
    ${GUI}/src/xy_gui_event.c
    ${GUI}/src/xy_gui_widget.c
    ${GUI}/src/xy_gui_button.c
    ${GUI}/src/xy_gui_checkbox.c
    ${GUI}/src/xy_gui_label.c
    ${GUI}/src/xy_gui_progress.c
    ${GUI}/src/xy_gui_slider.c
    ${GUI}/src/xy_gui_container.c
    ${GUI}/src/xy_gui_draw.c
    ${GUI}/src/xy_font.c
    ${TRACE}/xy_log/src/xy_log.c
    ${CLIB}/xy_stdio.c
    ${CLIB}/xy_string.c
    ${CLIB}/xy_ctype.c
    ${UNIT_SUPPORT}/test_clib_alloc_shim.c
)
target_link_libraries(test_gui_widgets m)

# --- Test 37: GUI display backend bridge contract ---
xy_add_unit_test(test_gui_display_backend gui_display_backend UNITY
    ${UNIT_GUI}/test_gui_display_backend.c
    ${GUI}/xy_gui.c
    ${DISPLAY}/led_drivers/xy_led_driver.c
)

# --- Test 37a: GUI to SSD1306 display-driver adapter contract ---
xy_add_unit_test(test_gui_ssd1306_adapter gui_ssd1306_adapter UNITY
    ${UNIT_GUI}/test_gui_ssd1306_adapter.c
    ${GUI}/xy_gui.c
    ${GUI}/src/xy_gui_ssd1306_adapter.c
    ${DISPLAY}/oled/ssd1306/xy_oled_ssd1306.c
)

# --- Test 37b: GUI effects public headers stay self-contained ---
xy_add_unit_test(test_gui_effects_headers gui_effects_headers UNITY
    ${UNIT_GUI}/test_gui_effects_headers.c
)
target_include_directories(test_gui_effects_headers PRIVATE
    ${GUI}/effects
)

# --- Test 37b: GUI effects algorithm contracts ---
xy_add_unit_test(test_gui_effects gui_effects UNITY
    ${UNIT_GUI}/test_gui_effects.c
    ${GUI}/effects/xy_gui_effect_fade.c
    ${GUI}/effects/xy_gui_effect_blink.c
    ${GUI}/effects/xy_gui_effect_breath.c
    ${GUI}/effects/xy_gui_effect_slide.c
    ${GUI}/effects/xy_gui_effect_rotate.c
)
target_include_directories(test_gui_effects PRIVATE
    ${GUI}/effects
)
target_link_libraries(test_gui_effects m)

# --- Test 37c: GUI LED-screen extended effects public headers stay self-contained ---
xy_add_unit_test(test_gui_led_screen_effects gui_led_screen_effects UNITY
    ${UNIT_GUI}/test_gui_led_screen_effects.c
)
target_include_directories(test_gui_led_screen_effects PRIVATE
    ${GUI}/effects
)

# --- Test 37d: GUI RGB extended effects compile against serial-RGB seam ---
xy_add_unit_test(test_gui_rgb_extended_effects_compile gui_rgb_extended_effects_compile UNITY
    ${UNIT_GUI}/test_gui_rgb_extended_effects_compile.c
    ${GUI}/effects/xy_rgb_fx_extended.c
    ${GUI}/effects/xy_rgb_fx_music.c
    ${GUI}/effects/xy_rgb_fx_matrix.c
    ${GUI}/effects/xy_rgb_fx_3d.c
)
target_include_directories(test_gui_rgb_extended_effects_compile PRIVATE
    ${DISPLAY}/led_drivers/serial_rgb
    ${GUI}/effects
)
target_link_libraries(test_gui_rgb_extended_effects_compile m)

# --- Test 37e: GUI bitmap font lookup and measurement contracts ---
xy_add_unit_test(test_gui_fonts gui_fonts UNITY
    ${UNIT_GUI}/test_gui_fonts.c
    ${GUI}/fonts/xy_font_8x16.c
    ${GUI}/fonts/xy_font_16x24.c
    ${GUI}/fonts/xy_font_chinese_16x16.c
)

# --- Test 37g: GUI bitmap font manifest/inventory contracts ---
xy_add_unit_test(test_gui_font_manifest gui_font_manifest UNITY
    ${UNIT_GUI}/test_gui_font_manifest.c
    ${GUI}/fonts/xy_font_8x16.c
    ${GUI}/fonts/xy_font_16x24.c
    ${GUI}/fonts/xy_font_chinese_16x16.c
)

# --- Test 37h: GUI bitmap font generator validates manifest structure/output preview ---
add_test(NAME gui_font_generator_manifest
    COMMAND python3 ${GUI}/fonts/tools/generate_bitmap_font.py --check
    WORKING_DIRECTORY ${ROOT}
)
add_test(NAME gui_font_generator_output
    COMMAND python3 ${GUI}/fonts/tools/generate_bitmap_font.py --self-test-output
    WORKING_DIRECTORY ${ROOT}
)
add_test(NAME gui_font_generator_write
    COMMAND python3 ${GUI}/fonts/tools/generate_bitmap_font.py --self-test-write
    WORKING_DIRECTORY ${ROOT}
)
add_test(NAME gui_font_generator_glyph_metadata
    COMMAND python3 ${GUI}/fonts/tools/generate_bitmap_font.py --self-test-glyph-metadata
    WORKING_DIRECTORY ${ROOT}
)

# --- Test 37f: GUI font engine draw/cache contracts ---
xy_add_unit_test(test_gui_font_engine gui_font_engine UNITY
    ${UNIT_GUI}/test_gui_font_engine.c
    ${GUI}/src/xy_font.c
    ${TRACE}/xy_log/src/xy_log.c
    ${CLIB}/xy_stdio.c
    ${CLIB}/xy_string.c
    ${CLIB}/xy_ctype.c
    ${UNIT_SUPPORT}/test_clib_alloc_shim.c
)
target_include_directories(test_gui_fonts PRIVATE
    ${GUI}/fonts
)
target_include_directories(test_gui_font_manifest PRIVATE
    ${GUI}/fonts
)
target_include_directories(test_gui_font_engine PRIVATE
    ${GUI}/inc
)

# --- Test 38: Analog ADC/DAC, MCP3008, and HX711 host contracts ---
xy_add_unit_test(test_analog_devices analog_devices UNITY
    ${UNIT_ANALOG}/test_analog_devices.c
    ${ANALOG}/xy_adc.c
    ${ANALOG}/src/xy_mcp3008.c
    ${ANALOG}/src/xy_hx711.c
    ${TRACE}/xy_log/src/xy_log.c
    ${CLIB}/xy_stdio.c
    ${CLIB}/xy_string.c
    ${CLIB}/xy_ctype.c
    ${UNIT_SUPPORT}/test_clib_alloc_shim.c
)

# --- Test 39: SYS timer and state-machine host contracts ---
xy_add_unit_test(test_sys_timer_sm sys_timer_sm UNITY
    ${UNIT_SYS}/test_sys_timer_sm.c
    ${SYS}/xy_timer/xy_timer.c
    ${SYS}/xy_state_machine/xy_st.c
)
target_compile_definitions(test_sys_timer_sm PRIVATE reentrant= PLATFORM=0 PLATFORM_C51=1)

xy_add_unit_test(test_bootreason_check bootreason_check UNITY
    ${UNIT_KERNEL}/test_bootreason_check.c
)

# --- Test 40: Net SMBus/PMBus helpers and default ops ---
xy_add_unit_test(test_net_smbus_pmbus net_smbus_pmbus UNITY
    ${UNIT_NET}/test_net_smbus_pmbus.c
    ${NET}/smbus/xy_smbus.c
    ${NET}/smbus/xy_pmbus.c
    ${TRACE}/xy_log/src/xy_log.c
    ${CLIB}/xy_stdio.c
    ${CLIB}/xy_string.c
    ${CLIB}/xy_ctype.c
    ${UNIT_SUPPORT}/test_clib_alloc_shim.c
)
target_link_libraries(test_net_smbus_pmbus m)

# --- Test 41: Actuator framework, relay, servo, PWM, and batch contracts ---
xy_add_unit_test(test_actuator_framework actuator_framework UNITY
    ${UNIT_ACTUATOR}/test_actuator_framework.c
    ${ACTUATOR}/xy_actuator.c
)
target_link_libraries(test_actuator_framework m)

# --- Test 41b: Actuator example stays aligned with public APIs ---
xy_add_unit_test(test_actuator_example_relay_servo_pwm actuator_example_relay_servo_pwm
    ${ACTUATOR}/examples/example_relay_servo_pwm.c
    ${ACTUATOR}/xy_actuator.c
)
target_link_libraries(test_actuator_example_relay_servo_pwm m)

# --- Test 42: BQ25620 charger I2C register and status contracts ---
xy_add_unit_test(test_charger_bq25620 charger_bq25620 UNITY
    ${UNIT_PM}/test_charger_bq25620.c
    ${ROOT}/components/charger/src/xy_bq25620.c
)
target_include_directories(test_charger_bq25620 PRIVATE ${ROOT}/components/charger/inc)

# --- Test 43: Standalone fuel gauge core API contracts ---
xy_add_unit_test(test_fuel_gauge_core fuel_gauge_core UNITY
    ${UNIT_FUEL_GAUGE}/test_fuel_gauge_core.c
    ${ROOT}/components/fuel_gauge/core/fuel_gauge_core.c
    ${ROOT}/components/fuel_gauge/core/xy_fuel_gauge_safety.c
    ${ROOT}/components/fuel_gauge/core/xy_fuel_gauge_security.c
    ${ROOT}/components/fuel_gauge/core/xy_fuel_gauge_status.c
    ${CLIB}/xy_stdio.c
    ${CLIB}/xy_string.c
    ${CLIB}/xy_ctype.c
    ${UNIT_SUPPORT}/test_clib_alloc_shim.c
)
target_include_directories(test_fuel_gauge_core PRIVATE ${ROOT}/components/fuel_gauge/inc)

# --- Test 44: BQ27Z746 fuel gauge driver host guard ---
xy_add_unit_test(test_fg_bq27z746 fg_bq27z746 UNITY
    ${UNIT_FUEL_GAUGE}/test_fg_bq27z746.c
    ${ROOT}/components/fuel_gauge/drivers/xy_fg_bq27z746.c
    ${ROOT}/components/fuel_gauge/core/fuel_gauge_core.c
    ${CLIB}/xy_stdio.c
    ${CLIB}/xy_string.c
    ${CLIB}/xy_ctype.c
    ${UNIT_SUPPORT}/test_clib_alloc_shim.c
)
target_include_directories(test_fg_bq27z746 PRIVATE
    ${ROOT}/components/fuel_gauge/drivers
    ${ROOT}/components/fuel_gauge/inc
    ${SENSOR}/inc
)

# --- Test 45: BQ40Z50 fuel gauge driver host guard ---
xy_add_unit_test(test_fg_bq40z50 fg_bq40z50 UNITY
    ${UNIT_FUEL_GAUGE}/test_fg_bq40z50.c
    ${ROOT}/components/fuel_gauge/drivers/xy_fg_bq40z50.c
    ${ROOT}/components/fuel_gauge/core/fuel_gauge_core.c
    ${CLIB}/xy_stdio.c
    ${CLIB}/xy_string.c
    ${CLIB}/xy_ctype.c
    ${UNIT_SUPPORT}/test_clib_alloc_shim.c
)
target_include_directories(test_fg_bq40z50 PRIVATE
    ${ROOT}/components/fuel_gauge/drivers
    ${ROOT}/components/fuel_gauge/inc
    ${SENSOR}/inc
)

# --- Test 46: MAX17043 fuel gauge driver host guard ---
xy_add_unit_test(test_fg_max17043 fg_max17043 UNITY
    ${UNIT_FUEL_GAUGE}/test_fg_max17043.c
    ${ROOT}/components/fuel_gauge/drivers/xy_fg_max17043.c
    ${ROOT}/components/fuel_gauge/core/fuel_gauge_core.c
    ${CLIB}/xy_stdio.c
    ${CLIB}/xy_string.c
    ${CLIB}/xy_ctype.c
    ${UNIT_SUPPORT}/test_clib_alloc_shim.c
)
target_include_directories(test_fg_max17043 PRIVATE
    ${ROOT}/components/fuel_gauge/drivers
    ${ROOT}/components/fuel_gauge/inc
    ${SENSOR}/inc
)

# --- Test 47: BQ27Z561 fuel gauge driver host guard ---
xy_add_unit_test(test_fg_bq27z561 fg_bq27z561 UNITY
    ${UNIT_FUEL_GAUGE}/test_fg_bq27z561.c
    ${ROOT}/components/fuel_gauge/drivers/xy_fg_bq27z561.c
    ${ROOT}/components/fuel_gauge/core/fuel_gauge_core.c
    ${CLIB}/xy_stdio.c
    ${CLIB}/xy_string.c
    ${CLIB}/xy_ctype.c
    ${UNIT_SUPPORT}/test_clib_alloc_shim.c
)
target_include_directories(test_fg_bq27z561 PRIVATE
    ${ROOT}/components/fuel_gauge/drivers
    ${ROOT}/components/fuel_gauge/inc
    ${SENSOR}/inc
)

# --- Test 48: Fuel Gauge SMBus board-validation smoke skeleton ---
xy_add_unit_test(test_fuel_gauge_smbus_hardware_smoke_example fuel_gauge_smbus_hardware_smoke_example UNITY
    ${UNIT_FUEL_GAUGE}/test_fuel_gauge_smbus_hardware_smoke_example.c
    ${ROOT}/components/fuel_gauge/drivers/xy_fg_bq40z50.c
    ${ROOT}/components/fuel_gauge/drivers/xy_fg_bq27z561.c
    ${ROOT}/components/fuel_gauge/core/fuel_gauge_core.c
    ${CLIB}/xy_stdio.c
    ${CLIB}/xy_string.c
    ${CLIB}/xy_ctype.c
    ${UNIT_SUPPORT}/test_clib_alloc_shim.c
)
target_include_directories(test_fuel_gauge_smbus_hardware_smoke_example PRIVATE
    ${ROOT}/components/fuel_gauge/drivers
    ${ROOT}/components/fuel_gauge/inc
    ${SENSOR}/inc
)

# --- Test 49: Unity + FFF framework smoke guard ---
xy_add_unit_test(test_fff_smoke fff_smoke UNITY
    ${UNIT_FRAMEWORK}/test_fff_smoke.c
)

# Convenience target: build + run all tests
get_property(XY_UNIT_TEST_TARGETS GLOBAL PROPERTY XY_UNIT_TEST_TARGETS)
add_custom_target(run_unit_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS ${XY_UNIT_TEST_TARGETS}
    COMMENT "Running unit tests"
)

if(XY_ENABLE_UNIT_COVERAGE)
    find_program(GCOVR_EXECUTABLE gcovr)
    if(GCOVR_EXECUTABLE)
        set(XY_UNIT_COVERAGE_DIR ${CMAKE_BINARY_DIR}/coverage)
        add_custom_target(unit_coverage
            COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
            COMMAND ${CMAKE_COMMAND} -E make_directory ${XY_UNIT_COVERAGE_DIR}
            COMMAND ${GCOVR_EXECUTABLE}
                    --root ${ROOT}
                    --filter ${ROOT}/components
                    --exclude ${ROOT}/tests
                    --exclude ${ROOT}/third_party
                    --exclude ${ROOT}/build
                    --merge-mode-functions merge-use-line-min
                    --print-summary
                    --html --html-details
                    --output ${XY_UNIT_COVERAGE_DIR}/index.html
            DEPENDS run_unit_tests
            COMMENT "Run unit tests and generate gcovr coverage report"
        )
    else()
        add_custom_target(unit_coverage
            COMMAND ${CMAKE_COMMAND} -E echo "gcovr_not_found_install_gcovr_to_generate_coverage_reports"
            COMMAND ${CMAKE_COMMAND} -E false
            DEPENDS run_unit_tests
        )
    endif()
endif()
