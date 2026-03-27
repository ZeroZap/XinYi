# DM (Device Management) Component CMakeLists.txt

cmake_minimum_required(VERSION 3.10)

if(NOT XY_COMPONENT_DM)
    return()
endif()

# Collect DM source files
set(DM_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/xy_json.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/xy_fs.c
)

# Create DM library
add_library(xy_dm STATIC ${DM_SOURCES})

target_include_directories(xy_dm PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/coreJSON
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/components/trace/xy_log/inc
)

target_compile_definitions(xy_dm PRIVATE COMPONENT_DM=1)

# Link dependencies
target_link_libraries(xy_dm 
    xy_trace
    xy_clib
)

# Compiler options
target_compile_options(xy_dm PRIVATE -Wall -Wextra -O2)
