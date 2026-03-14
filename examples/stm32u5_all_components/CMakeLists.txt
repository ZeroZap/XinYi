cmake_minimum_required(VERSION 3.12)

# XinYi Framework - STM32U5 All Components Build Test
project(xy_stm32u5_all_components VERSION 1.0.0 LANGUAGES C ASM)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# ==================== MCU Configuration ====================

set(MCU_TARGET "STM32U599")
set(CPU_ARCH "cortex-m33")
set(FPU "fpv5-sp-d16")
set(FPU_FLAGS "-mfloat-abi=hard -mfpu=${FPU}")

message(STATUS "===========================================")
message(STATUS "XinYi Framework - All Components Build Test")
message(STATUS "Target: ${MCU_TARGET} (${CPU_ARCH})")
message(STATUS "===========================================")

# ==================== Compiler Flags ====================

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mcpu=${CPU_ARCH} ${FPU_FLAGS}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wpedantic")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffunction-sections -fdata-sections")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DSTM32U599xx -DUSE_HAL_DRIVER")

# Disable warnings for unused variables in test build
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unused-parameter -Wno-unused-variable")

# ==================== Linker Flags ====================

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -mcpu=${CPU_ARCH} ${FPU_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -specs=nano.specs")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--gc-sections")

# ==================== Include Directories ====================

set(XINYI_ROOT ${CMAKE_SOURCE_DIR}/../..)

include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${XINYI_ROOT}/components
    ${XINYI_ROOT}/components/clib/xy_clib/inc
    ${XINYI_ROOT}/components/hal/inc
    ${XINYI_ROOT}/components/hal/stm32/stm32u5
    ${XINYI_ROOT}/components/kernel/osal/inc
    ${XINYI_ROOT}/components/kernel/misc
    ${XINYI_ROOT}/components/gui/inc
    ${XINYI_ROOT}/components/device/inc
    ${XINYI_ROOT}/components/sensor/inc
    ${XINYI_ROOT}/components/crypto/inc
    ${XINYI_ROOT}/components/pm/inc
    ${XINYI_ROOT}/components/ipc/inc
    ${XINYI_ROOT}/components/dm/inc
    ${XINYI_ROOT}/components/fota/inc
    ${XINYI_ROOT}/components/trace/inc
    ${XINYI_ROOT}/components/sys/xy_state_machine/inc
)

# ==================== Source Files ====================

set(SOURCES
    src/main.c
    src/stm32u5xx_it.c
    src/system_stm32u5xx.c
)

# ==================== Add All Components ====================

message(STATUS "Adding components...")

# Core Libraries
add_subdirectory(${XINYI_ROOT}/components/clib/xy_clib ${CMAKE_BINARY_DIR}/xy_clib)
add_subdirectory(${XINYI_ROOT}/components/kernel/osal ${CMAKE_BINARY_DIR}/xy_osal)
add_subdirectory(${XINYI_ROOT}/components/kernel/misc ${CMAKE_BINARY_DIR}/xy_kernel_misc)

# HAL
add_subdirectory(${XINYI_ROOT}/components/hal/stm32/stm32u5 ${CMAKE_BINARY_DIR}/xy_hal_stm32u5)

# Core Components
add_subdirectory(${XINYI_ROOT}/components/device ${CMAKE_BINARY_DIR}/xy_device)
add_subdirectory(${XINYI_ROOT}/components/sensor ${CMAKE_BINARY_DIR}/xy_sensor)
add_subdirectory(${XINYI_ROOT}/components/crypto ${CMAKE_BINARY_DIR}/xy_crypto)
add_subdirectory(${XINYI_ROOT}/components/pm ${CMAKE_BINARY_DIR}/xy_pm)
add_subdirectory(${XINYI_ROOT}/components/ipc ${CMAKE_BINARY_DIR}/xy_ipc)
add_subdirectory(${XINYI_ROOT}/components/dm ${CMAKE_BINARY_DIR}/xy_dm)
add_subdirectory(${XINYI_ROOT}/components/fota ${CMAKE_BINARY_DIR}/xy_fota)
add_subdirectory(${XINYI_ROOT}/components/trace ${CMAKE_BINARY_DIR}/xy_trace)
add_subdirectory(${XINYI_ROOT}/components/sys/xy_state_machine ${CMAKE_BINARY_DIR}/xy_state_machine)

# GUI
add_subdirectory(${XINYI_ROOT}/components/gui ${CMAKE_BINARY_DIR}/xy_gui)

# Drivers
add_subdirectory(${XINYI_ROOT}/components/drivers ${CMAKE_BINARY_DIR}/xy_drivers)

# NET
add_subdirectory(${XINYI_ROOT}/components/net ${CMAKE_BINARY_DIR}/xy_net)

message(STATUS "All components added successfully!")

# ==================== Create Executable ====================

add_executable(${PROJECT_NAME} ${SOURCES})

# Link all libraries
target_link_libraries(${PROJECT_NAME}
    xy_clib
    xy_osal
    xy_kernel_misc
    xy_hal_stm32u5
    xy_device
    xy_sensor
    xy_crypto
    xy_pm
    xy_ipc
    xy_dm
    xy_fota
    xy_trace
    xy_state_machine
    xy_gui
    xy_drivers
    xy_net
)

# ==================== Output Configuration ====================

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

# ==================== Build Summary ====================

message(STATUS "")
message(STATUS "Build Configuration Summary:")
message(STATUS "  Project: ${PROJECT_NAME}")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Target: ${MCU_TARGET}")
message(STATUS "  Output: ${CMAKE_BINARY_DIR}/bin/${PROJECT_NAME}.elf")
message(STATUS "")
