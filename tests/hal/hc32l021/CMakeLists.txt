# HC32L021 HAL Driver Tests (PC Simulation)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../third_party/unity
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../components/hal/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../components/hal/PC
)

# GPIO PC Simulation Test
add_executable(test_gpio_pc_simulation 
    test_gpio_pc_simulation.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../third_party/unity/unity.c
)
target_link_libraries(test_gpio_pc_simulation xy_hal)

# Set platform-specific definitions
target_compile_definitions(test_gpio_pc_simulation PRIVATE XY_PLATFORM_PC)