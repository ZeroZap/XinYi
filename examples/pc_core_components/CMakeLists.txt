cmake_minimum_required(VERSION 3.10)

# XinYi Framework - PC Core Components Build Test
# Only includes components that compile cleanly

project(xy_pc_core_components VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# PC Simulation Mode
add_definitions(-DXY_HAL_PC_SIM -DHAL_PLATFORM_PC -DXY_OS_DISABLE_ASM)

message(STATUS "===========================================")
message(STATUS "XinYi Framework - Core Components Test")
message(STATUS "Platform: PC Simulation")
message(STATUS "===========================================")

# ==================== Include Directories ====================

set(XINYI_ROOT ${CMAKE_SOURCE_DIR}/../..)

include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${XINYI_ROOT}/components
    ${XINYI_ROOT}/components/clib/xy_clib/inc
    ${XINYI_ROOT}/components/clib/xy_clib
    ${XINYI_ROOT}/components/hal/inc
    ${XINYI_ROOT}/components/hal/PC
    ${XINYI_ROOT}/components/kernel/osal/inc
    ${XINYI_ROOT}/components/kernel/misc
    ${XINYI_ROOT}/components/gui
    ${XINYI_ROOT}/components/gui/inc
    ${XINYI_ROOT}/components/device/inc
    ${XINYI_ROOT}/components/drivers/inc
    ${XINYI_ROOT}/components/trace/xy_log/inc
    ${XINYI_ROOT}/components/sys/xy_state_machine/inc
)

# ==================== Source Files ====================

set(SOURCES
    src/main.c
    src/stubs.c
)

# ==================== Add Components ====================

message(STATUS "Adding core components...")

# Core Libraries
add_subdirectory(${XINYI_ROOT}/components/clib/xy_clib ${CMAKE_BINARY_DIR}/xy_clib)
add_subdirectory(${XINYI_ROOT}/components/kernel/osal ${CMAKE_BINARY_DIR}/xy_osal)
add_subdirectory(${XINYI_ROOT}/components/kernel/misc ${CMAKE_BINARY_DIR}/xy_kernel_misc)

# HAL PC
add_subdirectory(${XINYI_ROOT}/components/hal/PC ${CMAKE_BINARY_DIR}/xy_hal_pc)

# Device
add_subdirectory(${XINYI_ROOT}/components/device ${CMAKE_BINARY_DIR}/xy_device)

# GUI
add_subdirectory(${XINYI_ROOT}/components/gui ${CMAKE_BINARY_DIR}/xy_gui)

# Drivers
add_subdirectory(${XINYI_ROOT}/components/drivers ${CMAKE_BINARY_DIR}/xy_drivers)

# Trace
add_subdirectory(${XINYI_ROOT}/components/trace ${CMAKE_BINARY_DIR}/xy_trace)

# State Machine
add_subdirectory(${XINYI_ROOT}/components/sys/xy_state_machine ${CMAKE_BINARY_DIR}/xy_state_machine)

message(STATUS "Core components added successfully!")

# ==================== Create Executable ====================

add_executable(${PROJECT_NAME} ${SOURCES})

# Link all libraries
target_link_libraries(${PROJECT_NAME}
    xy_osal
    xy_hal_pc
    xy_gui
    xy_device
    xy_drivers
    xy_trace
    m
)

# ==================== Build Summary ====================

message(STATUS "")
message(STATUS "Build Configuration Summary:")
message(STATUS "  Project: ${PROJECT_NAME}")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Platform: PC")
message(STATUS "  Components: clib, osal, hal, device, gui, drivers, trace, state_machine")
message(STATUS "  Output: ${CMAKE_BINARY_DIR}/${PROJECT_NAME}")
message(STATUS "")
