# Options: PC, STM32F4, STM32U5, WCH
set(HAL_PLATFORM "PC" CACHE STRING "HAL Platform (PC, STM32F4, STM32U5, WCH)")

# Collect source files based on platform
if(HAL_PLATFORM STREQUAL "PC")
    # PC simulation layer (Linux/POSIX)
    set(HAL_SOURCES
        PC/xy_hal_pc.c
        PC/xy_hal_gpio_pc.c
        PC/xy_hal_i2c_pc.c
        PC/xy_hal_spi_pc.c
        PC/xy_hal_uart_pc.c
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
    )
    message(STATUS "XY HAL: Building for STM32F4")

elseif(HAL_PLATFORM STREQUAL "STM32U5")
    # STM32U5 implementation
    file(GLOB HAL_SOURCES
        "stm32/stm32u5/*.c"
        "stm32/src/*.c"
    )
    # Exclude example_usage.c (outdated examples).
    list(REMOVE_ITEM HAL_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/stm32/stm32u5/example_usage.c")
    # Pre-existing STM32U5 HAL impls with macro/signature drift against the
    # in-tree minimal SDK headers. Excluded so the platform can at least
    # build the unified API (xy_hal_gpio.c). Fix these as needed.
    foreach(_drifted
        xy_hal_adc.c xy_hal_can.c xy_hal_dac.c xy_hal_dma.c xy_hal_exti.c
        xy_hal_flash.c xy_hal_i2c.c xy_hal_i2s.c xy_hal_lp_timer.c
        xy_hal_pwm.c xy_hal_rng.c xy_hal_rtc.c xy_hal_spi.c xy_hal_timer.c
        xy_hal_uart.c xy_hal_wdg.c)
        list(REMOVE_ITEM HAL_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/stm32/stm32u5/${_drifted}")
    endforeach()
    set(HAL_DEFINES
        HAL_PLATFORM_STM32
        STM32U5
    )
    set(STM32U5_INCLUDE_DIR ${STM32U5_HAL_PATH}/Inc)
    message(STATUS "XY HAL: Building for STM32U5")

elseif(HAL_PLATFORM STREQUAL "WCH")
    # WCH CH32 implementation
    file(GLOB HAL_SOURCES
        "wch/*.c"
    )
    set(HAL_DEFINES
        HAL_PLATFORM_WCH
        WCH
    )
    message(STATUS "XY HAL: Building for WCH")

else()
    message(FATAL_ERROR "Unsupported HAL_PLATFORM: ${HAL_PLATFORM}")
endif()

# Create HAL library
add_library(xy_hal ${HAL_SOURCES})

# Include directories
target_include_directories(xy_hal PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>
    $<INSTALL_INTERFACE:include>
)

# Compile definitions
target_compile_definitions(xy_hal PRIVATE ${HAL_DEFINES})

# Add STM32U5 HAL include directory if path is provided
if(DEFINED STM32U5_HAL_PATH)
    target_include_directories(xy_hal PRIVATE ${STM32U5_HAL_PATH}/Inc)
endif()

# Add HC32L021 HAL support
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/hc32l021/CMakeLists.txt)
endif()