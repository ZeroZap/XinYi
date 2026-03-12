cmake_minimum_required(VERSION 3.10)
project(component_demo C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Platform selection
set(PLATFORM "PC" CACHE STRING "Target platform (PC/STM32/WCH)")

# Compile definitions
if(PLATFORM STREQUAL "PC")
    add_definitions(-DHAL_PLATFORM_PC)
    add_definitions(-DXY_HAL_PC_SIM)
    message(STATUS "Building for PC simulation")
else()
    message(STATUS "Building for ${PLATFORM}")
endif()

# Log level
set(LOG_LEVEL "DEBUG" CACHE STRING "Log level (DEBUG/INFO/WARN/ERROR)")
add_definitions(-DLOG_LEVEL_${LOG_LEVEL})

# Demo options
option(DEMO_OSAL "Enable OSAL demo" ON)
option(DEMO_HAL "Enable HAL demo" ON)
option(DEMO_DEVICE "Enable Device demo" ON)
option(DEMO_SENSOR "Enable Sensor demo" ON)
option(DEMO_CRYPTO "Enable Crypto demo" ON)
option(DEMO_PM "Enable Power Management demo" ON)

# Source files
set(SOURCES
    main.c
)

if(DEMO_OSAL)
    list(APPEND SOURCES demo_osal.c)
    add_definitions(-DDEMO_OSAL)
endif()

if(DEMO_HAL)
    list(APPEND SOURCES demo_hal.c)
    add_definitions(-DDEMO_HAL)
endif()

if(DEMO_DEVICE)
    list(APPEND SOURCES demo_device.c)
    add_definitions(-DDEMO_DEVICE)
endif()

if(DEMO_SENSOR)
    list(APPEND SOURCES demo_sensor.c)
    add_definitions(-DDEMO_SENSOR)
endif()

if(DEMO_CRYPTO)
    list(APPEND SOURCES demo_crypto.c)
    add_definitions(-DDEMO_CRYPTO)
endif()

if(DEMO_PM)
    list(APPEND SOURCES demo_pm.c)
    add_definitions(-DDEMO_PM)
endif()

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/../../components/kernel/osal/inc
    ${CMAKE_SOURCE_DIR}/../../components/kernel/misc/inc
    ${CMAKE_SOURCE_DIR}/../../components/hal/inc
    ${CMAKE_SOURCE_DIR}/../../components/hal
    ${CMAKE_SOURCE_DIR}/../../components/device
    ${CMAKE_SOURCE_DIR}/../../components/device/inc
    ${CMAKE_SOURCE_DIR}/../../components/sensor/inc
    ${CMAKE_SOURCE_DIR}/../../components/crypto/inc
    ${CMAKE_SOURCE_DIR}/../../components/clib/xy_clib
    ${CMAKE_SOURCE_DIR}/../../components/pm
)

# Source directories
include_directories(
    ${CMAKE_SOURCE_DIR}/../../components/kernel/osal/backend/baremetal
    ${CMAKE_SOURCE_DIR}/../../components/hal/PC
)

# Collect HAL PC sources
set(HAL_PC_SOURCES
    ${CMAKE_SOURCE_DIR}/../../components/hal/PC/xy_hal_pc.c
    ${CMAKE_SOURCE_DIR}/../../components/hal/PC/xy_hal_pin_pc.c
    ${CMAKE_SOURCE_DIR}/../../components/hal/PC/xy_hal_i2c_pc.c
    ${CMAKE_SOURCE_DIR}/../../components/hal/PC/xy_hal_spi_pc.c
    ${CMAKE_SOURCE_DIR}/../../components/hal/PC/xy_hal_uart_pc.c
)

# Collect OSAL baremetal sources
set(OSAL_SOURCES
    ${CMAKE_SOURCE_DIR}/../../components/kernel/osal/baremetal/xy_os_baremetal.c
)

# Collect component sources
set(DEVICE_SOURCES
    ${CMAKE_SOURCE_DIR}/../../components/device/xy_device.c
    ${CMAKE_SOURCE_DIR}/../../components/device/xy_device_core.c
)

set(SENSOR_SOURCES
    ${CMAKE_SOURCE_DIR}/../../components/sensor/src/xy_sht30.c
    ${CMAKE_SOURCE_DIR}/../../components/sensor/src/xy_mpu6050.c
)

set(CRYPTO_SOURCES
    ${CMAKE_SOURCE_DIR}/../../components/crypto/src/xy_aes.c
    ${CMAKE_SOURCE_DIR}/../../components/crypto/src/xy_sha256.c
    ${CMAKE_SOURCE_DIR}/../../components/crypto/src/xy_crc.c
)

set(CLIB_SOURCES
    ${CMAKE_SOURCE_DIR}/../../components/clib/xy_clib/xy_string.c
    ${CMAKE_SOURCE_DIR}/../../components/clib/xy_clib/xy_printf.c
)

set(KERNEL_SOURCES
    ${CMAKE_SOURCE_DIR}/../../components/kernel/misc/src/xy_log.c
)

# Create executable
add_executable(component_demo
    ${SOURCES}
    ${HAL_PC_SOURCES}
    ${OSAL_SOURCES}
    ${DEVICE_SOURCES}
    ${SENSOR_SOURCES}
    ${CRYPTO_SOURCES}
    ${CLIB_SOURCES}
    ${KERNEL_SOURCES}
)

# Link libraries (none for PC, just compile all sources)
target_link_libraries(component_demo m)

# Installation
install(TARGETS component_demo DESTINATION bin)
