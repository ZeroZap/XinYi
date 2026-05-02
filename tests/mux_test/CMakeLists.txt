# XY_MUX Tests CMakeLists.txt
# Provides unit tests for MUX components

cmake_minimum_required(VERSION 3.15)
project(xy_mux_tests C)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Enable warnings
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Include directories
set(INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/../components/mux/inc
)

# Test files
set(TEST_FILES
    test_mux_core.c
    test_mux_gpio.c
    test_mux_i2c.c
)

# Create tests
foreach(TEST ${TEST_FILES})
    get_filename_component(TEST_NAME ${TEST} NAME_WE)

    add_executable(${TEST_NAME} ${TEST})

    target_include_directories(${TEST_NAME} PRIVATE ${INCLUDE_DIRS})

    # Link with required libraries
    target_link_libraries(${TEST_NAME} PRIVATE
        xy_mux
    )

    # Set output directory
    set_target_properties(${TEST_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin
    )

    # Add custom test target
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endforeach()

# Group tests in IDE
add_custom_target(tests)
foreach(TEST ${TEST_FILES})
    get_filename_component(TEST_NAME ${TEST} NAME_WE)
    add_dependencies(tests ${TEST_NAME})
endforeach()

# Print configuration
message(STATUS "XY_MUX Tests Configuration:")
message(STATUS "  Source directory: ${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS "  Include directories: ${INCLUDE_DIRS}")
message(STATUS "  Tests: ${TEST_FILES}")
