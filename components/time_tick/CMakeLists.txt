cmake_minimum_required(VERSION 3.10)
project(xy_time_tick VERSION 1.0.0 LANGUAGES C)

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

file(GLOB_RECURSE COMPONENT_HEADERS "inc/*.h")
file(GLOB TEST_SOURCES "test/*.c")

# Create static library
add_library(xy_time_tick STATIC ${COMPONENT_SOURCES})

# Set library properties
set_target_properties(xy_time_tick PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    PUBLIC_HEADER "${COMPONENT_HEADERS}"
)

# Windows system needs additional libraries
if(WIN32)
    target_link_libraries(xy_time_tick PRIVATE advapi32)
endif()

# Create test executable if test sources exist
if(TEST_SOURCES)
    add_executable(test_time_tick ${TEST_SOURCES})
    target_link_libraries(test_time_tick xy_time_tick)
    
    if(WIN32)
        target_link_libraries(test_time_tick advapi32)
    endif()
    
    # Enable testing
    enable_testing()
    add_test(NAME time_tick_test COMMAND test_time_tick)
endif()

# Installation rules
install(TARGETS xy_time_tick
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    PUBLIC_HEADER DESTINATION include
)

# Packaging
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
