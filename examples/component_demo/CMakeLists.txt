cmake_minimum_required(VERSION 3.10)
project(component_demo C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Platform selection
set(PLATFORM "PC" CACHE STRING "Target platform (PC/STM32)")

# Compile definitions
if(PLATFORM STREQUAL "PC")
    add_definitions(-DHAL_PLATFORM_PC)
    add_definitions(-DXY_HAL_PC_SIM)
    message(STATUS "Building for PC simulation")
endif()

# Demo options
option(DEMO_OSAL "Enable OSAL demo" ON)
option(DEMO_DEVICE "Enable Device demo" ON)
option(DEMO_CRYPTO "Enable Crypto demo" ON)

# Source files
set(SOURCES
    main.c
    osal_pc.c
)

if(DEMO_DEVICE)
    list(APPEND SOURCES demo_device.c)
    add_definitions(-DDEMO_DEVICE)
endif()

if(DEMO_OSAL)
    list(APPEND SOURCES demo_osal.c)
    add_definitions(-DDEMO_OSAL)
endif()

if(DEMO_CRYPTO)
    list(APPEND SOURCES demo_crypto.c)
    add_definitions(-DDEMO_CRYPTO)
endif()

# Create executable
add_executable(component_demo ${SOURCES})

# Installation
install(TARGETS component_demo DESTINATION bin)
