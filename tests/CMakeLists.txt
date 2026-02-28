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

# CLib component tests (unified in tests/ directory)
if(EXISTS ${CMAKE_SOURCE_DIR}/components/clib/xy_clib)
    add_component_test(test_xy_clib
        ${CMAKE_SOURCE_DIR}/components/clib/xy_clib
        "${CMAKE_SOURCE_DIR}/tests/test_xy_clib.c"
    )
endif()

# ==================== Crypto Tests ====================

# Crypto component tests (unified in tests/ directory)
if(EXISTS ${CMAKE_SOURCE_DIR}/components/crypto)
    add_component_test(test_crypto
        ${CMAKE_SOURCE_DIR}/components/crypto
        "${CMAKE_SOURCE_DIR}/tests/test_crypto.c"
    )
    target_include_directories(test_crypto PRIVATE
        ${CMAKE_SOURCE_DIR}/components/crypto/xymd
        ${CMAKE_SOURCE_DIR}/components/crypto/xy_aes
        ${CMAKE_SOURCE_DIR}/components/crypto/xy_base
        ${CMAKE_SOURCE_DIR}/components/crypto/xy_crc
        ${CMAKE_SOURCE_DIR}/components/crypto/xy_hex
        ${CMAKE_SOURCE_DIR}/components/crypto/xy_hmac
        ${CMAKE_SOURCE_DIR}/components/crypto/xy_rng
    )
endif()

# ==================== Custom Test Targets ====================

# Trace component tests (unified in tests/ directory)
if(EXISTS ${CMAKE_SOURCE_DIR}/components/trace)
    add_component_test(test_trace
        ${CMAKE_SOURCE_DIR}/components/trace
        "${CMAKE_SOURCE_DIR}/tests/test_trace.c"
    )
    target_include_directories(test_trace PRIVATE
        ${CMAKE_SOURCE_DIR}/components/trace/xy_log/inc
    )
endif()

# ==================== DM (Data Management) Tests ====================

# DM component tests (unified in tests/ directory)
if(EXISTS ${CMAKE_SOURCE_DIR}/components/dm)
    add_component_test(test_dm
        ${CMAKE_SOURCE_DIR}/components/dm
        "${CMAKE_SOURCE_DIR}/tests/test_dm.c"
    )
    target_include_directories(test_dm PRIVATE
        ${CMAKE_SOURCE_DIR}/components/dm/xy_tlv
    )
endif()

# ==================== NET (Network) Tests ====================

# NET component tests (unified in tests/ directory)
if(EXISTS ${CMAKE_SOURCE_DIR}/components/net)
    add_component_test(test_net
        ${CMAKE_SOURCE_DIR}/components/net
        "${CMAKE_SOURCE_DIR}/tests/test_net.c"
    )
    target_include_directories(test_net PRIVATE
        ${CMAKE_SOURCE_DIR}/components/net/xy_iso7816
        ${CMAKE_SOURCE_DIR}/components/net/modbus
    )
endif()

# ==================== Sensor Tests ====================

# Sensor component tests (unified in tests/ directory)
if(EXISTS ${CMAKE_SOURCE_DIR}/components/sensor)
    add_component_test(test_sensor
        ${CMAKE_SOURCE_DIR}/components/sensor
        "${CMAKE_SOURCE_DIR}/tests/test_sensor.c"
    )
    target_include_directories(test_sensor PRIVATE
        ${CMAKE_SOURCE_DIR}/components/sensor
    )
endif()

# ==================== IPC Tests ====================

# IPC component tests (unified in tests/ directory)
if(EXISTS ${CMAKE_SOURCE_DIR}/components/ipc)
    add_component_test(test_ipc
        ${CMAKE_SOURCE_DIR}/components/ipc
        "${CMAKE_SOURCE_DIR}/tests/test_ipc.c"
    )
    target_include_directories(test_ipc PRIVATE
        ${CMAKE_SOURCE_DIR}/components/ipc/pipe
        ${CMAKE_SOURCE_DIR}/components/ipc/observer
    )
endif()

# ==================== PM (Power Management) Tests ====================

# PM component tests (unified in tests/ directory)
if(EXISTS ${CMAKE_SOURCE_DIR}/components/pm)
    add_component_test(test_pm
        ${CMAKE_SOURCE_DIR}/components/pm
        "${CMAKE_SOURCE_DIR}/tests/test_pm.c"
    )
    target_include_directories(test_pm PRIVATE
        ${CMAKE_SOURCE_DIR}/components/pm/charger
        ${CMAKE_SOURCE_DIR}/components/pm/fuel-gauge
    )
endif()

# ==================== HAL Tests ====================

# HAL component tests (unified in tests/ directory)
if(EXISTS ${CMAKE_SOURCE_DIR}/components/hal)
    add_component_test(test_hal
        ${CMAKE_SOURCE_DIR}/components/hal
        "${CMAKE_SOURCE_DIR}/tests/test_hal.c"
    )
    target_include_directories(test_hal PRIVATE
        ${CMAKE_SOURCE_DIR}/components/hal/inc
    )
endif()

# ==================== PID Tests ====================

# PID component tests (unified in tests/ directory)
if(EXISTS ${CMAKE_SOURCE_DIR}/components/pid)
    add_component_test(test_pid
        ${CMAKE_SOURCE_DIR}/components/pid
        "${CMAKE_SOURCE_DIR}/tests/test_pid.c"
    )
    target_include_directories(test_pid PRIVATE
        ${CMAKE_SOURCE_DIR}/components/pid
    )
endif()

# ==================== ADDC Tests ====================

# ADDC component tests (unified in tests/ directory)
if(EXISTS ${CMAKE_SOURCE_DIR}/components/addc)
    add_component_test(test_addc
        ${CMAKE_SOURCE_DIR}/components/addc
        "${CMAKE_SOURCE_DIR}/tests/test_addc.c"
    )
    target_include_directories(test_addc PRIVATE
        ${CMAKE_SOURCE_DIR}/components/addc
    )
endif()

# ==================== Device Tests ====================

# Device component tests (unified in tests/ directory)
if(EXISTS ${CMAKE_SOURCE_DIR}/components/device)
    add_component_test(test_device
        ${CMAKE_SOURCE_DIR}/components/device
        "${CMAKE_SOURCE_DIR}/tests/test_device.c"
    )
    target_include_directories(test_device PRIVATE
        ${CMAKE_SOURCE_DIR}/components/device
    )
endif()

# Run all tests
add_custom_target(run_all_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS test_osal test_crypto test_xy_clib test_trace test_dm test_net test_sensor test_ipc test_pm test_hal test_pid test_addc test_device
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
