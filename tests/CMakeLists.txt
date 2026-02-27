##
## @file CMakeLists.txt
## @brief XinYi Unified Test Platform
## @version 1.0
## @date 2026-02-28
##

cmake_minimum_required(VERSION 3.12)

project(xy_tests C)

# Enable testing
enable_testing()

# ==================== Configuration ====================

option(BUILD_TESTING "Build tests" ON)
option(TEST_VERBOSITY "Verbose test output" ON)
option(TEST_COVERAGE "Enable code coverage" OFF)

if(NOT BUILD_TESTING)
    message(STATUS "Testing is disabled")
    return()
endif()

# ==================== Unity Framework ====================

set(UNITY_DIR ${CMAKE_SOURCE_DIR}/third_party/unity)

if(NOT EXISTS ${UNITY_DIR})
    message(WARNING "Unity framework not found at ${UNITY_DIR}")
    message(WARNING "Tests will not be built")
    return()
endif()

include_directories(${UNITY_DIR})

# ==================== Component Tests ====================

# Helper function to add component test
function(add_component_test name source_dir test_files)
    add_executable(${name}
        ${UNITY_DIR}/unity.c
        ${test_files}
    )

    target_include_directories(${name} PRIVATE
        ${source_dir}/include
        ${source_dir}
    )

    target_link_libraries(${name} PRIVATE
        ${ARGN}
    )

    if(TEST_COVERAGE)
        target_compile_options(${name} PRIVATE --coverage)
        target_link_options(${name} PRIVATE --coverage)
    endif()

    add_test(NAME ${name} COMMAND ${name})

    if(TEST_VERBOSITY)
        set_tests_properties(${name} PROPERTIES
            LABELS "${name}"
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        )
    endif()

    message(STATUS "Added test: ${name}")
endfunction()

# ==================== OSAL Tests ====================

if(EXISTS ${CMAKE_SOURCE_DIR}/components/kernel/osal/tests)
    add_subdirectory(${CMAKE_SOURCE_DIR}/components/kernel/osal/tests osal_tests)
endif()

# ==================== CLib Tests ====================

if(EXISTS ${CMAKE_SOURCE_DIR}/components/clib/xy_clib/tests)
    add_component_test(test_xy_clib
        ${CMAKE_SOURCE_DIR}/components/clib/xy_clib
        "${CMAKE_SOURCE_DIR}/components/clib/xy_clib/tests/test_filter.c;
         ${CMAKE_SOURCE_DIR}/components/clib/xy_clib/tests/test_sort.c"
    )
endif()

# ==================== Crypto Tests ====================

if(EXISTS ${CMAKE_SOURCE_DIR}/components/crypto/tests)
    add_subdirectory(${CMAKE_SOURCE_DIR}/components/crypto/tests crypto_tests)
endif()

# ==================== Custom Test Targets ====================

# Run all tests
add_custom_target(run_all_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS test_osal
    COMMENT "Running all tests..."
)

# Coverage report
if(TEST_COVERAGE)
    find_program(GCOVR_PATH gcovr)
    if(GCOVR_PATH)
        add_custom_target(coverage
            COMMAND ${GCOVR_PATH} -r ${CMAKE_SOURCE_DIR} --html --html-details -o coverage.html
            COMMENT "Generating coverage report..."
        )
    endif()
endif()

# ==================== Summary ====================

message(STATUS "")
message(STATUS "Test Configuration:")
message(STATUS "  Build Testing: ${BUILD_TESTING}")
message(STATUS "  Verbose Output: ${TEST_VERBOSITY}")
message(STATUS "  Code Coverage: ${TEST_COVERAGE}")
message(STATUS "")
message(STATUS "Available targets:")
message(STATUS "  run_all_tests  - Run all tests")
message(STATUS "  coverage       - Generate coverage report (if TEST_COVERAGE=ON)")
message(STATUS "")
