cmake_minimum_required(VERSION 3.10)
project(net_mqtt_example VERSION 1.0.0 LANGUAGES C)

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
    ../../components/net/src
    ../../components/trace/xy_log/inc
    ../../components/kernel/osal/inc
    ../../components/clib/xy_clib
    ../../components/hal/PC
)

# Source files
set(SOURCE_FILES
    net_mqtt_example.c
)

# Create executable
add_executable(net_mqtt_example ${SOURCE_FILES})

# Link dependencies
target_link_libraries(net_mqtt_example
    xy_net      # Network components including MQTT
    xy_trace    # Logging
    xy_osal     # OS Abstraction Layer
    xy_hal      # Hardware Abstraction Layer (PC simulation)
)

# Installation rules
install(TARGETS net_mqtt_example
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
