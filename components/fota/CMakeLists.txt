# FOTA Component CMakeLists.txt
# XinYi Framework - FOTA Component

cmake_minimum_required(VERSION 3.10)

# Check if FOTA is enabled (Kconfig generates CONFIG_FOTA_ENABLED)
if(NOT CONFIG_FOTA_ENABLED)
    return()
endif()

message(STATUS "=== FOTA Component: Building ===")

# Source files - main + src/
file(GLOB FOTA_ROOT_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
)
list(FILTER FOTA_ROOT_SOURCES EXCLUDE REGEX "test/.*")

file(GLOB FOTA_SRC_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
)

set(FOTA_SOURCES
    ${FOTA_ROOT_SOURCES}
    ${FOTA_SRC_SOURCES}
)

# External Flash support is implemented through the generic flash ops/backup hooks in
# xy_fota.c and the tracked src/*.c helpers. Do not append a board-specific NOR source
# here unless that file actually exists; enabling FOTA_EXTERNAL_FLASH must remain a
# valid compile-time configuration even when no dedicated NOR backend is present.
if(CONFIG_FOTA_EXTERNAL_FLASH AND CONFIG_NOR_FLASH_ENABLED)
    message(STATUS "   FOTA: External Flash hooks enabled")
else()
    message(STATUS "   FOTA: Using internal Flash")
endif()

# Create static library before attaching include directories/properties.
add_library(xy_fota STATIC ${FOTA_SOURCES})
add_library(fota_component ALIAS xy_fota)

# Include directories
target_include_directories(xy_fota PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../crypto/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../hal/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../device/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../drivers/storage/flash/w25q128
    ${CMAKE_CURRENT_SOURCE_DIR}/../kernel/osal/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../trace/xy_log/inc
)

# Secure FOTA has no built-in signature verifier. Consumers must inject a
# reviewed provider at runtime; the security-rejected ECDSA placeholder is not
# linked as an authentication fallback.

# Compile definitions
target_compile_definitions(xy_fota PRIVATE
    FOTA_ENABLED=1
)

if(CONFIG_FOTA_DUAL_BANK)
    target_compile_definitions(xy_fota PRIVATE FOTA_DUAL_BANK=1)
endif()

if(CONFIG_FOTA_ROLLBACK)
    target_compile_definitions(xy_fota PRIVATE FOTA_ROLLBACK=1)
endif()

if(CONFIG_FOTA_EXTERNAL_FLASH)
    target_compile_definitions(xy_fota PRIVATE FOTA_EXTERNAL_FLASH=1)
endif()

# Link dependencies
target_link_libraries(xy_fota PUBLIC
    xy_trace
    xy_clib
)

# Add HAL dependency for STM32
if(XY_PLATFORM_STM32)
    target_link_libraries(xy_fota PUBLIC xy_hal)
endif()

# Compiler options
target_compile_options(xy_fota PRIVATE -Wall -Wextra -O2)

message(STATUS "=== FOTA Component: Done ===")