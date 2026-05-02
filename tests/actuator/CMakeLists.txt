# XinYi Actuator Tests CMake Configuration
cmake_minimum_required(VERSION 3.10)

# Unity framework location
set(UNITY_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../UniTest/unity)
set(ACTUATOR_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../components/actuator)

# Include paths
set(ACTUATOR_TEST_INCLUDES
    ${ACTUATOR_SOURCE_DIR}
    ${UNITY_SOURCE_DIR}
)

# Compile definitions
set(ACTUATOR_TEST_DEFINITIONS
    XY_PLATFORM_PC=1
)

# ==================== Test: Actuator Core ====================
add_executable(test_actuator_core
    ${CMAKE_CURRENT_SOURCE_DIR}/test_actuator_core.c
    ${ACTUATOR_SOURCE_DIR}/xy_actuator.c
    ${UNITY_SOURCE_DIR}/unity.c
)
target_include_directories(test_actuator_core PRIVATE ${ACTUATOR_TEST_INCLUDES})
target_compile_definitions(test_actuator_core PRIVATE ${ACTUATOR_TEST_DEFINITIONS})
target_link_libraries(test_actuator_core m)

# ==================== Test: Actuator Relay ====================
add_executable(test_actuator_relay
    ${CMAKE_CURRENT_SOURCE_DIR}/test_actuator_relay.c
    ${ACTUATOR_SOURCE_DIR}/xy_actuator.c
    ${UNITY_SOURCE_DIR}/unity.c
)
target_include_directories(test_actuator_relay PRIVATE ${ACTUATOR_TEST_INCLUDES})
target_compile_definitions(test_actuator_relay PRIVATE ${ACTUATOR_TEST_DEFINITIONS})
target_link_libraries(test_actuator_relay m)

# ==================== Test: Actuator PWM ====================
add_executable(test_actuator_pwm
    ${CMAKE_CURRENT_SOURCE_DIR}/test_actuator_pwm.c
    ${ACTUATOR_SOURCE_DIR}/xy_actuator.c
    ${UNITY_SOURCE_DIR}/unity.c
)
target_include_directories(test_actuator_pwm PRIVATE ${ACTUATOR_TEST_INCLUDES})
target_compile_definitions(test_actuator_pwm PRIVATE ${ACTUATOR_TEST_DEFINITIONS})
target_link_libraries(test_actuator_pwm m)

# ==================== Test: Actuator Servo ====================
add_executable(test_actuator_servo
    ${CMAKE_CURRENT_SOURCE_DIR}/test_actuator_servo.c
    ${ACTUATOR_SOURCE_DIR}/xy_actuator.c
    ${UNITY_SOURCE_DIR}/unity.c
)
target_include_directories(test_actuator_servo PRIVATE ${ACTUATOR_TEST_INCLUDES})
target_compile_definitions(test_actuator_servo PRIVATE ${ACTUATOR_TEST_DEFINITIONS})
target_link_libraries(test_actuator_servo m)

# ==================== Test Summary ====================
add_custom_target(actuator_tests
    test_actuator_core
    test_actuator_relay
    test_actuator_pwm
    test_actuator_servo
    COMMENT "Building all actuator tests..."
)

add_custom_target(run_actuator_tests
    COMMAND test_actuator_core
    COMMAND test_actuator_relay
    COMMAND test_actuator_pwm
    COMMAND test_actuator_servo
    COMMENT "Running all actuator tests..."
    DEPENDS actuator_tests
)
