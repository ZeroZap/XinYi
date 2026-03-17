# STM32U5 Platform Configuration
# ARM Cortex-M33 with TrustZone

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

# ARM GCC toolchain settings
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)
set(CMAKE_OBJCOPY arm-none-eabi-objcopy)
set(CMAKE_SIZE arm-none-eabi-size)

# CPU flags for STM32U5 (Cortex-M33)
set(CPU_FLAGS
    -mcpu=cortex-m33
    -mthumb
    -mfloat-abi=hard
    -mfpu=fpv5-sp-d16
)

# STM32U5 specific defines
set(PLATFORM_DEFINES
    HAL_PLATFORM_STM32
    PLATFORM_STM32U5
    STM32U5
    STM32U5xx
    USE_HAL_DRIVER
    ARM_CORTEX_M33
    FPU
)

# Compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CPU_FLAGS}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Os")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffunction-sections -fdata-sections")

# Linker flags
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${CPU_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--gc-sections")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--print-memory-usage")

# STM32U5 HAL sources
set(HAL_PLATFORM_SOURCES
    ${CMAKE_SOURCE_DIR}/components/hal/stm32/stm32u5/*.c
    ${CMAKE_SOURCE_DIR}/components/hal/stm32/src/*.c
)

# Include directories
set(PLATFORM_INCLUDE_DIRS
    ${CMAKE_SOURCE_DIR}/components/hal/stm32/stm32u5
    ${CMAKE_SOURCE_DIR}/components/hal/stm32/inc
)

message(STATUS "STM32U5 Platform Configuration")
message(STATUS "  CPU: Cortex-M33 with FPU")
message(STATUS "  HAL Sources: ${HAL_PLATFORM_SOURCES}")
