cmake_minimum_required(VERSION 3.10)
project(xy_xy_clib VERSION 1.0.0 LANGUAGES C)

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
include_directories(inc)

# Collect source files
file(GLOB_RECURSE COMPONENT_SOURCES "*.c")
list(FILTER COMPONENT_SOURCES EXCLUDE REGEX "test/.*")
list(FILTER COMPONENT_SOURCES EXCLUDE REGEX "examples/.*")
list(FILTER COMPONENT_SOURCES EXCLUDE REGEX "_example\\.c$")

file(GLOB_RECURSE COMPONENT_HEADERS "inc/*.h")

# Create static library
add_library(xy_xy_clib STATIC ${COMPONENT_SOURCES})

# Set library properties
set_target_properties(xy_xy_clib PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    PUBLIC_HEADER "${COMPONENT_HEADERS}"
)

# Windows system needs additional libraries
if(WIN32)
    target_link_libraries(xy_xy_clib PRIVATE advapi32)
endif()

# Tests are provided by the repository-level tests/unit suite.
# Run `make test-unit` from the repository root, or focus this component with:
#   cmake --build build/tests/unit --target test_clib -j"$(nproc)"
#   ctest --test-dir build/tests/unit -R '^clib_component$' --output-on-failure

# Installation rules
install(TARGETS xy_xy_clib
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    PUBLIC_HEADER DESTINATION include
)

# Packaging
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)