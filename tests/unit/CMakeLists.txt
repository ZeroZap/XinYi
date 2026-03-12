cmake_minimum_required(VERSION 3.10)
project(unit_tests C)
set(CMAKE_C_STANDARD 99)

# PC simulation mode
add_definitions(-DHAL_PLATFORM_PC -DXY_HAL_PC_SIM)

# Enable coverage
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --coverage -g -O0")

include_directories(
    ${CMAKE_SOURCE_DIR}/../../components/device
    ${CMAKE_SOURCE_DIR}/../../components/device/inc
    ${CMAKE_SOURCE_DIR}/../../components/hal/inc
    ${CMAKE_SOURCE_DIR}/../../components/hal
)

# Device tests
add_executable(test_device
    test_device.c
    ${CMAKE_SOURCE_DIR}/../../components/device/xy_device.c
    ${CMAKE_SOURCE_DIR}/../../components/device/xy_device_core.c
    ${CMAKE_SOURCE_DIR}/../../components/hal/PC/xy_hal_pc.c
    ${CMAKE_SOURCE_DIR}/../../components/hal/PC/xy_hal_pin_pc.c
    ${CMAKE_SOURCE_DIR}/../../components/hal/PC/xy_hal_i2c_pc.c
    ${CMAKE_SOURCE_DIR}/../../components/hal/PC/xy_hal_spi_pc.c
    ${CMAKE_SOURCE_DIR}/../../components/hal/PC/xy_hal_uart_pc.c
)

# Add test target
add_custom_target(run_tests
    COMMAND ${CMAKE_BINARY_DIR}/test_device
    DEPENDS test_device
    COMMENT "Running unit tests"
)
