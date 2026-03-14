cmake_minimum_required(VERSION 3.10)

# XinYi STM32U5 GUI Demo
project(stm32u5_gui_demo VERSION 1.0.0 LANGUAGES C ASM)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# MCU Configuration
set(MCU_TARGET "STM32U599")
set(CPU_ARCH "cortex-m33")
set(FPU "fpv5-sp-d16")
set(FPU_FLAGS "-mfloat-abi=hard -mfpu=${FPU}")

message(STATUS "Building for ${MCU_TARGET} (${CPU_ARCH})")

# XinYi Framework Root
set(XINYI_ROOT ${CMAKE_SOURCE_DIR}/../..)

# STM32U5 HAL Library (assumes STM32CubeU5 is installed)
# Adjust this path based on your STM32CubeU5 installation
set(STM32_CUBE_U5_PATH ${XINYI_ROOT}/third_party/STM32CubeU5)

if(NOT EXISTS ${STM32_CUBE_U5_PATH})
    message(WARNING "STM32CubeU5 not found at ${STM32_CUBE_U5_PATH}")
    message(WARNING "Please install STM32CubeU5 or adjust the path")
endif()

# Compiler Flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mcpu=${CPU_ARCH} ${FPU_FLAGS}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wpedantic")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffunction-sections -fdata-sections")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DSTM32U599xx -DUSE_HAL_DRIVER")

# Linker Flags
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -mcpu=${CPU_ARCH} ${FPU_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -specs=nano.specs")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--gc-sections")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--print-memory-usage")

# Include Directories
include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${XINYI_ROOT}/components/gui/inc
    ${XINYI_ROOT}/components/hal/inc
    ${XINYI_ROOT}/components/hal/stm32/stm32u5
    ${XINYI_ROOT}/components/kernel/osal/inc
    ${STM32_CUBE_U5_PATH}/Drivers/STM32U5xx_HAL_Driver/Inc
    ${STM32_CUBE_U5_PATH}/Drivers/CMSIS/Device/ST/STM32U5xx/Include
    ${STM32_CUBE_U5_PATH}/Drivers/CMSIS/Include
)

# Source Files
set(SOURCES
    src/main.c
    src/stm32u5xx_it.c
    src/system_stm32u5xx.c
    src/stm32u5xx_hal_conf.h
)

# XinYi GUI Library
add_subdirectory(${XINYI_ROOT}/components/gui ${CMAKE_BINARY_DIR}/xy_gui)

# XinYi HAL (STM32U5)
add_subdirectory(${XINYI_ROOT}/components/hal/stm32/stm32u5 ${CMAKE_BINARY_DIR}/xy_hal_stm32u5)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Link libraries
target_link_libraries(${PROJECT_NAME}
    xy_gui
    xy_hal_stm32u5
)

# Output binary
set_target_properties(${PROJECT_NAME} PROPERTIES
    SUFFIX ".elf"
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Generate binary/hex files
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${PROJECT_NAME}> ${CMAKE_BINARY_DIR}/bin/${PROJECT_NAME}.bin
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${PROJECT_NAME}> ${CMAKE_BINARY_DIR}/bin/${PROJECT_NAME}.hex
    COMMENT "Generating .bin and .hex files"
)

message(STATUS "STM32U5 GUI Demo configured")
message(STATUS "  Output: ${CMAKE_BINARY_DIR}/bin/${PROJECT_NAME}.elf")
