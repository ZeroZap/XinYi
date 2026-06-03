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
    ${NET}/xy_iso7816               # xy_iso7816.h
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
    ${PM}/fuel-gauge                # xy_fuel_gauge.h
    ${MUX}/inc                      # xy_mux*.h
    ${PID}                          # xy_pid.h
    ${PID}/inc                      # xy_pid_auto.h
    ${OSAL}/inc                     # xy_os_tick.h
    ${DM}/xy_base64                 # xy_base64.h
    ${IPC}/pipe                     # xy_pipe.h
    ${SENSOR}                       # sensor_core.h / sensor_type.h
    ${ROOT}/UniTest/unity           # unity.h
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

# --- Test 1: framework end-to-end ---
add_executable(test_device test_device.c ${FRAMEWORK_SOURCES})

# --- Test 2: SHT30 driver integration ---
add_executable(test_sht30_integration
    test_sht30_integration.c
    ${DRV}/sensor/temperature/sht30/xy_sht30.c
    ${FRAMEWORK_SOURCES}
)

# --- Test 3: heterogeneous sensor coexistence ---
add_executable(test_sensors_multi
    test_sensors_multi.c
    ${DRV}/sensor/temperature/sht30/xy_sht30.c
    ${DRV}/sensor/motion/mpu6050/xy_mpu6050.c
    ${DRV}/sensor/pressure/bmp280/xy_bmp280.c
    ${DRV}/sensor/adc/ads1115/xy_ads1115.c
    ${FRAMEWORK_SOURCES}
)

# --- Test 4: SPI device helper + mixed-bus registry ---
add_executable(test_spi_device test_spi_device.c ${FRAMEWORK_SOURCES})

# --- Test 6: XY_DEVICE_REGISTER auto-registration via GCC constructors ---
add_executable(test_auto_register test_auto_register.c ${FRAMEWORK_SOURCES})

# --- Test 8: device registry example stays aligned with public APIs ---
add_executable(test_device_registry_example
    ${ROOT}/examples/device_registry_example.c
    ${FRAMEWORK_SOURCES}
)

# --- Test 9: device driver template compiles and runs against public APIs ---
add_executable(test_device_driver_template
    ${ROOT}/examples/device_driver_template.c
    ${FRAMEWORK_SOURCES}
)

# --- Test 10: CAN protocol component API + callback/fifo behavior ---
add_executable(test_can
    test_can.c
    ${NET}/src/xy_can.c
    ${CLIB}/xy_stdio.c
)

# --- Test 11: LTE component lifecycle, callback storage, and validation ---
add_executable(test_lte
    test_lte.c
    ${NET}/src/xy_lte.c
)

# --- Test 12: nano_modbus compatibility API over mb_tiny ---
add_executable(test_nano_modbus
    test_nano_modbus.c
    ${NET}/modbus_tiny/mb_tiny.c
)

# --- Test 13: ISO7816 public include, lifecycle, and APDU behavior ---
add_executable(test_iso7816
    test_iso7816.c
    ${NET}/xy_iso7816/xy_iso7816.c
)

# --- Test 14: ISO7816 usage example stays aligned with public APIs ---
add_executable(test_iso7816_example
    ${NET}/xy_iso7816/xy_iso7816_example.c
    ${NET}/xy_iso7816/xy_iso7816.c
    test_iso7816_example_main.c
)

# --- Test 15: Net umbrella/core lifecycle and platform helpers ---
add_executable(test_net_core
    test_net_core.c
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

# --- Test 16: CRC core and common variants ---
add_executable(test_crc
    test_crc.c
    ${CRYPTO}/xy_crc/xy_crc.c
)

# --- Test 5: OSAL baremetal primitives (mutex/sem/event/queue/pool) ---
set(OSAL_ROOT ${ROOT}/components/kernel)

# --- Test 7: optional xy_device_async_*_ex helper API ---
add_executable(test_device_async_helper
    test_device_async_helper.c
    ${DEV}/src/xy_device_async.c
)
target_include_directories(test_device_async_helper PRIVATE
    ${OSAL_ROOT}/osal
    ${OSAL_ROOT}/osal/inc
)

add_executable(test_osal
    test_osal.c
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
add_test(NAME can_component COMMAND test_can)
add_test(NAME lte_component COMMAND test_lte)
add_test(NAME nano_modbus_compat COMMAND test_nano_modbus)
add_test(NAME iso7816_component COMMAND test_iso7816)
add_test(NAME iso7816_example COMMAND test_iso7816_example)
add_test(NAME net_core COMMAND test_net_core)
add_test(NAME crypto_crc COMMAND test_crc)

# --- Test 16: Crypto CSPRNG lifecycle, buffering, and helpers ---
add_executable(test_csprng
    test_csprng.c
    ${CRYPTO}/xy_rng/xy_csprng.c
    ${CLIB}/xy_stdio.c
)
add_test(NAME crypto_csprng COMMAND test_csprng)

# --- Test 17: Crypto simple RNG validation and output shape ---
add_executable(test_random
    test_random.c
    ${CRYPTO}/xy_rng/xy_random.c
)
add_test(NAME crypto_random COMMAND test_random)

# --- Test 18: Crypto Base64 and Hex validation/roundtrip behavior ---
add_executable(test_encode
    test_encode.c
    ${CRYPTO}/xy_base/xy_base64.c
    ${CRYPTO}/xy_hex/xy_hex.c
)
target_include_directories(test_encode PRIVATE
    ${CRYPTO}
    ${CRYPTO}/inc
)
add_test(NAME crypto_encode COMMAND test_encode)

# --- Test 19: Crypto MD5/SHA-256 vectors and incremental hashing ---
add_executable(test_hash
    test_hash.c
    ${CRYPTO}/xy_md/xy_md5.c
    ${CRYPTO}/xy_hmac/xy_sha256.c
)
target_include_directories(test_hash PRIVATE
    ${CRYPTO}
    ${CRYPTO}/inc
)
add_test(NAME crypto_hash COMMAND test_hash)

# --- Test 20: Crypto AES/HMAC/SM3/SM4/ChaCha20 coverage ---
add_executable(test_cipher_hmac
    test_cipher_hmac.c
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
add_test(NAME crypto_cipher_hmac COMMAND test_cipher_hmac)

# --- Test 21: Sensor framework lifecycle, registry, FIFO, calibration, filters ---
add_executable(test_sensor_framework
    test_sensor_framework.c
    ${SENSOR}/sensor_core.c
    ${SENSOR}/sensor_fifo.c
    ${SENSOR}/sensor_calibration.c
    ${SENSOR}/sensor_filter.c
    ${SENSOR}/sensor_threshold.c
    ${ROOT}/UniTest/unity/unity.c
)
add_test(NAME sensor_framework COMMAND test_sensor_framework)

# --- Test 22: HAL PC simulation contracts for GPIO/UART/I2C/SPI/SYS ---
add_executable(test_hal_pc
    test_hal_pc.c
    ${HAL}/PC/xy_hal_pc.c
    ${HAL}/PC/xy_hal_gpio_pc.c
    ${HAL}/PC/xy_hal_i2c_pc.c
    ${HAL}/PC/xy_hal_spi_pc.c
    ${HAL}/PC/xy_hal_uart_pc.c
    ${ROOT}/UniTest/unity/unity.c
)
target_include_directories(test_hal_pc PRIVATE
    ${HAL}/inc
    ${HAL}/PC
)
add_test(NAME hal_pc COMMAND test_hal_pc)

# --- Test 23: PM lifecycle, charger, fuel gauge, and PC platform contracts ---
add_executable(test_pm
    test_pm.c
    ${PM}/src/xy_pm_system.c
    ${PM}/src/xy_pm_adc.c
    ${PM}/src/xy_pm_platform.c
    ${PM}/src/xy_charger.c
    ${PM}/src/xy_fuel_gauge.c
    ${CLIB}/xy_stdio.c
    ${CLIB}/xy_string.c
    ${CLIB}/xy_ctype.c
    test_clib_alloc_shim.c
)
target_compile_definitions(test_pm PRIVATE PLATFORM_PC XY_PM_PLATFORM_OWNS_CHARGER_HW)
add_test(NAME pm_component COMMAND test_pm)

# --- Test 24: Trace logging output and dynamic-level contracts ---
add_executable(test_trace
    test_trace.c
    ${TRACE}/xy_log/src/xy_log.c
    ${CLIB}/xy_stdio.c
    ${CLIB}/xy_string.c
    ${CLIB}/xy_ctype.c
    test_clib_alloc_shim.c
)
target_compile_definitions(test_trace PRIVATE XY_LOG_LEVEL=XY_LOG_LEVEL_DEBUG)
add_test(NAME trace_component COMMAND test_trace)

# --- Test 25: CLIB common/string/stdio/ring-buffer contracts ---
add_executable(test_clib
    test_clib.c
    ${CLIB}/xy_common.c
    ${CLIB}/xy_string.c
    ${CLIB}/xy_stdio.c
    ${CLIB}/xy_ctype.c
    ${CLIB}/xy_rb.c
    test_clib_alloc_shim.c
)
add_test(NAME clib_component COMMAND test_clib)

# --- Test 26: MUX manager, registration, and TLV packet contracts ---
add_executable(test_mux_core
    ${ROOT}/tests/mux_test/test_mux_core.c
    ${MUX}/src/xy_mux.c
    ${MUX}/src/xy_mux_gpio.c
    ${MUX}/src/xy_mux_i2c.c
    ${MUX}/src/xy_mux_spi.c
    ${MUX}/src/xy_mux_uart.c
    ${TRACE}/xy_log/src/xy_log.c
    ${CLIB}/xy_stdio.c
    ${CLIB}/xy_string.c
    ${CLIB}/xy_ctype.c
    test_clib_alloc_shim.c
)
add_test(NAME mux_core COMMAND test_mux_core)

# --- Test 27: PID core lifecycle, tuning, limits, and compute contracts ---
add_executable(test_pid_core
    test_pid_core.c
    ${PID}/xy_pid.c
    ${TRACE}/xy_log/src/xy_log.c
    ${CLIB}/xy_stdio.c
)
target_link_libraries(test_pid_core m)
add_test(NAME pid_core COMMAND test_pid_core)

# --- Test 28: DM base64 public vectors and binary roundtrip ---
add_executable(test_dm_base64
    test_dm_base64.c
    ${DM}/xy_base64/xy_base64.c
)
target_include_directories(test_dm_base64 PRIVATE ${ROOT}/components)
add_test(NAME dm_base64 COMMAND test_dm_base64)

# --- Test 29: IPC pipe ring-buffer contracts ---
add_executable(test_ipc_pipe
    test_ipc_pipe.c
    ${IPC}/pipe/xy_pipe.c
)
add_test(NAME ipc_pipe COMMAND test_ipc_pipe)

# Convenience target: build + run all tests
add_custom_target(run_unit_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS test_device test_sht30_integration test_sensors_multi test_spi_device
            test_osal test_auto_register test_device_async_helper test_can test_lte
            test_nano_modbus test_iso7816 test_iso7816_example test_net_core
            test_crc test_csprng test_random test_encode test_hash test_cipher_hmac
            test_sensor_framework test_hal_pc test_pm test_trace test_clib test_mux_core
            test_pid_core test_dm_base64 test_ipc_pipe
    COMMENT "Running unit tests"
)
