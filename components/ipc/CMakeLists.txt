cmake_minimum_required(VERSION 3.10)
project(xy_ipc VERSION 1.0.0 LANGUAGES C)

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
    ../kernel/misc
    ../kernel/osal
    ../kernel/osal/inc
    ../trace/xy_log/inc
)

# Collect source files
file(GLOB_RECURSE COMPONENT_SOURCES "*.c")
list(FILTER COMPONENT_SOURCES EXCLUDE REGEX "test/.*")
list(FILTER COMPONENT_SOURCES EXCLUDE REGEX ".*example\\.c$")

file(GLOB_RECURSE COMPONENT_HEADERS "inc/*.h")

# Create static library
add_library(xy_ipc STATIC ${COMPONENT_SOURCES})

# Set library properties
set_target_properties(xy_ipc PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    PUBLIC_HEADER "${COMPONENT_HEADERS}"
)

# Windows system needs additional libraries
if(WIN32)
    target_link_libraries(xy_ipc PRIVATE advapi32)
endif()

# Tests live in the repository-level Unity/CTest suite. Use `make test-unit`
# or focused tests such as `ipc_pipe` and `ipc_broker`.

# Installation rules
install(TARGETS xy_ipc
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    PUBLIC_HEADER DESTINATION include
)

# Packaging
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
