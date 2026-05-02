# XY_MUX Examples CMakeLists.txt
# Provides example applications for MUX components

cmake_minimum_required(VERSION 3.15)
project(xy_mux_examples C)

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

# Example files
set(EXAMPLE_FILES
    example_gpio.c
    example_i2c.c
    example_spi.c
    example_uart.c
)

# Create examples
foreach(EXAMPLE ${EXAMPLE_FILES})
    get_filename_component(EXAMPLE_NAME ${EXAMPLE} NAME_WE)

    add_executable(${EXAMPLE_NAME} ${EXAMPLE})

    target_include_directories(${EXAMPLE_NAME} PRIVATE ${INCLUDE_DIRS})

    # Link with required libraries
    target_link_libraries(${EXAMPLE_NAME} PRIVATE
        xy_mux
    )

    # Set output directory
    set_target_properties(${EXAMPLE_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin
    )
endforeach()

# Group examples in IDE
add_custom_target(examples)
foreach(EXAMPLE ${EXAMPLE_FILES})
    get_filename_component(EXAMPLE_NAME ${EXAMPLE} NAME_WE)
    add_dependencies(examples ${EXAMPLE_NAME})
endforeach()

# Print configuration
message(STATUS "XY_MUX Examples Configuration:")
message(STATUS "  Source directory: ${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS "  Include directories: ${INCLUDE_DIRS}")
message(STATUS "  Examples: ${EXAMPLE_FILES}")
