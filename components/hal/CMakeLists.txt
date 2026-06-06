# Options: PC, STM32F4, STM32U5, STM32L4, WCH, HC32
set(HAL_PLATFORM "PC" CACHE STRING "HAL Platform (PC, STM32F4, STM32U5, STM32L4, WCH, HC32)")

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
    # No remaining exclusions.  xy_hal_i2s.c is a NOT_SUPPORTED stub on U5
    # — see the file header for the SPI-in-I2S-mode story.
    set(HAL_DEFINES
        HAL_PLATFORM_STM32
        STM32U5
    )
    set(STM32U5_INCLUDE_DIR ${STM32U5_HAL_PATH}/Inc)
    message(STATUS "XY HAL: Building for STM32U5")

elseif(HAL_PLATFORM STREQUAL "STM32L4")
    # STM32L4 implementation currently reuses the STM32F4 wrapper sources
    # until dedicated components/hal/stm32/stm32l4 sources are added.
    file(GLOB HAL_SOURCES
        "stm32/stm32f4/*.c"
        "stm32/src/*.c"
    )
    set(HAL_DEFINES
        HAL_PLATFORM_STM32
        STM32L4
    )
    message(STATUS "XY HAL: Building for STM32L4")

elseif(HAL_PLATFORM STREQUAL "WCH")
    # WCH CH32V — XY HAL wrappers under wch/ch32x/src/ + vendor SDK peripheral
    # drivers from MCU/wch/<chip>/SRC/Peripheral/src/.
    if(NOT DEFINED WCH_CHIP)
        set(WCH_CHIP CH32V30x)
    endif()
    file(GLOB HAL_SOURCES
        "wch/ch32x/src/*.c"
        "${CMAKE_SOURCE_DIR}/MCU/wch/${WCH_CHIP}/SRC/Peripheral/src/*.c"
    )
    set(HAL_DEFINES
        HAL_PLATFORM_WCH
        MCU_CH32
        ${WCH_CHIP}
    )
    message(STATUS "XY HAL: Building for WCH (chip=${WCH_CHIP})")

elseif(HAL_PLATFORM STREQUAL "HC32")
    # HC32 implementation — XY wrappers live under hc32/<chip>/, vendor SDK
    # drivers are compiled directly from MCU/HC32/<CHIP>/driver/src/.
    if(NOT DEFINED HC32_CHIP)
        set(HC32_CHIP HC32L021)
    endif()
    string(TOLOWER "${HC32_CHIP}" _HC32_CHIP_LOWER)
    file(GLOB HAL_SOURCES
        "hc32/${_HC32_CHIP_LOWER}/*.c"
        "${CMAKE_SOURCE_DIR}/MCU/HC32/${HC32_CHIP}/driver/src/*.c"
        "${CMAKE_SOURCE_DIR}/MCU/HC32/${HC32_CHIP}/mcu/common/system_*.c"
    )
    set(HAL_DEFINES
        HAL_PLATFORM_HC32
        ${HC32_CHIP}
    )
    message(STATUS "XY HAL: Building for HC32 (chip=${HC32_CHIP})")

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

