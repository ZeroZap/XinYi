##
## @file CMakeLists.txt
## @brief CMake configuration for XY HAL Component
## @version 2.0
## @date 2026-03-11
##

cmake_minimum_required(VERSION 3.10)
project(xy_hal C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# HAL Platform Selection
# Options: PC, STM32F4, STM32U5, WCH
set(HAL_PLATFORM "PC" CACHE STRING "HAL Platform (PC, STM32F4, STM32U5, WCH)")

# Collect source files based on platform
if(HAL_PLATFORM STREQUAL "PC")
    # PC simulation layer (Linux/POSIX)
    set(HAL_SOURCES
        PC/xy_hal_pc.c
        PC/xy_hal_pin_pc.c
        PC/xy_hal_i2c_pc.c
        PC/xy_hal_spi_pc.c
        PC/xy_hal_uart_pc.c
        stm32/src/xy_hal_flash.c
    )
    set(HAL_DEFINES
        HAL_PLATFORM_PC
        XY_HAL_PC_SIM
    )
    message(STATUS "XY HAL: Building for PC simulation")
    
elseif(HAL_PLATFORM STREQUAL "STM32F4")
    # STM32F4 implementation
    file(GLOB HAL_SOURCES
        "stm32/stm32f4/*.c"
        "stm32/src/*.c"
    )
    set(HAL_DEFINES
        HAL_PLATFORM_STM32
        STM32F4
        STM32_HAL_ENABLED
        USE_HAL_DRIVER
    )
    message(STATUS "XY HAL: Building for STM32F4")
    
elseif(HAL_PLATFORM STREQUAL "STM32U5")
    # STM32U5 implementation (exclude example files)
    file(GLOB HAL_SOURCES
        "stm32/stm32u5/*.c"
        "stm32/src/*.c"
    )
    # Remove example files from build
    list(REMOVE_ITEM HAL_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/stm32/stm32u5/example_usage.c
    )
    set(HAL_DEFINES
        HAL_PLATFORM_STM32
        STM32U5
        STM32_HAL_ENABLED
        USE_HAL_DRIVER
    )
    message(STATUS "XY HAL: Building for STM32U5")
    
elseif(HAL_PLATFORM STREQUAL "WCH")
    # WCH implementation
    file(GLOB HAL_SOURCES
        "wch/*.c"
    )
    set(HAL_DEFINES
        HAL_PLATFORM_WCH
        WCH_HAL_ENABLED
    )
    message(STATUS "XY HAL: Building for WCH")
    
else()
    message(FATAL_ERROR "Unknown HAL_PLATFORM: ${HAL_PLATFORM}")
endif()

# Header files
set(HAL_HEADERS
    inc/xy_hal.h
    inc/xy_hal_pin.h
    inc/xy_hal_uart.h
    inc/xy_hal_spi.h
    inc/xy_hal_i2c.h
    inc/xy_hal_timer.h
    inc/xy_hal_pwm.h
    inc/xy_hal_rtc.h
    inc/xy_hal_dma.h
    inc/xy_hal_lp_timer.h
    inc/xy_hal_flash.h
    inc/xy_hal_adc.h
    inc/xy_hal_gpio.h
)

# Create library
add_library(xy_hal STATIC ${HAL_SOURCES})

# Include directories
target_include_directories(xy_hal PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32
    ${CMAKE_CURRENT_SOURCE_DIR}/../trace/xy_log/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../clib/xy_clib
)

# Compile definitions
target_compile_definitions(xy_hal PUBLIC ${HAL_DEFINES})

# Installation
install(TARGETS xy_hal
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

install(FILES ${HAL_HEADERS}
    DESTINATION include/xy_hal
)

# Export platform info
message(STATUS "XY HAL Sources: ${HAL_SOURCES}")
