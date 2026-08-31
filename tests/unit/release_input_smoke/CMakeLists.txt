cmake_minimum_required(VERSION 3.16)
project(xinyi_release_input_smoke C)

enable_testing()

set(ROOT "${CMAKE_CURRENT_LIST_DIR}/../../..")
set(DEVICE "${ROOT}/components/device")
set(HAL "${ROOT}/components/hal")

add_executable(test_device_driver_template
    "${ROOT}/examples/device_driver_template.c"
    "${ROOT}/tests/unity/unity.c"
    "${DEVICE}/src/xy_device.c"
    "${DEVICE}/src/xy_device_pm.c"
    "${DEVICE}/src/xy_device_bus_helpers.c"
    "${DEVICE}/xy_device_core.c"
    "${HAL}/PC/xy_hal_pc.c"
    "${HAL}/PC/xy_hal_gpio_pc.c"
    "${HAL}/PC/xy_hal_i2c_pc.c"
    "${HAL}/PC/xy_hal_spi_pc.c"
    "${HAL}/PC/xy_hal_uart_pc.c"
)

target_compile_features(test_device_driver_template PRIVATE c_std_99)
target_compile_definitions(test_device_driver_template PRIVATE HAL_PLATFORM_PC)
target_include_directories(test_device_driver_template PRIVATE
    "${DEVICE}/inc"
    "${DEVICE}"
    "${HAL}/inc"
    "${HAL}/PC"
    "${ROOT}/components/common/inc"
    "${ROOT}/components/clib/xy_clib"
    "${ROOT}/tests/unity"
)

add_test(NAME device_driver_template COMMAND test_device_driver_template)
