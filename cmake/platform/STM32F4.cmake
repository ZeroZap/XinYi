# STM32F4 Platform Configuration
# ARM Cortex-M4 with FPU

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(CMAKE_C_COMPILER   arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)
set(CMAKE_OBJCOPY      arm-none-eabi-objcopy)
set(CMAKE_SIZE         arm-none-eabi-size)

# CPU flags for STM32F4 (Cortex-M4F)
set(CPU_FLAGS
    -mcpu=cortex-m4
    -mthumb
    -mfloat-abi=hard
    -mfpu=fpv4-sp-d16
)

# STM32F4 specific defines (default chip: STM32F407xx; override via -DSTM32F4_CHIP=STM32F405xx etc.)
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

# Compiler flags
set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -Wall -Wextra -Os -ffunction-sections -fdata-sections")
set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -fno-builtin -ffreestanding")

# Linker flags
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--gc-sections -Wl,--print-memory-usage -nostartfiles")

# STM32F4 HAL sources
set(HAL_PLATFORM_SOURCES
    ${CMAKE_SOURCE_DIR}/components/hal/stm32/stm32f4/*.c
)

# Include directories: XY HAL headers + stm32f4 impl dir (provides stm32_hal.h shim)
# + SDK headers from MCU/ST/STM32F4
set(PLATFORM_INCLUDE_DIRS
    ${CMAKE_SOURCE_DIR}/components/hal/stm32/stm32f4
    ${CMAKE_SOURCE_DIR}/components/hal/stm32/inc
    ${CMAKE_SOURCE_DIR}/MCU/ST/STM32F4/Drivers/STM32F4xx_HAL_Driver/Inc
    ${CMAKE_SOURCE_DIR}/MCU/ST/STM32F4/Drivers/CMSIS/Device/ST/STM32F4xx/Include
    ${CMAKE_SOURCE_DIR}/MCU/ST/STM32F4/Drivers/CMSIS/Include
)

message(STATUS "STM32F4 Platform Configuration")
message(STATUS "  CPU: Cortex-M4 with FPU")
message(STATUS "  Chip: ${STM32F4_CHIP}")
message(STATUS "  HAL Sources: ${HAL_PLATFORM_SOURCES}")
