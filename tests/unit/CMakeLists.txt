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
    ${DEV}/xy_device.c              # i2c/spi/uart/gpio helpers + register API
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

enable_testing()
add_test(NAME device_framework  COMMAND test_device)
add_test(NAME sht30_integration COMMAND test_sht30_integration)
add_test(NAME sensors_multi     COMMAND test_sensors_multi)
add_test(NAME spi_device        COMMAND test_spi_device)

# Convenience target: build + run all tests
add_custom_target(run_unit_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS test_device test_sht30_integration test_sensors_multi test_spi_device
    COMMENT "Running unit tests"
)
