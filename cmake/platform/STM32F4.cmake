# STM32F4 Platform Configuration — Cortex-M4 with FPU
# Variables only; compiler/linker paths are set in root CMakeLists before project()

set(CPU_FLAGS
    -mcpu=cortex-m4
    -mthumb
    -mfloat-abi=hard
    -mfpu=fpv4-sp-d16
)

# Default chip; override with -DSTM32F4_CHIP=STM32F405xx etc.
if(NOT DEFINED STM32F4_CHIP)
    set(STM32F4_CHIP STM32F407xx)
endif()

set(PLATFORM_DEFINES
    HAL_PLATFORM_STM32
    PLATFORM_STM32F4
    STM32F4
    STM32F4xx
    ${STM32F4_CHIP}
    USE_HAL_DRIVER
    ARM_CORTEX_M4
    FPU
)

# Include dirs: XY HAL impl dir (provides stm32_hal.h shim) + SDK headers
set(PLATFORM_INCLUDE_DIRS
    ${CMAKE_SOURCE_DIR}/components/hal/stm32/stm32f4
    ${CMAKE_SOURCE_DIR}/components/hal/stm32/inc
    ${CMAKE_SOURCE_DIR}/MCU/ST/STM32F4/Drivers/STM32F4xx_HAL_Driver/Inc
    ${CMAKE_SOURCE_DIR}/MCU/ST/STM32F4/Drivers/CMSIS/Device/ST/STM32F4xx/Include
    ${CMAKE_SOURCE_DIR}/MCU/ST/STM32F4/Drivers/CMSIS/Include
)

set(PLATFORM_LINKER_FLAGS "-Wl,--gc-sections -Wl,--print-memory-usage -nostartfiles")

message(STATUS "Platform: STM32F4 (Cortex-M4F, chip=${STM32F4_CHIP})")
