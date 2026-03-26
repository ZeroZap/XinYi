# XinYi HAL - HC32 Platform CMake Configuration

cmake_minimum_required(VERSION 3.10)

# HC32 Platform Selection
if(NOT DEFINED HC32_SERIES)
    set(HC32_SERIES "HC32L021" CACHE STRING "HC32 MCU Series")
endif()

# Convert to lowercase for directory names
string(TOLOWER ${HC32_SERIES} HC32_SERIES_LOWER)

message(STATUS "HC32 Series: ${HC32_SERIES} (dir: ${HC32_SERIES_LOWER})")

# xhsc Library Root (auto-detect)
if(NOT DEFINED XHYSC_ROOT)
    set(XHYSC_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../../xhsc")
endif()

message(STATUS "xhsc Root: ${XHYSC_ROOT}")

# HC32 Source Files
set(HC32_HAL_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/${HC32_SERIES_LOWER}/xy_hal_gpio_device.c
    ${CMAKE_CURRENT_SOURCE_DIR}/${HC32_SERIES_LOWER}/xy_hal_uart_device.c
    ${CMAKE_CURRENT_SOURCE_DIR}/${HC32_SERIES_LOWER}/xy_hal_spi_device.c
    ${CMAKE_CURRENT_SOURCE_DIR}/${HC32_SERIES_LOWER}/xy_hal_i2c_device.c
    ${CMAKE_CURRENT_SOURCE_DIR}/${HC32_SERIES_LOWER}/xy_hal_adc_device.c
    ${CMAKE_CURRENT_SOURCE_DIR}/${HC32_SERIES_LOWER}/xy_hal_pwm_device.c
    ${CMAKE_CURRENT_SOURCE_DIR}/${HC32_SERIES_LOWER}/xy_hal_timer_device.c
)

# HC32 Header Include Paths
set(HC32_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/../inc                          # XinYi HAL common headers
    ${XHYSC_ROOT}/${HC32_SERIES}/driver/inc                     # xhsc official headers
    ${XHYSC_ROOT}/${HC32_SERIES}/mcu/common                     # xhsc base types
    ${XHYSC_ROOT}/${HC32_SERIES}/mcu                            # xhsc device headers
)

# Verify xhsc library exists
if(NOT EXISTS ${XHYSC_ROOT}/${HC32_SERIES}/driver/inc)
    message(FATAL_ERROR "xhsc library not found at ${XHYSC_ROOT}/${HC32_SERIES}")
endif()

# Create HC32 HAL Library
add_library(hal_hc32 STATIC ${HC32_HAL_SOURCES})

target_include_directories(hal_hc32 PUBLIC
    ${HC32_INCLUDE_DIRS}
)

# Compile Definitions
target_compile_definitions(hal_hc32 PUBLIC
    HC32L021
    __ARM_ARCH=6M
)

# Compiler Flags for ARM Cortex-M0+
target_compile_options(hal_hc32 PUBLIC
    -mcpu=cortex-m0plus
    -mthumb
    -Wall
    -Wextra
    -Wno-unused-parameter
    -std=c99
)
