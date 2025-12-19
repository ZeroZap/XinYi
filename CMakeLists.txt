cmake_minimum_required(VERSION 3.10)
project(XY_Framework VERSION 1.0.0 LANGUAGES C)

# Set C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Compiler options
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Component directories
set(COMPONENTS
    crypto
    xy_clib
    dm
    net
    device
    trace
    osal
    Bank
    sensor
    ipc
    time_tick
    xy_key
    xy_state_machine
    fota
    kernel
    misc
    pm
    xfer
    xy_code_style
)

# Add each component as a subdirectory
foreach(component ${COMPONENTS})
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/components/${component}/CMakeLists.txt)
        add_subdirectory(components/${component})
    endif()
endforeach()

# Top-level targets
add_custom_target(all_components
    COMMENT "Building all components"
)

add_custom_target(clean_all
    COMMENT "Cleaning all components"
)

add_custom_target(test_all
    COMMENT "Testing all components"
)

# Installation rules
install(DIRECTORY components/
    DESTINATION components
    FILES_MATCHING PATTERN "*.h"
)

# Packaging
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)