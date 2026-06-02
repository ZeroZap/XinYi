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

include_directories(
    ${DEV}                          # xy_device_core.h
    ${DEV}/inc                      # xy_device.h
    ${HAL}/inc                      # xy_hal_*.h
    ${HAL}/PC                       # xy_hal_pc.h
    ${DRV}/sensor/temperature/sht30 # xy_sht30.h
    ${DRV}/sensor/motion/mpu6050    # xy_mpu6050.h
    ${DRV}/sensor/pressure/bmp280   # xy_bmp280.h
    ${DRV}/sensor/adc/ads1115       # xy_ads1115.h
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

# Convenience target: build + run all tests
add_custom_target(run_unit_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS test_device test_sht30_integration test_sensors_multi test_spi_device
            test_osal test_auto_register test_device_async_helper
    COMMENT "Running unit tests"
)
