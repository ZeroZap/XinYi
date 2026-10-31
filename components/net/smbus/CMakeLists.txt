# SMBus Component CMakeLists.txt
# SMBus / PMBus 协议栈
#
# Opt-in through Kconfig (`PROTO_SMBUS`, "SMBus / PMBus"): the library belongs to a build only when
# that symbol is on, and `cmake/Kconfig.cmake` turns it into the CMake variable `XY_PROTO_SMBUS`.
#
# The gate used to read `XY_CONFIG_SMBUS_ENABLED`, which is a preprocessor macro in
# components/xy_config.h and never a CMake variable, so `NOT <undefined> AND NOT <undefined>` was
# TRUE and this file returned before declaring its library - from every configure, always. Nothing
# added the directory either, so the target this file promises had never existed. The
# `check_dead_cmake_options` gate now refuses a condition on a name CMake cannot hold.

cmake_minimum_required(VERSION 3.10)

if(NOT XY_PROTO_SMBUS)
    return()
endif()

# SMBus sources
set(SMBUS_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/xy_smbus.c
    ${CMAKE_CURRENT_SOURCE_DIR}/xy_pmbus.c
)

# Add library
add_library(xy_smbus STATIC ${SMBUS_SOURCES})

target_include_directories(xy_smbus PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# `xy_log` is the header, not a target: linking it only ever emitted `-lxy_log` at link time.
target_include_directories(xy_smbus PRIVATE
    ${CMAKE_SOURCE_DIR}/components/trace/xy_log/inc
)
target_link_libraries(xy_smbus PRIVATE xy_trace)

target_compile_definitions(xy_smbus PRIVATE XY_COMPONENT_SMBUS=1)
target_compile_options(xy_smbus PRIVATE -Wall -Wextra)
