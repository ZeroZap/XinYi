cmake_minimum_required(VERSION 3.10)
project(component_demo C)
set(CMAKE_C_STANDARD 99)

set(PLATFORM "PC" CACHE STRING "Target platform")

if(PLATFORM STREQUAL "PC")
    add_definitions(-DHAL_PLATFORM_PC -DXY_HAL_PC_SIM)
    message(STATUS "Building for PC simulation")
endif()

option(DEMO_OSAL "Enable OSAL demo" ON)
option(DEMO_DEVICE "Enable Device demo" ON)
option(DEMO_CRYPTO "Enable Crypto demo" ON)

include_directories(
    ${CMAKE_SOURCE_DIR}/../../components/kernel/osal
    ${CMAKE_SOURCE_DIR}/../../components/hal/inc
    ${CMAKE_SOURCE_DIR}/../../components/device
    ${CMAKE_SOURCE_DIR}/../../components/device/inc
)

set(SOURCES main.c osal_pc.c)

if(DEMO_DEVICE)
    list(APPEND SOURCES demo_device.c)
endif()

if(DEMO_OSAL)
    list(APPEND SOURCES demo_osal.c)
endif()

if(DEMO_CRYPTO)
    list(APPEND SOURCES demo_crypto.c)
endif()

add_executable(component_demo ${SOURCES})
target_link_libraries(component_demo m)
install(TARGETS component_demo DESTINATION bin)
