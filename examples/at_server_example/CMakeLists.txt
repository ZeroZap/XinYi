cmake_minimum_required(VERSION 3.10)
project(at_server_example VERSION 1.0.0 LANGUAGES C)

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
    ../../components/net/at/ats
    ../../components/trace/xy_log/inc
    ../../components/kernel/osal/inc
    ../../components/clib/xy_clib
    ../../components/hal/PC
)

# Source files
set(SOURCE_FILES
    at_server_example.c
)

# Create executable
add_executable(at_server_example ${SOURCE_FILES})

# Link dependencies
target_link_libraries(at_server_example 
    xy_net      # AT Server and Client
    xy_trace    # Logging
    xy_osal     # OS Abstraction Layer
    xy_hal      # Hardware Abstraction Layer (PC simulation)
)

# Installation rules
install(TARGETS at_server_example
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)