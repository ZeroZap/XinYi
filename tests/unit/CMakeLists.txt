cmake_minimum_required(VERSION 3.10)
project(xy_unit_tests C)
set(CMAKE_C_STANDARD 99)

# PC simulation mode
add_definitions(-DHAL_PLATFORM_PC -DXY_HAL_PC_SIM)
add_compile_options(-Wall -Wextra -Wno-unused-parameter -g -O0)

# Path roots
set(ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../..)
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

function(xy_add_unit_test TARGET_NAME TEST_NAME)
    cmake_parse_arguments(XY_UNIT "UNITY" "" "" ${ARGN})

    set(XY_UNIT_SOURCES ${XY_UNIT_UNPARSED_ARGUMENTS})
    if(XY_UNIT_UNITY)
        list(APPEND XY_UNIT_SOURCES ${ROOT}/tests/unity/unity.c)
    endif()

    add_executable(${TARGET_NAME} ${XY_UNIT_SOURCES})
    add_test(NAME ${TEST_NAME} COMMAND ${TARGET_NAME})
endfunction()

# --- Test 1: framework end-to-end ---
add_executable(test_device
    ${UNIT_DEVICE}/test_device_framework.c
    ${FRAMEWORK_SOURCES}
    ${ROOT}/tests/unity/unity.c
)

# --- Test 2: SHT30 driver integration ---
add_executable(test_sht30_integration
    ${UNIT_SENSOR}/test_sht30_integration.c
    ${DRV}/sensor/temperature/sht30/xy_sht30.c
    ${FRAMEWORK_SOURCES}
    ${ROOT}/tests/unity/unity.c
)

# --- Test 3: heterogeneous sensor coexistence ---
add_executable(test_sensors_multi
    ${UNIT_SENSOR}/test_sensors_multi.c
    ${DRV}/sensor/temperature/sht30/xy_sht30.c
    ${DRV}/sensor/motion/mpu6050/xy_mpu6050.c
    ${DRV}/sensor/pressure/bmp280/xy_bmp280.c
    ${DRV}/sensor/adc/ads1115/xy_ads1115.c
    ${FRAMEWORK_SOURCES}
    ${ROOT}/tests/unity/unity.c
)

# --- Test 4: SPI device helper + mixed-bus registry ---
add_executable(test_spi_device
    ${UNIT_DEVICE}/test_spi_device.c
    ${FRAMEWORK_SOURCES}
    ${ROOT}/tests/unity/unity.c
)

# --- Test 6: XY_DEVICE_REGISTER auto-registration via GCC constructors ---
add_executable(test_auto_register
    ${UNIT_DEVICE}/test_auto_register.c
    ${FRAMEWORK_SOURCES}
    ${ROOT}/tests/unity/unity.c
)

# --- Test 8: device registry example stays aligned with public APIs ---
add_executable(test_device_registry_example
    ${ROOT}/examples/device_registry_example.c
    ${FRAMEWORK_SOURCES}
)

# --- Test 9: device driver template compiles and runs against public APIs ---
add_executable(test_device_driver_template
    ${ROOT}/examples/device_driver_template.c
    ${FRAMEWORK_SOURCES}
    ${ROOT}/tests/unity/unity.c
)

# --- Test 10: CAN protocol component API + callback/fifo behavior ---
xy_add_unit_test(test_can can_component UNITY
    ${UNIT_NET}/test_can.c
    ${NET}/src/xy_can.c
    ${CLIB}/xy_stdio.c
)

# --- Test 11: LTE component lifecycle, callback storage, and validation ---
xy_add_unit_test(test_lte lte_component UNITY
    ${UNIT_NET}/test_lte.c
    ${NET}/src/xy_lte.c
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
add_executable(test_net_core
    ${UNIT_NET}/test_net_core.c
    ${NET}/src/xy_net.c
    ${NET}/src/xy_net_platform.c
    ${NET}/modbus_tiny/mb_tiny.c
    ${CLIB}/xy_stdio.c
    ${ROOT}/tests/unity/unity.c
)

target_include_directories(test_net_core PRIVATE
    ${NET}/modbus_tiny
    ${NET}/modbus_full
    ${NET}/at/atc/core
)

# --- Test 16: CRC core and common variants ---
xy_add_unit_test(test_crc crypto_crc UNITY
    ${UNIT_CRYPTO}/test_crc.c
    ${CRYPTO}/xy_crc/xy_crc.c
)

# --- Test 5: OSAL baremetal primitives (mutex/sem/event/queue/pool) ---
set(OSAL_ROOT ${ROOT}/components/kernel)

# --- Test 7: optional xy_device_async_*_ex helper API ---
add_executable(test_device_async_helper
    ${UNIT_DEVICE}/test_device_async_helper.c
    ${DEV}/src/xy_device_async.c
    ${ROOT}/tests/unity/unity.c
)
target_include_directories(test_device_async_helper PRIVATE
    ${OSAL_ROOT}/osal
    ${OSAL_ROOT}/osal/inc
)

add_executable(test_osal
    ${UNIT_KERNEL}/test_osal.c
    ${OSAL_ROOT}/osal/backend/baremetal/xy_os_baremetal.c
    ${OSAL_ROOT}/osal/src/xy_os_tick.c
    ${OSAL_ROOT}/misc/xy_tick.c
    ${OSAL_ROOT}/misc/xy_timer_sw.c
    ${ROOT}/tests/unity/unity.c
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

enable_testing()
add_test(NAME device_framework  COMMAND test_device)
add_test(NAME sht30_integration COMMAND test_sht30_integration)
add_test(NAME sensors_multi     COMMAND test_sensors_multi)
add_test(NAME spi_device        COMMAND test_spi_device)
add_test(NAME osal_baremetal    COMMAND test_osal)
add_test(NAME auto_register     COMMAND test_auto_register)
add_test(NAME device_async_helper COMMAND test_device_async_helper)
add_test(NAME device_registry_example COMMAND test_device_registry_example)
add_test(NAME device_driver_template COMMAND test_device_driver_template)
add_test(NAME net_core COMMAND test_net_core)

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

# --- Test 34: GUI core drawing and object contracts ---
add_executable(test_gui_core
    ${UNIT_GUI}/test_gui_core.c
    ${GUI}/xy_gui.c
    ${ROOT}/tests/unity/unity.c
)
add_test(NAME gui_core COMMAND test_gui_core)

# --- Test 35: GUI widget base and theme contracts ---
add_executable(test_gui_widget_theme
    ${UNIT_GUI}/test_gui_widget_theme.c
    ${GUI}/src/xy_gui_widget.c
    ${GUI}/src/xy_gui_theme.c
    ${ROOT}/tests/unity/unity.c
)
add_test(NAME gui_widget_theme COMMAND test_gui_widget_theme)

# --- Test 36: GUI event and remaining widget contracts ---
add_executable(test_gui_widgets
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
    ${ROOT}/tests/unity/unity.c
)
target_link_libraries(test_gui_widgets m)
add_test(NAME gui_widgets COMMAND test_gui_widgets)

# --- Test 38: Analog ADC/DAC, MCP3008, and HX711 host contracts ---
add_executable(test_analog_devices
    ${UNIT_ANALOG}/test_analog_devices.c
    ${ANALOG}/xy_adc.c
    ${ANALOG}/src/xy_mcp3008.c
    ${ANALOG}/src/xy_hx711.c
    ${TRACE}/xy_log/src/xy_log.c
    ${CLIB}/xy_stdio.c
    ${CLIB}/xy_string.c
    ${CLIB}/xy_ctype.c
    ${UNIT_SUPPORT}/test_clib_alloc_shim.c
    ${ROOT}/tests/unity/unity.c
)
add_test(NAME analog_devices COMMAND test_analog_devices)

# --- Test 39: SYS timer and state-machine host contracts ---
add_executable(test_sys_timer_sm
    ${UNIT_SYS}/test_sys_timer_sm.c
    ${SYS}/xy_timer/xy_timer.c
    ${SYS}/xy_state_machine/xy_st.c
    ${ROOT}/tests/unity/unity.c
)
target_compile_definitions(test_sys_timer_sm PRIVATE reentrant= PLATFORM=0 PLATFORM_C51=1)
add_test(NAME sys_timer_sm COMMAND test_sys_timer_sm)

add_executable(test_bootreason_check
    ${UNIT_KERNEL}/test_bootreason_check.c
    ${ROOT}/tests/unity/unity.c
)
add_test(NAME bootreason_check COMMAND test_bootreason_check)

# --- Test 40: Net SMBus/PMBus helpers and default ops ---
add_executable(test_net_smbus_pmbus
    ${UNIT_NET}/test_net_smbus_pmbus.c
    ${NET}/smbus/xy_smbus.c
    ${NET}/smbus/xy_pmbus.c
    ${TRACE}/xy_log/src/xy_log.c
    ${CLIB}/xy_stdio.c
    ${CLIB}/xy_string.c
    ${CLIB}/xy_ctype.c
    ${UNIT_SUPPORT}/test_clib_alloc_shim.c
    ${ROOT}/tests/unity/unity.c
)
target_link_libraries(test_net_smbus_pmbus m)
add_test(NAME net_smbus_pmbus COMMAND test_net_smbus_pmbus)

# --- Test 41: Actuator framework, relay, servo, PWM, and batch contracts ---
add_executable(test_actuator_framework
    ${UNIT_ACTUATOR}/test_actuator_framework.c
    ${ROOT}/tests/unity/unity.c
    ${ACTUATOR}/xy_actuator.c
)
target_link_libraries(test_actuator_framework m)
add_test(NAME actuator_framework COMMAND test_actuator_framework)

# --- Test 42: BQ25620 charger I2C register and status contracts ---
add_executable(test_charger_bq25620
    ${UNIT_PM}/test_charger_bq25620.c
    ${ROOT}/components/charger/src/xy_bq25620.c
    ${ROOT}/tests/unity/unity.c
)
target_include_directories(test_charger_bq25620 PRIVATE ${ROOT}/components/charger/inc)
add_test(NAME charger_bq25620 COMMAND test_charger_bq25620)

# --- Test 43: Standalone fuel gauge core API contracts ---
add_executable(test_fuel_gauge_core
    ${UNIT_FUEL_GAUGE}/test_fuel_gauge_core.c
    ${ROOT}/components/fuel_gauge/core/fuel_gauge_core.c
    ${ROOT}/components/fuel_gauge/core/xy_fuel_gauge_safety.c
    ${ROOT}/components/fuel_gauge/core/xy_fuel_gauge_security.c
    ${ROOT}/components/fuel_gauge/core/xy_fuel_gauge_status.c
    ${CLIB}/xy_stdio.c
    ${CLIB}/xy_string.c
    ${CLIB}/xy_ctype.c
    ${UNIT_SUPPORT}/test_clib_alloc_shim.c
    ${ROOT}/tests/unity/unity.c
)
target_include_directories(test_fuel_gauge_core PRIVATE ${ROOT}/components/fuel_gauge/inc)
add_test(NAME fuel_gauge_core COMMAND test_fuel_gauge_core)

# --- Test 44: BQ27Z746 fuel gauge driver host guard ---
add_executable(test_fg_bq27z746
    ${UNIT_FUEL_GAUGE}/test_fg_bq27z746.c
    ${ROOT}/components/fuel_gauge/drivers/xy_fg_bq27z746.c
    ${ROOT}/components/fuel_gauge/core/fuel_gauge_core.c
    ${ROOT}/tests/unity/unity.c
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
add_test(NAME fg_bq27z746 COMMAND test_fg_bq27z746)

# --- Test 45: BQ40Z50 fuel gauge driver host guard ---
add_executable(test_fg_bq40z50
    ${UNIT_FUEL_GAUGE}/test_fg_bq40z50.c
    ${ROOT}/components/fuel_gauge/drivers/xy_fg_bq40z50.c
    ${ROOT}/components/fuel_gauge/core/fuel_gauge_core.c
    ${ROOT}/tests/unity/unity.c
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
add_test(NAME fg_bq40z50 COMMAND test_fg_bq40z50)

# --- Test 46: MAX17043 fuel gauge driver host guard ---
add_executable(test_fg_max17043
    ${UNIT_FUEL_GAUGE}/test_fg_max17043.c
    ${ROOT}/components/fuel_gauge/drivers/xy_fg_max17043.c
    ${ROOT}/components/fuel_gauge/core/fuel_gauge_core.c
    ${ROOT}/tests/unity/unity.c
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
add_test(NAME fg_max17043 COMMAND test_fg_max17043)

# --- Test 47: BQ27Z561 fuel gauge driver host guard ---
add_executable(test_fg_bq27z561
    ${UNIT_FUEL_GAUGE}/test_fg_bq27z561.c
    ${ROOT}/components/fuel_gauge/drivers/xy_fg_bq27z561.c
    ${ROOT}/components/fuel_gauge/core/fuel_gauge_core.c
    ${ROOT}/tests/unity/unity.c
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
add_test(NAME fg_bq27z561 COMMAND test_fg_bq27z561)

# --- Test 48: Unity + FFF framework smoke guard ---
xy_add_unit_test(test_fff_smoke fff_smoke UNITY
    ${UNIT_FRAMEWORK}/test_fff_smoke.c
)

# Convenience target: build + run all tests
add_custom_target(run_unit_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS test_device test_sht30_integration test_sensors_multi test_spi_device
            test_osal test_auto_register test_device_async_helper test_can test_lte
            test_nano_modbus test_modbus_legacy test_iso7816 test_iso7816_example test_net_core
            test_crc test_csprng test_random test_encode test_hash test_cipher_hmac test_lwc
            test_sensor_framework test_hal_pc test_pm test_pm_platform_fallback test_trace test_clib test_mux_core
            test_mux_i2c test_mux_gpio test_mux_spi test_mux_uart test_pid_core test_dm_base64 test_dm_tlv test_dm_nvm test_dm_factory test_dm_fee test_dm_corejson test_ipc_pipe test_ipc_broker test_storage_eeprom_24xx test_display_lcd test_display_oled_ws2812 test_display_rgb_matrix test_display_serial_rgb_headers test_display_led_driver test_mqtt_client test_at_client
            test_at_server test_fota_core test_gui_core test_gui_widget_theme
            test_gui_widgets test_analog_devices test_sys_timer_sm test_net_smbus_pmbus
            test_actuator_framework test_charger_bq25620 test_fuel_gauge_core test_fg_bq27z746 test_fg_bq40z50 test_fg_max17043 test_fg_bq27z561
            test_fff_smoke
    COMMENT "Running unit tests"
)
