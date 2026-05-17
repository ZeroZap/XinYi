cmake_minimum_required(VERSION 3.10)
project(xy_drivers VERSION 1.0.0 LANGUAGES C)

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
    ../device/inc
    ../device
    ../hal/inc
    ../kernel/osal
    ../kernel/osal/inc
    ../trace/xy_log/inc
    ../pm/fuel-gauge
    ../pm/charger
    ../pm/inc
)

# Add subdirectories for organized drivers
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/sensor)
    add_subdirectory(sensor)
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/display)
    add_subdirectory(display)
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/storage)
    add_subdirectory(storage)
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/power)
    add_subdirectory(power)
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/wireless)
    add_subdirectory(wireless)
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/system)
    add_subdirectory(system)
endif()

# Collect source files
file(GLOB_RECURSE COMPONENT_SOURCES "*.c")
list(FILTER COMPONENT_SOURCES EXCLUDE REGEX "test/.*")
list(FILTER COMPONENT_SOURCES EXCLUDE REGEX "samples/.*")
list(FILTER COMPONENT_SOURCES EXCLUDE REGEX "examples/.*")
# Display LCD drivers use old xy_gpio_write / xy_spi_transmit API (incompatible
# with current xy_hal_gpio / xy_hal_spi signatures) — exclude until refactored.
list(FILTER COMPONENT_SOURCES EXCLUDE REGEX ".*/display/.*")

file(GLOB_RECURSE COMPONENT_HEADERS "inc/*.h")
file(GLOB TEST_SOURCES "test/*.c")

# Create static library
add_library(xy_drivers STATIC ${COMPONENT_SOURCES})

# Set library properties
set_target_properties(xy_drivers PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    PUBLIC_HEADER "${COMPONENT_HEADERS}"
)

# Windows system needs additional libraries
if(WIN32)
    target_link_libraries(xy_drivers PRIVATE advapi32)
endif()

# Create test executable if test sources exist
if(TEST_SOURCES)
    add_executable(test_drivers ${TEST_SOURCES})
    target_link_libraries(test_drivers xy_drivers)

    if(WIN32)
        target_link_libraries(test_drivers advapi32)
    endif()

    # Enable testing
    enable_testing()
    add_test(NAME drivers_test COMMAND test_drivers)
endif()

# Installation rules
install(TARGETS xy_drivers
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    PUBLIC_HEADER DESTINATION include
)

# Packaging
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
