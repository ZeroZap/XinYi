# FOTA Examples CMakeLists.txt
#
# This CMake file builds all FOTA example applications
# Reference: SECURE_FOTA.md, xy_fota.h

cmake_minimum_required(VERSION 3.15)
project(fota_examples C)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Build options
option(BUILD_EXAMPLES "Build FOTA examples" ON)

# Source directory
set(FOTA_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../components/fota)

# Include paths
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../../components/fota/inc
)

# Executable definitions
set(FOTA_EXAMPLES
    example_basic
    example_secure
    example_dual_bank
    example_flash_layout
)

# Build each example
foreach(EXAMPLE ${FOTA_EXAMPLES})
    set(EXAMPLE_SRC ${CMAKE_CURRENT_SOURCE_DIR}/${EXAMPLE}.c)

    if(NOT EXISTS ${EXAMPLE_SRC})
        message(WARNING "Example source not found: ${EXAMPLE_SRC}")
        continue()
    endif()

    add_executable(${EXAMPLE} ${EXAMPLE_SRC})

    # Link with math library for basic examples
    target_link_libraries(${EXAMPLE} m)

    # Set output name
    set_target_properties(${EXAMPLE} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin
    )

    # Add custom target for running example
    add_custom_target(run_${EXAMPLE}
        COMMAND ${CMAKE_CURRENT_BINARY_DIR}/bin/${EXAMPLE}
        DEPENDS ${EXAMPLE}
        COMMENT "Running ${EXAMPLE}..."
    )

    message(STATUS "Added example: ${EXAMPLE}")
endforeach()

# Print build summary
message(STATUS "")
message(STATUS "FOTA Examples Build Summary:")
message(STATUS "===========================")
message(STATUS "Source dir: ${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS "FOTA component: ${FOTA_SRC_DIR}")
message(STATUS "Examples: ${FOTA_EXAMPLES}")
message(STATUS "")

# Create a helpful target to run all examples
add_custom_target(run_all_fota_examples
    COMMAND echo "Run individual examples with: make run_<example_name>"
    COMMENT "FOTA examples build complete"
)

# Installation support
install(TARGETS ${FOTA_EXAMPLES}
        RUNTIME DESTINATION bin/fota_examples
)

# Print help message
add_custom_command(TARGET run_all_fota_examples POST_BUILD
    COMMAND echo ""
    COMMAND echo "Available examples to run:"
    foreach(EXAMPLE ${FOTA_EXAMPLES})
        COMMAND echo "  make run_${EXAMPLE}  - Run ${EXAMPLE}"
    endforeach()
    COMMAND echo ""
    COMMENT "FOTA Examples Help"
)
