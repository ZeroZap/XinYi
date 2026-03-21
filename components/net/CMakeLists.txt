cmake_minimum_required(VERSION 3.10)
project(xy_net VERSION 1.0.0 LANGUAGES C)

# Set C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Compiler options
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Include directories
include_directories(
    inc
    modbus
    xy_mqtt
    ${CMAKE_CURRENT_SOURCE_DIR}/../../components/trace/xy_log/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../../components/kernel/osal
    ${CMAKE_CURRENT_SOURCE_DIR}/../../components/clib/xy_clib
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

# Collect source files (only core network files)
set(NET_SOURCES
    src/xy_net.c
    src/xy_net_platform.c
)

# Add protocol sources based on configuration
# Modbus (enabled)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/modbus/mb_slave.c)
    list(APPEND NET_SOURCES modbus/mb_slave.c)
endif()

# CAN (disabled - incomplete implementation)
# if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/src/xy_can.c)
#     list(APPEND NET_SOURCES src/xy_can.c)
# endif()

# LTE (disabled by default)
# if(CONFIG_NET_LTE AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/src/xy_lte.c)
#     list(APPEND NET_SOURCES src/xy_lte.c)
# endif()

# MQTT (disabled - incomplete implementation)  
# if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/xy_mqtt/xy_mqtt.c)
#     list(APPEND NET_SOURCES xy_mqtt/xy_mqtt.c)
# endif()

# Collect headers
file(GLOB_RECURSE NET_HEADERS "inc/*.h")

# Create static library
add_library(xy_net STATIC ${NET_SOURCES})

# Set library properties
set_target_properties(xy_net PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    PUBLIC_HEADER "${NET_HEADERS}"
)

# Link dependencies
target_link_libraries(xy_net 
    xy_trace  # For logging
)

# Windows system needs additional libraries
if(WIN32)
    target_link_libraries(xy_net PRIVATE advapi32)
endif()

# Installation rules
install(TARGETS xy_net
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    PUBLIC_HEADER DESTINATION include
)

# Packaging
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)