# STM32L4 Platform Configuration — Cortex-M4 with FPU
# Variables only; compiler/linker paths are set in root CMakeLists before project()

set(CPU_FLAGS
    -mcpu=cortex-m4
    -mthumb
    -mfloat-abi=hard
    -mfpu=fpv4-sp-d16
)

# Default chip; override with -DSTM32L4_CHIP=STM32L476xx etc.
if(NOT DEFINED STM32L4_CHIP)
    set(STM32L4_CHIP STM32L476xx)
endif()

set(PLATFORM_DEFINES
    HAL_PLATFORM_STM32
    PLATFORM_STM32L4
    STM32L4
    STM32L4xx
    ${STM32L4_CHIP}
    USE_HAL_DRIVER
    ARM_CORTEX_M4
)

# Include dirs: STM32L4 application HAL configuration/shims first, then the
# shared STM32 wrapper headers and SDK headers from the uniform
# STM32L4/STM32CubeL4 path.
set(PLATFORM_INCLUDE_DIRS
    ${CMAKE_SOURCE_DIR}/components/hal/stm32/stm32l4
    ${CMAKE_SOURCE_DIR}/components/hal/stm32/inc
    ${CMAKE_SOURCE_DIR}/MCU/ST/STM32L4/STM32CubeL4/Drivers/STM32L4xx_HAL_Driver/Inc
    ${CMAKE_SOURCE_DIR}/MCU/ST/STM32L4/STM32CubeL4/Drivers/CMSIS/Device/ST/STM32L4xx/Include
    ${CMAKE_SOURCE_DIR}/MCU/ST/STM32L4/STM32CubeL4/Drivers/CMSIS/Include
)

set(PLATFORM_LINKER_FLAGS "-Wl,--gc-sections -Wl,--print-memory-usage -nostartfiles")

message(STATUS "Platform: STM32L4 (Cortex-M4F, chip=${STM32L4_CHIP})")
