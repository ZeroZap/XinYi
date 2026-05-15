# STM32U5 Platform Configuration — Cortex-M33 with TrustZone
# Variables only; compiler/linker paths are set in root CMakeLists before project()

set(CPU_FLAGS
    -mcpu=cortex-m33
    -mthumb
    -mfloat-abi=hard
    -mfpu=fpv5-sp-d16
)

set(PLATFORM_DEFINES
    HAL_PLATFORM_STM32
    PLATFORM_STM32U5
    STM32U5
    STM32U5xx
    USE_HAL_DRIVER
    ARM_CORTEX_M33
    FPU
)

set(PLATFORM_INCLUDE_DIRS
    ${CMAKE_SOURCE_DIR}/components/hal/stm32/stm32u5
    ${CMAKE_SOURCE_DIR}/components/hal/stm32/inc
    ${CMAKE_SOURCE_DIR}/MCU/ST/STM32U5/Inc
)

set(PLATFORM_LINKER_FLAGS "-Wl,--gc-sections -Wl,--print-memory-usage")

message(STATUS "Platform: STM32U5 (Cortex-M33 + FPU + TrustZone)")
