# STM32U5 Platform Configuration — Cortex-M33 with TrustZone
# Variables only; compiler/linker paths are set in root CMakeLists before project()

set(CPU_FLAGS
    -mcpu=cortex-m33
    -mthumb
    -mfloat-abi=hard
    -mfpu=fpv5-sp-d16
)

# Default chip; override with -DSTM32U5_CHIP=STM32U585xx etc.
if(NOT DEFINED STM32U5_CHIP)
    set(STM32U5_CHIP STM32U575xx)
endif()

set(PLATFORM_DEFINES
    HAL_PLATFORM_STM32
    PLATFORM_STM32U5
    STM32U5
    STM32U5xx
    ${STM32U5_CHIP}
    USE_HAL_DRIVER
    ARM_CORTEX_M33
    FPU
)

# Include order matters: our customized stm32u5xx_hal_conf.h (in stm32u5/)
# must be found before any conf template that ships with the HAL driver.
set(PLATFORM_INCLUDE_DIRS
    ${CMAKE_SOURCE_DIR}/components/hal/stm32/stm32u5
    ${CMAKE_SOURCE_DIR}/components/hal/stm32/inc
    ${CMAKE_SOURCE_DIR}/MCU/ST/STM32U5/Inc            # stm32u5xx-hal-driver
    ${CMAKE_SOURCE_DIR}/MCU/ST/cmsis_device_u5/Include # CMSIS device (stm32u575xx.h etc.)
    ${CMAKE_SOURCE_DIR}/MCU/CMSIS/Include              # CMSIS-Core (core_cm33.h, cmsis_gcc.h ...)
)

set(PLATFORM_LINKER_FLAGS "-Wl,--gc-sections -Wl,--print-memory-usage")

message(STATUS "Platform: STM32U5 (Cortex-M33 + FPU + TrustZone)")
