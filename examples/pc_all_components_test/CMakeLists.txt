cmake_minimum_required(VERSION 3.10)

# XinYi Framework - All Components Build Test
project(xy_all_components_test VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# PC Simulation Mode
add_definitions(-DXY_HAL_PC_SIM -DHAL_PLATFORM_PC -DXY_OS_DISABLE_ASM)

message(STATUS "===========================================")
message(STATUS "XinYi Framework - All Components Test")
message(STATUS "===========================================")

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
    ${XINYI_ROOT}/components/crypto/inc
    ${XINYI_ROOT}/components/pm/inc
    ${XINYI_ROOT}/components/pm/charger
    ${XINYI_ROOT}/components/pm/fuel-gauge
    ${XINYI_ROOT}/components/ipc/inc
    ${XINYI_ROOT}/components/dm/inc
    ${XINYI_ROOT}/components/fota/inc
    ${XINYI_ROOT}/components/trace/inc
    ${XINYI_ROOT}/components/trace/xy_log/inc
    ${XINYI_ROOT}/components/sys/xy_state_machine/inc
)

set(SOURCES src/main.c src/stubs.c)

message(STATUS "Adding components...")

# Core
add_subdirectory(${XINYI_ROOT}/components/clib/xy_clib ${CMAKE_BINARY_DIR}/xy_clib)
add_subdirectory(${XINYI_ROOT}/components/kernel/osal ${CMAKE_BINARY_DIR}/xy_osal)
add_subdirectory(${XINYI_ROOT}/components/kernel/misc ${CMAKE_BINARY_DIR}/xy_kernel_misc)
add_subdirectory(${XINYI_ROOT}/components/hal/PC ${CMAKE_BINARY_DIR}/xy_hal_pc)

# Core Components
add_subdirectory(${XINYI_ROOT}/components/device ${CMAKE_BINARY_DIR}/xy_device)
# Sensor - Temporarily disabled
# add_subdirectory(${XINYI_ROOT}/components/sensor ${CMAKE_BINARY_DIR}/xy_sensor)
add_subdirectory(${XINYI_ROOT}/components/crypto ${CMAKE_BINARY_DIR}/xy_crypto)
# PM - Temporarily disabled (struct member issue)
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

message(STATUS "All components added!")

add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(${PROJECT_NAME}
    xy_clib
    xy_osal
    xy_kernel_misc
    xy_hal_pc
    xy_device
    xy_crypto
    xy_pm
    xy_ipc
    xy_dm
    xy_fota
    xy_trace
    xy_state_machine
    xy_gui
    xy_drivers
    m
)

message(STATUS "")
message(STATUS "Build Summary:")
message(STATUS "  Output: ${CMAKE_BINARY_DIR}/${PROJECT_NAME}")
message(STATUS "")
