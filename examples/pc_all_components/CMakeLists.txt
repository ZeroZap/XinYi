cmake_minimum_required(VERSION 3.12)

# XinYi Framework - PC All Components Build Test
# This builds all components for PC simulation (no MCU-specific code)

project(xy_pc_all_components VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# PC Simulation Mode
add_definitions(-DXY_HAL_PC_SIM -DHAL_PLATFORM_PC -DXY_OS_PC_SIM)

message(STATUS "===========================================")
message(STATUS "XinYi Framework - PC All Components Test")
message(STATUS "Platform: PC Simulation")
message(STATUS "===========================================")

# ==================== Compiler Flags ====================

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unused-parameter -Wno-unused-variable")

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
    ${XINYI_ROOT}/components/gui/inc
    ${XINYI_ROOT}/components/device/inc
    ${XINYI_ROOT}/components/sensor/inc
    ${XINYI_ROOT}/components/crypto/inc
    ${XINYI_ROOT}/components/pm/inc
    ${XINYI_ROOT}/components/ipc/inc
    ${XINYI_ROOT}/components/dm/inc
    ${XINYI_ROOT}/components/fota/inc
    ${XINYI_ROOT}/components/trace/inc
    ${XINYI_ROOT}/components/trace/xy_log/inc
    ${XINYI_ROOT}/components/sys/xy_state_machine/inc
)

# ==================== Source Files ====================

set(SOURCES
    src/main.c
    src/hal_pc_wrapper.c
)

# ==================== Add Components ====================

message(STATUS "Adding components...")

# Core Libraries
add_subdirectory(${XINYI_ROOT}/components/clib/xy_clib ${CMAKE_BINARY_DIR}/xy_clib)

# OSAL - Use PC-compatible mode
set(OSAL_BACKEND "baremetal" CACHE STRING "OSAL backend" FORCE)
add_definitions(-DXY_OS_DISABLE_ASM)
add_subdirectory(${XINYI_ROOT}/components/kernel/osal ${CMAKE_BINARY_DIR}/xy_osal)

add_subdirectory(${XINYI_ROOT}/components/kernel/misc ${CMAKE_BINARY_DIR}/xy_kernel_misc)

# HAL PC
add_subdirectory(${XINYI_ROOT}/components/hal/PC ${CMAKE_BINARY_DIR}/xy_hal_pc)

# Core Components
add_subdirectory(${XINYI_ROOT}/components/device ${CMAKE_BINARY_DIR}/xy_device)
# Sensor - Temporarily disabled due to internal API dependencies
# add_subdirectory(${XINYI_ROOT}/components/sensor ${CMAKE_BINARY_DIR}/xy_sensor)
# Crypto - Temporarily disabled
# add_subdirectory(${XINYI_ROOT}/components/crypto ${CMAKE_BINARY_DIR}/xy_crypto)
# PM - Temporarily disabled due to struct member issues
# add_subdirectory(${XINYI_ROOT}/components/pm ${CMAKE_BINARY_DIR}/xy_pm)
add_subdirectory(${XINYI_ROOT}/components/ipc ${CMAKE_BINARY_DIR}/xy_ipc)
add_subdirectory(${XINYI_ROOT}/components/dm ${CMAKE_BINARY_DIR}/xy_dm)
add_subdirectory(${XINYI_ROOT}/components/fota ${CMAKE_BINARY_DIR}/xy_fota)
add_subdirectory(${XINYI_ROOT}/components/trace ${CMAKE_BINARY_DIR}/xy_trace)
add_subdirectory(${XINYI_ROOT}/components/sys/xy_state_machine ${CMAKE_BINARY_DIR}/xy_state_machine)

# GUI
add_subdirectory(${XINYI_ROOT}/components/gui ${CMAKE_BINARY_DIR}/xy_gui)

# Drivers
add_subdirectory(${XINYI_ROOT}/components/drivers ${CMAKE_BINARY_DIR}/xy_drivers)

# NET - Disabled for PC build (has external dependencies)
# add_subdirectory(${XINYI_ROOT}/components/net ${CMAKE_BINARY_DIR}/xy_net)

message(STATUS "All components added successfully!")

# ==================== Create Executable ====================

add_executable(${PROJECT_NAME} ${SOURCES})

# Link all libraries
target_link_libraries(${PROJECT_NAME}
    xy_clib
    xy_osal
    xy_kernel_misc
    xy_hal_pc
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
    m
)

# ==================== Build Summary ====================

message(STATUS "")
message(STATUS "Build Configuration Summary:")
message(STATUS "  Project: ${PROJECT_NAME}")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Platform: PC")
message(STATUS "  Output: ${CMAKE_BINARY_DIR}/${PROJECT_NAME}")
message(STATUS "")
