# Options: PC, STM32F4, STM32U5, STM32L4, WCH, HC32
set(HAL_PLATFORM "PC" CACHE STRING "HAL Platform (PC, STM32F4, STM32U5, STM32L4, WCH, HC32)")

# Collect source files based on platform
if(HAL_PLATFORM STREQUAL "PC")
    # PC simulation layer (Linux/POSIX)
    set(HAL_SOURCES
        PC/xy_hal_pc.c
        PC/xy_hal_gpio_pc.c
        PC/xy_hal_i2c_pc.c
        PC/xy_hal_irq_pc.c
        PC/xy_hal_rtc_pc.c
        PC/xy_hal_spi_pc.c
        PC/xy_hal_uart_pc.c
    )
    set(HAL_DEFINES
        HAL_PLATFORM_PC
        XY_HAL_PC_SIM
    )
    message(STATUS "XY HAL: Building for PC simulation")
    
elseif(HAL_PLATFORM STREQUAL "STM32F4")
    # STM32F4 implementation.
    #
    # Every stm32f4 wrapper is wrapped in `#ifdef STM32_HAL_ENABLED`; without
    # that define each translation unit compiles to nothing and this platform
    # gate passes while the xy_hal archive exports no HAL symbols at all.  The
    # define is mandatory here and guarded by
    # tests/unit/hal_component/check_hal_platform_build_contract.py.
    file(GLOB HAL_SOURCES
        "stm32/stm32f4/*.c"
        "stm32/src/*.c"
    )
    # Wrappers whose signatures still drift from the current public HAL
    # contracts.  They are excluded explicitly (never silently) until the
    # migration slice lands; re-adding one without fixing it fails the build.
    set(XY_HAL_F4_DEFERRED_SOURCES
        xy_hal_rtc.c
        xy_hal_spi.c
        xy_hal_timer.c)
    # Literal filters are authoritative (the build-contract guard parses them),
    # so a wrapper can never be "listed as deferred" without being excluded.
    list(FILTER HAL_SOURCES EXCLUDE REGEX "/xy_hal_rtc\.c$")
    list(FILTER HAL_SOURCES EXCLUDE REGEX "/xy_hal_spi\.c$")
    list(FILTER HAL_SOURCES EXCLUDE REGEX "/xy_hal_timer\.c$")
    set(HAL_DEFINES
        HAL_PLATFORM_STM32
        STM32F4
        STM32_HAL_ENABLED
    )
    message(STATUS "XY HAL: Building for STM32F4")
    message(STATUS "XY HAL: STM32F4 contract-deferred wrappers: ${XY_HAL_F4_DEFERRED_SOURCES}")

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
    # STM32L4 reuses F4 wrappers except where a dedicated L4 source exists.
    #
    # STM32_HAL_ENABLED is mandatory: the reused stm32f4 wrappers (adc/dac/exti/
    # lp_timer/pwm/rng) are `#ifdef STM32_HAL_ENABLED` guarded and would
    # otherwise compile to empty objects, leaving this — the reference-board
    # platform — with an archive that silently omits those peripherals.
    file(GLOB HAL_SOURCES
        "stm32/stm32f4/*.c"
        "stm32/src/*.c"
    )
    list(FILTER HAL_SOURCES EXCLUDE REGEX "/xy_hal_dma\\.c$")
    list(FILTER HAL_SOURCES EXCLUDE REGEX "/xy_hal_flash\\.c$")
    list(FILTER HAL_SOURCES EXCLUDE REGEX "/xy_hal_gpio\\.c$")
    list(FILTER HAL_SOURCES EXCLUDE REGEX "/xy_hal_i2c\\.c$")
    list(FILTER HAL_SOURCES EXCLUDE REGEX "/xy_hal_rtc\\.c$")
    list(FILTER HAL_SOURCES EXCLUDE REGEX "/xy_hal_spi\\.c$")
    list(FILTER HAL_SOURCES EXCLUDE REGEX "/xy_hal_sys\\.c$")
    list(FILTER HAL_SOURCES EXCLUDE REGEX "/xy_hal_timer\\.c$")
    list(FILTER HAL_SOURCES EXCLUDE REGEX "/xy_hal_uart\\.c$")
    list(FILTER HAL_SOURCES EXCLUDE REGEX "/xy_hal_wdg\\.c$")
    list(FILTER HAL_SOURCES EXCLUDE REGEX "/xy_hal_adc\\.c$")
    list(APPEND HAL_SOURCES
        "stm32/stm32l4/xy_hal_delay.c"
        "stm32/stm32l4/xy_hal_dma.c"
        "stm32/stm32l4/xy_hal_flash.c"
        "stm32/stm32l4/xy_hal_gpio.c"
        "stm32/stm32l4/xy_hal_i2c.c"
        "stm32/stm32l4/xy_hal_qspi.c"
        "stm32/stm32l4/xy_hal_rtc.c"
        "stm32/stm32l4/xy_hal_spi.c"
        "stm32/stm32l4/xy_hal_sys.c"
        "stm32/stm32l4/xy_hal_timer.c"
        "stm32/stm32l4/xy_hal_uart.c"
        "stm32/stm32l4/xy_hal_wdg.c")
    # Sources this branch deliberately does not build (family mismatch, see
    # backlog S6-108):
    #   stm32/stm32f4/xy_hal_adc.c   — F4 ADC->CCR / sample-time contract is F4-only
    #
    # xy_hal_qspi.c was previously recorded here as "signature drift".  That
    # diagnosis was wrong: the wrapper is contract-correct and the only reason
    # it failed to compile was that this platform's own
    # stm32l4/stm32l4xx_hal_conf.h never enabled HAL_QSPI_MODULE_ENABLED, so
    # QSPI_HandleTypeDef/QSPI_* were undeclared.  The reference board's external
    # flash (W25Q128) is QSPI-attached, so the wrapper is built here now and the
    # module enable is guarded by
    # tests/unit/hal_component/check_hal_platform_build_contract.py.
    set(HAL_DEFINES
        HAL_PLATFORM_STM32
        STM32L4
        STM32_HAL_ENABLED
    )
    message(STATUS "XY HAL: Building for STM32L4")

elseif(HAL_PLATFORM STREQUAL "WCH")
    # WCH CH32V — XY HAL wrappers under wch/ch32x/src/ + vendor SDK peripheral
    # drivers from MCU/wch/<chip>/SRC/Peripheral/src/.
    if(NOT DEFINED WCH_CHIP)
        set(WCH_CHIP CH32V30x)
    endif()
    # Keep the first CH32V307 board slice limited to the UART path.  The older
    # generic WCH wrappers for ADC/CAN/I2C/PWM/SPI do not match the current HAL
    # contracts and must not block the board bring-up build.
    set(HAL_SOURCES
        "wch/ch32x/src/xy_hal_adc.c"
        "wch/ch32x/src/xy_hal_can.c"
        "wch/ch32x/src/xy_hal_dac.c"
        "wch/ch32x/src/xy_hal_delay.c"
        "wch/ch32x/src/xy_hal_dma.c"
        "wch/ch32x/src/xy_hal_exti.c"
        "wch/ch32x/src/xy_hal_flash.c"
        "wch/ch32x/src/xy_hal_gpio.c"
        "wch/ch32x/src/xy_hal_i2c.c"
        "wch/ch32x/src/xy_hal_i2s.c"
        "wch/ch32x/src/xy_hal_pwm.c"
        "wch/ch32x/src/xy_hal_rng.c"
        "wch/ch32x/src/xy_hal_rtc.c"
        "wch/ch32x/src/xy_hal_spi.c"
        "wch/ch32x/src/xy_hal_sys.c"
        "wch/ch32x/src/xy_hal_timer.c"
        "wch/ch32x/src/xy_hal_uart.c"
        "wch/ch32x/src/xy_hal_wdg.c"
        "${WCH_SDK_DIR}/Peripheral/src/ch32v30x_rcc.c"
        "${WCH_SDK_DIR}/Peripheral/src/ch32v30x_usart.c"
        "${WCH_SDK_DIR}/Peripheral/src/ch32v30x_gpio.c"
        "${WCH_SDK_DIR}/Peripheral/src/ch32v30x_adc.c"
        "${WCH_SDK_DIR}/Peripheral/src/ch32v30x_can.c"
        "${WCH_SDK_DIR}/Peripheral/src/ch32v30x_dbgmcu.c"
        "${WCH_SDK_DIR}/Peripheral/src/ch32v30x_dma.c"
        "${WCH_SDK_DIR}/Peripheral/src/ch32v30x_flash.c"
        "${WCH_SDK_DIR}/Peripheral/src/ch32v30x_i2c.c"
        "${WCH_SDK_DIR}/Peripheral/src/ch32v30x_rng.c"
        "${WCH_SDK_DIR}/Peripheral/src/ch32v30x_rtc.c"
        "${WCH_SDK_DIR}/Peripheral/src/ch32v30x_bkp.c"
        "${WCH_SDK_DIR}/Peripheral/src/ch32v30x_pwr.c"
        "${WCH_SDK_DIR}/Peripheral/src/ch32v30x_spi.c"
        "${WCH_SDK_DIR}/Peripheral/src/ch32v30x_tim.c"
        "${WCH_SDK_DIR}/Peripheral/src/ch32v30x_iwdg.c"
        "${WCH_SDK_DIR}/Peripheral/src/ch32v30x_wwdg.c"
        "${WCH_SYSTEM_DIR}/system_ch32v30x.c"
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

