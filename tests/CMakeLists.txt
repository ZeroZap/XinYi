# XinYi Framework Tests CMake Configuration

cmake_minimum_required(VERSION 3.10)

# Test executables
add_executable(at_client_test at_client_test.c)
add_executable(at_server_test at_server_test.c)

# Link against XinYi framework
target_link_libraries(at_client_test xy_net xy_misc xy_osal xy_trace xy_xy_clib)
target_link_libraries(at_server_test xy_net xy_misc xy_osal xy_trace xy_xy_clib)

# Include directories
target_include_directories(at_client_test PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/../components/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../components/hal/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../components/kernel/misc/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../components/kernel/misc
    ${CMAKE_CURRENT_SOURCE_DIR}/../components/kernel/osal/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../components/net/at/atc/core
    ${CMAKE_CURRENT_SOURCE_DIR}/../components/trace/xy_log/inc
)

target_include_directories(at_server_test PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/../components/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../components/hal/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../components/kernel/misc/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../components/kernel/misc
    ${CMAKE_CURRENT_SOURCE_DIR}/../components/kernel/osal/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../components/net/at/ats
    ${CMAKE_CURRENT_SOURCE_DIR}/../components/trace/xy_log/inc
)

# Compiler definitions for PC testing
target_compile_definitions(at_client_test PRIVATE XY_PLATFORM_PC XY_LOG_LEVEL=3)
target_compile_definitions(at_server_test PRIVATE XY_PLATFORM_PC XY_LOG_LEVEL=3)

# Add HC32L021 HAL tests subdirectory
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/hal/hc32l021/CMakeLists.txt)
    add_subdirectory(hal/hc32l021)
endif()