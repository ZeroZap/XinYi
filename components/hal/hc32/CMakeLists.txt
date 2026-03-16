# XinYi HAL - HC32 Platform CMake Configuration

cmake_minimum_required(VERSION 3.10)

# HC32 Platform Selection
if(NOT DEFINED HC32_SERIES)
    set(HC32_SERIES "HC32L021" CACHE STRING "HC32 MCU Series")
endif()

message(STATUS "HC32 Series: ${HC32_SERIES}")

# HC32 Source Files
set(HC32_HAL_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/${HC32_SERIES}/xy_hal_gpio_device.c
    # ${CMAKE_CURRENT_SOURCE_DIR}/${HC32_SERIES}/xy_hal_uart_device.c
    # ${CMAKE_CURRENT_SOURCE_DIR}/${HC32_SERIES}/xy_hal_spi_device.c
    # ${CMAKE_CURRENT_SOURCE_DIR}/${HC32_SERIES}/xy_hal_i2c_device.c
)

set(HC32_HAL_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
    ${XHYSC_ROOT}/HC32L021/driver/inc
)

# Create HC32 HAL Library
add_library(hal_hc32 STATIC ${HC32_HAL_SOURCES})

target_include_directories(hal_hc32 PUBLIC
    ${HC32_HAL_HEADERS}
    ${CMAKE_CURRENT_SOURCE_DIR}/../inc
)

# Link xhsc driver library
if(EXISTS ${XHYSC_ROOT})
    target_include_directories(hal_hc32 PUBLIC ${XHYSC_ROOT}/HC32L021/driver/inc)
endif()

# Compile Definitions
target_compile_definitions(hal_hc32 PUBLIC
    HC32L021
    __ARM_ARCH=6
)

# Compiler Flags
target_compile_options(hal_hc32 PUBLIC
    -mcpu=cortex-m0plus
    -mthumb
    -Wall
    -Wextra
)
